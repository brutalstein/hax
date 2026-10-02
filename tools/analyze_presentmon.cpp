#include "hax/core/model.hpp"
#include "hax/core/optimizer.hpp"
#include "hax/core/statistics.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

struct Capture {
  std::string label;
  hax::core::CandidateProfile profile;
  hax::core::SampleSeries samples;
  std::string latency_source;
};

std::vector<std::string> parse_csv_row(const std::string& line) {
  std::vector<std::string> fields;
  std::string field;
  bool quoted = false;

  for (std::size_t i = 0; i < line.size(); ++i) {
    const char ch = line[i];
    if (ch == '"') {
      if (quoted && i + 1 < line.size() && line[i + 1] == '"') {
        field.push_back('"');
        ++i;
      } else {
        quoted = !quoted;
      }
    } else if (ch == ',' && !quoted) {
      fields.push_back(std::move(field));
      field.clear();
    } else {
      field.push_back(ch);
    }
  }
  fields.push_back(std::move(field));
  return fields;
}

std::optional<double> parse_number(const std::string& value) {
  if (value.empty() || value == "NA" || value == "N/A") {
    return std::nullopt;
  }

  char* end = nullptr;
  const double parsed = std::strtod(value.c_str(), &end);
  if (end == value.c_str() || *end != '\0' || !std::isfinite(parsed)) {
    return std::nullopt;
  }
  return parsed;
}

std::optional<std::size_t> column(
    const std::map<std::string, std::size_t>& columns,
    const std::string_view name) {
  const auto it = columns.find(std::string(name));
  if (it == columns.end()) {
    return std::nullopt;
  }
  return it->second;
}

void append_if_valid(
    const std::vector<std::string>& row,
    const std::optional<std::size_t> index,
    std::vector<double>& output,
    const bool require_positive = true) {
  if (!index || *index >= row.size()) {
    return;
  }
  const auto value = parse_number(row[*index]);
  if (!value) {
    return;
  }
  if (require_positive && *value <= 0.0) {
    return;
  }
  if (*value > 10000.0) {
    return;
  }
  output.push_back(*value);
}

hax::core::CandidateProfile profile_for(const std::string& label) {
  hax::core::CandidateProfile profile;

  if (label == "browser-default") {
    profile.frame = hax::core::FramePolicy::browser_default;
    profile.priority = hax::core::PriorityPolicy::normal;
    profile.disable_vsync = false;
  } else {
    profile.frame = hax::core::FramePolicy::uncapped;
    profile.disable_vsync = true;
  }

  if (label.find("highgpu") != std::string::npos) {
    profile.gpu = hax::core::GpuPreference::high_performance;
  } else if (label.find("lowgpu") != std::string::npos) {
    profile.gpu = hax::core::GpuPreference::low_power;
  }

  if (label.find("pcores") != std::string::npos) {
    profile.cpu = hax::core::CpuPolicy::prefer_performance_cores;
  }

  if (label.find("highprio") != std::string::npos) {
    profile.priority = hax::core::PriorityPolicy::high;
  }

  return profile;
}

