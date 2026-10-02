#include "hax/core/optimizer.hpp"

#include "hax/core/statistics.hpp"

#include <algorithm>
#include <cmath>

namespace hax::core {
namespace {

double safe_ratio(
    const double value,
    const double baseline,
    const double epsilon = 1e-6) {
  return value / std::max(std::abs(baseline), epsilon);
}

double utilization_penalty(const double utilization) {
  const double normalized = std::clamp(utilization / 100.0, 0.0, 1.0);
  return normalized * normalized;
}

double thermal_penalty(const double headroom_c) {
  if (headroom_c >= 20.0) {
    return 0.0;
  }
  if (headroom_c <= 0.0) {
    return 4.0;
  }
  return (20.0 - headroom_c) / 20.0;
}

}  // namespace

Optimizer::Optimizer(OptimizerConfig config) : config_(config) {}

EvaluatedCandidate Optimizer::evaluate(
    const CandidateProfile& profile,
    const SampleSeries& samples,
    const Summary& baseline) const {
  EvaluatedCandidate result;
  result.profile = profile;
  result.summary = summarize(samples);

  if (result.summary.sample_count < config_.minimum_samples) {
    result.rejection_reason = "insufficient-samples";
    return result;
  }
  if (result.summary.dropped_frame_ratio > config_.max_dropped_frame_ratio) {
    result.rejection_reason = "dropped-frames";
    return result;
  }
  if (result.summary.thermal_headroom_c <
      config_.minimum_thermal_headroom_c) {
    result.rejection_reason = "thermal-headroom";
    return result;
  }
  if (result.summary.frame_drift_ratio >
      config_.maximum_frame_drift_ratio) {
    result.rejection_reason = "frame-time-drift";
    return result;
  }
  if (result.summary.latency_p50_ms <= 0.0 ||
      result.summary.frame_p50_ms <= 0.0) {
    result.rejection_reason = "invalid-timing";
    return result;
  }

  double present_component = 0.0;
  if (baseline.present_to_display_p95_ms > 0.0) {
    if (result.summary.present_to_display_p95_ms <= 0.0) {
      result.rejection_reason = "missing-present-timing";
      return result;
    }

    present_component = safe_ratio(
        result.summary.present_to_display_p95_ms,
        baseline.present_to_display_p95_ms);
  }

  const double uncertainty =
      safe_ratio(1.96 * result.summary.latency_standard_error_ms,
                 baseline.latency_p50_ms);

  result.score =
      config_.weight_latency_p50 *
          safe_ratio(result.summary.latency_p50_ms, baseline.latency_p50_ms) +
      config_.weight_latency_p99 *
          safe_ratio(result.summary.latency_p99_ms, baseline.latency_p99_ms) +
      config_.weight_frame_p99 *
          safe_ratio(result.summary.frame_p99_ms, baseline.frame_p99_ms) +
      config_.weight_frame_jitter *
          safe_ratio(
              result.summary.frame_mad_ms,
              std::max(baseline.frame_mad_ms, 0.01)) +
      config_.weight_present_to_display * present_component +
      config_.weight_cpu *
          utilization_penalty(result.summary.cpu_utilization) +
      config_.weight_gpu *
          utilization_penalty(result.summary.gpu_utilization) +
      config_.weight_thermal *
          thermal_penalty(result.summary.thermal_headroom_c) +
      config_.weight_hybrid_present *
          result.summary.hybrid_present_ratio +
      config_.uncertainty_weight * uncertainty;

  result.feasible = std::isfinite(result.score);
  if (!result.feasible) {
    result.rejection_reason = "non-finite-score";
  }
  return result;
}

std::optional<EvaluatedCandidate> Optimizer::choose_best(
    const std::vector<EvaluatedCandidate>& candidates,
    const EvaluatedCandidate& baseline) const {
  const EvaluatedCandidate* best = nullptr;

  for (const auto& candidate : candidates) {
    if (!candidate.feasible) {
      continue;
    }
    if (best == nullptr || candidate.score < best->score) {
      best = &candidate;
    }
  }

  if (best == nullptr) {
    return std::nullopt;
  }
  if (!baseline.feasible) {
    return *best;
  }

  const double required =
      baseline.score * (1.0 - config_.minimum_improvement_ratio);
  if (best->score >= required) {
    return baseline;
  }
  return *best;
}

}  // namespace hax::core
