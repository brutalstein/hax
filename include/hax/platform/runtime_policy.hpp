#pragma once

#include "hax/core/model.hpp"

namespace hax::platform {

struct AppliedPolicy {
  bool priority_applied{false};
  bool cpu_sets_applied{false};
  bool power_policy_applied{false};
};

[[nodiscard]] AppliedPolicy apply_runtime_policy(
    const hax::core::CandidateProfile& profile);

}  // namespace hax::platform
