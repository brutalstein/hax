#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace hax::core {

enum class FramePolicy : std::uint8_t {
  browser_default,
  uncapped,
};

enum class GpuPreference : std::uint8_t {
  system_default,
  low_power,
  high_performance,
};

enum class CpuPolicy : std::uint8_t {
  system_default,
  prefer_performance_cores,
};

enum class PriorityPolicy : std::uint8_t {
  normal,
  above_normal,
  high,
};

struct CandidateProfile {
  FramePolicy frame{FramePolicy::uncapped};
  GpuPreference gpu{GpuPreference::system_default};
  CpuPolicy cpu{CpuPolicy::system_default};
  PriorityPolicy priority{PriorityPolicy::above_normal};
  bool disable_vsync{true};
  bool disable_power_throttling{true};
  bool honor_timer_resolution{true};

  [[nodiscard]] std::string id() const;
};

struct SampleSeries {
  std::vector<double> pc_latency_ms;
  std::vector<double> frame_time_ms;
  std::vector<double> present_to_display_ms;
  double dropped_frame_ratio{0.0};
  double cpu_utilization{0.0};
  double gpu_utilization{0.0};
  double minimum_thermal_headroom_c{100.0};
  double duration_seconds{0.0};
};

struct Summary {
  double latency_p50_ms{0.0};
  double latency_p95_ms{0.0};
  double latency_p99_ms{0.0};
  double frame_p50_ms{0.0};
  double frame_p99_ms{0.0};
  double frame_mad_ms{0.0};
  double present_to_display_p95_ms{0.0};
  double mean_latency_ms{0.0};
  double latency_standard_error_ms{0.0};
  double dropped_frame_ratio{0.0};
  double cpu_utilization{0.0};
  double gpu_utilization{0.0};
  double thermal_headroom_c{100.0};
  std::size_t sample_count{0};
};

struct EvaluatedCandidate {
  CandidateProfile profile;
  Summary summary;
  double score{0.0};
  bool feasible{false};
  std::string rejection_reason;
};

struct OptimizerConfig {
  double max_dropped_frame_ratio{0.01};
  double minimum_thermal_headroom_c{8.0};
  std::size_t minimum_samples{120};
  double minimum_improvement_ratio{0.01};

  double weight_latency_p50{0.34};
  double weight_latency_p99{0.26};
  double weight_frame_p99{0.16};
  double weight_frame_jitter{0.10};
  double weight_present_to_display{0.08};
  double weight_cpu{0.03};
  double weight_gpu{0.02};
  double weight_thermal{0.01};
  double uncertainty_weight{0.20};
};

}  // namespace hax::core
