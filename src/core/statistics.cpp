#include "hax/core/statistics.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace hax::core {
namespace {

double mean(const std::vector<double>& values) {
  if (values.empty()) {
    return 0.0;
  }
  return std::accumulate(values.begin(), values.end(), 0.0) /
         static_cast<double>(values.size());
}

double standard_error(const std::vector<double>& values, const double sample_mean) {
  if (values.size() < 2) {
    return 0.0;
  }

  double sum_sq = 0.0;
  for (const double value : values) {
    const double delta = value - sample_mean;
    sum_sq += delta * delta;
  }
  const double variance = sum_sq / static_cast<double>(values.size() - 1);
  return std::sqrt(variance / static_cast<double>(values.size()));
}

std::vector<double> sorted_copy(const std::vector<double>& values) {
  auto copy = values;
  std::sort(copy.begin(), copy.end());
  return copy;
}

double segment_median(
    const std::vector<double>& values,
    const std::size_t begin,
    const std::size_t end) {
  if (begin >= end || end > values.size()) {
    return 0.0;
  }

  std::vector<double> segment(
      values.begin() + static_cast<std::ptrdiff_t>(begin),
      values.begin() + static_cast<std::ptrdiff_t>(end));
  std::sort(segment.begin(), segment.end());
  return percentile_sorted(segment, 0.5);
}

}  // namespace

double percentile_sorted(const std::vector<double>& sorted, const double q) {
  if (sorted.empty()) {
    return 0.0;
  }
  if (q <= 0.0) {
    return sorted.front();
  }
  if (q >= 1.0) {
    return sorted.back();
  }

  const double position = q * static_cast<double>(sorted.size() - 1);
  const auto lower = static_cast<std::size_t>(std::floor(position));
  const auto upper = static_cast<std::size_t>(std::ceil(position));
  if (lower == upper) {
    return sorted[lower];
  }

  const double fraction = position - static_cast<double>(lower);
  return sorted[lower] * (1.0 - fraction) + sorted[upper] * fraction;
}

double median_absolute_deviation(
    const std::vector<double>& values,
    const double median) {
  if (values.empty()) {
    return 0.0;
  }

  std::vector<double> deviations;
  deviations.reserve(values.size());
  for (const double value : values) {
    deviations.push_back(std::abs(value - median));
  }
  std::sort(deviations.begin(), deviations.end());
  return percentile_sorted(deviations, 0.5);
}

Summary summarize(const SampleSeries& samples) {
  Summary out;
  out.sample_count = std::min(
      samples.pc_latency_ms.size(), samples.frame_time_ms.size());
  if (samples.pc_latency_ms.empty() || samples.frame_time_ms.empty()) {
    return out;
  }

  const auto latency = sorted_copy(samples.pc_latency_ms);
  const auto frame = sorted_copy(samples.frame_time_ms);
  const auto present = sorted_copy(samples.present_to_display_ms);

  out.latency_p50_ms = percentile_sorted(latency, 0.50);
  out.latency_p95_ms = percentile_sorted(latency, 0.95);
  out.latency_p99_ms = percentile_sorted(latency, 0.99);
  out.frame_p50_ms = percentile_sorted(frame, 0.50);
  out.frame_p99_ms = percentile_sorted(frame, 0.99);
  out.frame_mad_ms =
      median_absolute_deviation(samples.frame_time_ms, out.frame_p50_ms);

  const std::size_t drift_window =
      std::max<std::size_t>(
          1,
          samples.frame_time_ms.size() / 5);
  const double first_median =
      segment_median(
          samples.frame_time_ms,
          0,
          drift_window);
  const double last_median =
      segment_median(
          samples.frame_time_ms,
          samples.frame_time_ms.size() - drift_window,
          samples.frame_time_ms.size());

  if (first_median > 0.0) {
    out.frame_drift_ratio =
        std::max(
            0.0,
            (last_median - first_median) / first_median);
  }

  out.present_to_display_p95_ms =
      present.empty() ? 0.0 : percentile_sorted(present, 0.95);
  out.mean_latency_ms = mean(samples.pc_latency_ms);
  out.latency_standard_error_ms =
      standard_error(samples.pc_latency_ms, out.mean_latency_ms);
  out.dropped_frame_ratio = samples.dropped_frame_ratio;
  out.hybrid_present_ratio =
      std::clamp(samples.hybrid_present_ratio, 0.0, 1.0);
  out.cpu_utilization = samples.cpu_utilization;
  out.gpu_utilization = samples.gpu_utilization;
  out.thermal_headroom_c = samples.minimum_thermal_headroom_c;
  return out;
}

}  // namespace hax::core
