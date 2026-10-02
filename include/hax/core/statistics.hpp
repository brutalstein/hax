#pragma once

#include "hax/core/model.hpp"

#include <vector>

namespace hax::core {

[[nodiscard]] double percentile_sorted(const std::vector<double>& sorted, double q);
[[nodiscard]] double median_absolute_deviation(const std::vector<double>& values, double median);
[[nodiscard]] Summary summarize(const SampleSeries& samples);

}  // namespace hax::core
