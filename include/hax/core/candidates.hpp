#pragma once

#include "hax/core/model.hpp"

#include <cstdint>
#include <vector>

namespace hax::core {

struct CandidateGenerationInput {
  std::uint32_t refresh_hz{60};
  bool has_multiple_gpus{false};
  bool has_heterogeneous_cpu{false};
};

[[nodiscard]] std::vector<CandidateProfile> generate_candidates(
    const CandidateGenerationInput& input);

}  // namespace hax::core
