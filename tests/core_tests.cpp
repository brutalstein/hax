#include "hax/core/candidates.hpp"
#include "hax/core/optimizer.hpp"
#include "hax/core/statistics.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void require(const bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << '\n';
    std::exit(1);
  }
}

hax::core::SampleSeries samples(
    const double latency,
    const double frame,
    const double jitter,
    const std::size_t count = 1000) {
  hax::core::SampleSeries out;
  out.pc_latency_ms.reserve(count);
  out.frame_time_ms.reserve(count);
  out.present_to_display_ms.reserve(count);

  for (std::size_t i = 0; i < count; ++i) {
    const double wave =
        static_cast<double>(static_cast<int>(i % 11) - 5) / 5.0;
    out.pc_latency_ms.push_back(latency + wave * jitter);
    out.frame_time_ms.push_back(frame + wave * jitter * 0.25);
    out.present_to_display_ms.push_back(0.35 + std::abs(wave) * 0.05);
  }

  out.dropped_frame_ratio = 0.0;
  out.cpu_utilization = 18.0;
  out.gpu_utilization = 9.0;
  out.minimum_thermal_headroom_c = 25.0;
  out.duration_seconds = 10.0;
  return out;
}

}  // namespace

int main() {
  {
    const hax::core::CandidateProfile defaults;
    require(
        defaults.frame == hax::core::FramePolicy::browser_default,
        "unprofiled launch uses browser-default frame policy");
    require(
        defaults.priority == hax::core::PriorityPolicy::normal,
        "unprofiled launch uses normal process priority");
    require(
        !defaults.disable_vsync,
        "unprofiled launch keeps browser vsync behavior");
  }

  {
    const std::vector<double> values{1.0, 2.0, 3.0, 4.0, 5.0};
    require(
        std::abs(hax::core::percentile_sorted(values, 0.5) - 3.0) < 1e-9,
        "median percentile");
  }

  const auto baseline_samples = samples(3.0, 1.0, 0.25);
  const auto baseline_summary = hax::core::summarize(baseline_samples);

  hax::core::Optimizer optimizer;
  hax::core::CandidateProfile baseline_profile;
  baseline_profile.frame = hax::core::FramePolicy::browser_default;
  baseline_profile.disable_vsync = false;

  const auto baseline =
      optimizer.evaluate(baseline_profile, baseline_samples, baseline_summary);
  require(baseline.feasible, "baseline feasible");

  hax::core::CandidateProfile faster_profile;
  faster_profile.frame = hax::core::FramePolicy::uncapped;

  const auto faster =
      optimizer.evaluate(faster_profile, samples(2.2, 0.85, 0.12), baseline_summary);
  require(faster.feasible, "faster feasible");
  require(faster.score < baseline.score, "faster score should be lower");

  auto hybrid_samples = samples(2.2, 0.85, 0.12);
  hybrid_samples.hybrid_present_ratio = 1.0;
  const auto hybrid =
      optimizer.evaluate(
          faster_profile,
          hybrid_samples,
          baseline_summary);
  require(hybrid.feasible, "hybrid candidate feasible");
  require(
      hybrid.score > faster.score,
      "cross-adapter presentation receives a tie-break penalty");

  auto hot_samples = samples(2.0, 0.8, 0.1);
  hot_samples.minimum_thermal_headroom_c = 2.0;
  const auto hot =
      optimizer.evaluate(faster_profile, hot_samples, baseline_summary);
  require(!hot.feasible, "thermally unsafe candidate rejected");


  auto drifting_samples = samples(2.0, 0.8, 0.05);
  for (std::size_t i = drifting_samples.frame_time_ms.size() * 4 / 5;
       i < drifting_samples.frame_time_ms.size();
       ++i) {
    drifting_samples.frame_time_ms[i] *= 1.25;
  }
  const auto drifting =
      optimizer.evaluate(
          faster_profile,
          drifting_samples,
          baseline_summary);
  require(
      !drifting.feasible,
      "frame-time drift candidate rejected");

  const auto best = optimizer.choose_best({baseline, faster, hot}, baseline);
  require(best.has_value(), "best candidate exists");
  require(
      best->profile.frame == hax::core::FramePolicy::uncapped,
      "faster candidate selected");

  const auto candidates = hax::core::generate_candidates({
      .refresh_hz = 240,
      .has_multiple_gpus = true,
      .has_heterogeneous_cpu = true,
  });
  require(!candidates.empty(), "candidate generation");
  require(candidates.size() <= 36, "candidate search remains bounded");

  std::cout << "All hax_core tests passed. candidates="
            << candidates.size() << '\n';
  return 0;
}
