#include "hax/core/candidates.hpp"

#include <sstream>

namespace hax::core {
namespace {

const char* frame_name(const FramePolicy value) {
  switch (value) {
    case FramePolicy::browser_default:
      return "frame-default";
    case FramePolicy::uncapped:
      return "frame-uncapped";
  }
  return "frame-unknown";
}

const char* gpu_name(const GpuPreference value) {
  switch (value) {
    case GpuPreference::system_default:
      return "gpu-default";
    case GpuPreference::low_power:
      return "gpu-low";
    case GpuPreference::high_performance:
      return "gpu-high";
  }
  return "gpu-unknown";
}

const char* cpu_name(const CpuPolicy value) {
  switch (value) {
    case CpuPolicy::system_default:
      return "cpu-default";
    case CpuPolicy::prefer_performance_cores:
      return "cpu-performance";
  }
  return "cpu-unknown";
}

const char* priority_name(const PriorityPolicy value) {
  switch (value) {
    case PriorityPolicy::normal:
      return "prio-normal";
    case PriorityPolicy::above_normal:
      return "prio-above";
    case PriorityPolicy::high:
      return "prio-high";
  }
  return "prio-unknown";
}

}  // namespace

std::string CandidateProfile::id() const {
  std::ostringstream stream;
  stream << frame_name(frame)
         << '-' << gpu_name(gpu)
         << '-' << cpu_name(cpu)
         << '-' << priority_name(priority)
         << (disable_vsync ? "-novsync" : "-vsync");
  return stream.str();
}

std::vector<CandidateProfile> generate_candidates(
    const CandidateGenerationInput& input) {
  const std::vector<FramePolicy> frame_policies{
      FramePolicy::browser_default,
      FramePolicy::uncapped,
  };

  std::vector<GpuPreference> gpu_policies{GpuPreference::system_default};
  if (input.has_multiple_gpus) {
    gpu_policies.push_back(GpuPreference::low_power);
    gpu_policies.push_back(GpuPreference::high_performance);
  }

  std::vector<CpuPolicy> cpu_policies{CpuPolicy::system_default};
  if (input.has_heterogeneous_cpu) {
    cpu_policies.push_back(CpuPolicy::prefer_performance_cores);
  }

  std::vector<CandidateProfile> candidates;
  const std::vector<PriorityPolicy> priority_policies{
      PriorityPolicy::normal,
      PriorityPolicy::above_normal,
      PriorityPolicy::high,
  };

  candidates.reserve(
      frame_policies.size() * gpu_policies.size() *
      cpu_policies.size() * priority_policies.size());

  for (const auto frame : frame_policies) {
    for (const auto gpu : gpu_policies) {
      for (const auto cpu : cpu_policies) {
        for (const auto priority : priority_policies) {
          CandidateProfile candidate;
          candidate.frame = frame;
          candidate.gpu = gpu;
          candidate.cpu = cpu;
          candidate.priority = priority;
          candidate.disable_vsync = frame == FramePolicy::uncapped;
          candidates.push_back(candidate);
        }
      }
    }
  }

  return candidates;
}

}  // namespace hax::core
