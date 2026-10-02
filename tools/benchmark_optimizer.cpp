#include "hax/core/candidates.hpp"
#include "hax/core/optimizer.hpp"
#include "hax/core/statistics.hpp"

#include <chrono>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <iostream>

namespace {

hax::core::SampleSeries synth(
    const std::size_t n,
    const double latency,
    const double frame_time) {
  hax::core::SampleSeries s;
  s.pc_latency_ms.reserve(n);
  s.frame_time_ms.reserve(n);
  s.present_to_display_ms.reserve(n);

  for (std::size_t i = 0; i < n; ++i) {
    const double x = static_cast<double>(i);
    s.pc_latency_ms.push_back(latency + 0.07 * std::sin(x * 0.017));
    s.frame_time_ms.push_back(frame_time + 0.02 * std::cos(x * 0.031));
    s.present_to_display_ms.push_back(0.35 + 0.01 * std::sin(x * 0.011));
  }

  s.cpu_utilization = 20.0;
  s.gpu_utilization = 12.0;
  s.minimum_thermal_headroom_c = 30.0;
  return s;
}

}  // namespace

int main() {
  constexpr std::size_t sample_count = 4096;
  constexpr std::size_t iterations = 8;

  const auto baseline_samples = synth(sample_count, 3.0, 1.0);
  const auto baseline = hax::core::summarize(baseline_samples);

  const auto candidates = hax::core::generate_candidates({
      .refresh_hz = 360,
      .has_multiple_gpus = true,
      .has_heterogeneous_cpu = true,
  });

  hax::core::Optimizer optimizer;
  const auto start = std::chrono::steady_clock::now();

  std::size_t evaluated = 0;
  double checksum = 0.0;

  for (std::size_t iteration = 0; iteration < iterations; ++iteration) {
    for (std::size_t i = 0; i < candidates.size(); ++i) {
      const double factor = 1.0 - static_cast<double>(i % 7) * 0.01;
      const auto result = optimizer.evaluate(
          candidates[i],
          synth(sample_count, 3.0 * factor, 1.0 * factor),
          baseline);
      checksum += result.score;
      ++evaluated;
    }
  }

  const auto elapsed = std::chrono::duration<double, std::milli>(
      std::chrono::steady_clock::now() - start);

  std::cout
      << std::fixed
      << std::setprecision(3)
      << "evaluations=" << evaluated
      << " samples/eval=" << sample_count
      << " total_ms=" << elapsed.count()
      << " us/eval="
      << (elapsed.count() * 1000.0 / static_cast<double>(evaluated))
      << " checksum=" << checksum
      << '\n';

  return 0;
}