std::optional<Capture> load_capture(
    const std::string& label,
    const std::string& path) {
  std::ifstream input(path);
  if (!input) {
    std::cerr << "Cannot open capture: " << path << '\n';
    return std::nullopt;
  }

  std::string line;
  if (!std::getline(input, line)) {
    return std::nullopt;
  }
  if (!line.empty() && line.back() == '\r') {
    line.pop_back();
  }

  const auto headers = parse_csv_row(line);
  std::map<std::string, std::size_t> columns;
  for (std::size_t i = 0; i < headers.size(); ++i) {
    columns.emplace(headers[i], i);
  }

  const auto all_input = column(columns, "MsAllInputToPhotonLatency");
  const auto click_input = column(columns, "MsClickToPhotonLatency");
  const auto display_latency = column(columns, "DisplayLatency");
  const auto between_presents = column(columns, "MsBetweenPresents");
  const auto until_displayed = column(columns, "MsUntilDisplayed");

  std::vector<double> input_latency;
  std::vector<double> click_latency;
  std::vector<double> display;
  std::vector<double> frame;
  std::vector<double> present_to_display;

  while (std::getline(input, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    const auto row = parse_csv_row(line);
    append_if_valid(row, all_input, input_latency);
    append_if_valid(row, click_input, click_latency);
    append_if_valid(row, display_latency, display);
    append_if_valid(row, between_presents, frame);
    append_if_valid(row, until_displayed, present_to_display);
  }

  constexpr std::size_t minimum_input_samples = 120;

  Capture capture;
  capture.label = label;
  capture.profile = profile_for(label);

  if (input_latency.size() >= minimum_input_samples) {
    capture.samples.pc_latency_ms = std::move(input_latency);
    capture.latency_source = "all-input-to-photon";
  } else if (click_latency.size() >= minimum_input_samples) {
    capture.samples.pc_latency_ms = std::move(click_latency);
    capture.latency_source = "click-to-photon";
  } else {
    capture.samples.pc_latency_ms = std::move(display);
    capture.latency_source = "frame-start-to-display";
  }

  capture.samples.frame_time_ms = std::move(frame);
  capture.samples.present_to_display_ms = std::move(present_to_display);

  // Uncapped rendering normally produces presents that are superseded before
  // scan-out. They are not classified as stability failures here.
  capture.samples.dropped_frame_ratio = 0.0;
  capture.samples.minimum_thermal_headroom_c = 100.0;
  capture.samples.cpu_utilization = 0.0;
  capture.samples.gpu_utilization = 0.0;

  if (capture.samples.pc_latency_ms.empty() ||
      capture.samples.frame_time_ms.empty()) {
    std::cerr << "Capture lacks required timing metrics: " << path << '\n';
    return std::nullopt;
  }

  return capture;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr
        << "Usage: hax_capture_analyzer label=path.csv "
        << "[label=path.csv ...]\n";
    return 2;
  }

  std::vector<Capture> captures;
  captures.reserve(static_cast<std::size_t>(argc - 1));

  for (int i = 1; i < argc; ++i) {
    const std::string argument(argv[i]);
    const auto equals = argument.find('=');
    if (equals == std::string::npos) {
      std::cerr << "Expected label=path: " << argument << '\n';
      return 2;
    }

    auto capture = load_capture(
        argument.substr(0, equals),
        argument.substr(equals + 1));
    if (!capture) {
      return 3;
    }
    captures.push_back(std::move(*capture));
  }

  const auto baseline_it = std::find_if(
      captures.begin(),
      captures.end(),
      [](const Capture& capture) {
        return capture.label == "browser-default";
      });
  if (baseline_it == captures.end()) {
    std::cerr << "browser-default baseline is required.\n";
    return 4;
  }

  const auto baseline_summary =
      hax::core::summarize(baseline_it->samples);
  hax::core::Optimizer optimizer;

  const auto baseline_eval = optimizer.evaluate(
      baseline_it->profile,
      baseline_it->samples,
      baseline_summary);
  if (!baseline_eval.feasible) {
    std::cerr << "Baseline is not feasible: "
              << baseline_eval.rejection_reason << '\n';
    return 5;
  }

  std::vector<hax::core::EvaluatedCandidate> evaluated;
  evaluated.reserve(captures.size());

  for (const auto& capture : captures) {
    auto result = optimizer.evaluate(
        capture.profile,
        capture.samples,
        baseline_summary);

    std::cout
        << "RESULT label=" << capture.label
        << " source=" << capture.latency_source
        << " feasible=" << (result.feasible ? "yes" : "no");

    if (result.feasible) {
      std::cout
          << std::fixed
          << std::setprecision(4)
          << " score=" << result.score
          << " latency_p50_ms=" << result.summary.latency_p50_ms
          << " latency_p99_ms=" << result.summary.latency_p99_ms
          << " frame_p99_ms=" << result.summary.frame_p99_ms
          << " frame_mad_ms=" << result.summary.frame_mad_ms
          << " present_p95_ms="
          << result.summary.present_to_display_p95_ms;
    } else {
      std::cout << " reason=" << result.rejection_reason;
    }

    std::cout << '\n';
    evaluated.push_back(std::move(result));
  }

  const auto best =
      optimizer.choose_best(evaluated, baseline_eval);
  if (!best) {
    std::cerr << "No feasible candidate.\n";
    return 6;
  }

  const auto winner = std::find_if(
      captures.begin(),
      captures.end(),
      [&best](const Capture& capture) {
        return capture.profile.id() == best->profile.id();
      });

  if (winner == captures.end()) {
    std::cerr << "Internal error resolving winning label.\n";
    return 7;
  }

  std::cout << "BEST=" << winner->label << '\n';
  return 0;
}
