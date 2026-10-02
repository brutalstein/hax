#pragma once

#include "hax/core/model.hpp"

#include <optional>
#include <vector>

namespace hax::core {

class Optimizer final {
 public:
  explicit Optimizer(OptimizerConfig config = {});

  [[nodiscard]] EvaluatedCandidate evaluate(
      const CandidateProfile& profile,
      const SampleSeries& samples,
      const Summary& baseline) const;

  [[nodiscard]] std::optional<EvaluatedCandidate> choose_best(
      const std::vector<EvaluatedCandidate>& candidates,
      const EvaluatedCandidate& baseline) const;

 private:
  OptimizerConfig config_;
};

}  // namespace hax::core
