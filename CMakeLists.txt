cmake_minimum_required(VERSION 3.24)

project(HaxballApp
  VERSION 0.1.0
  DESCRIPTION "Hardware-adaptive low-latency Haxball App for Windows"
  LANGUAGES CXX)

option(HAX_BUILD_APP "Build the Windows CEF application" OFF)
option(HAX_BUILD_TESTS "Build unit tests" ON)
option(HAX_BUILD_BENCHMARKS "Build micro-benchmarks" ON)
option(HAX_DOWNLOAD_CEF "Download the pinned CEF distribution when building the app" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  # Official CEF binary distributions and libcef_dll_wrapper use the static
  # MSVC runtime by default. Keep every project target on the same ABI.
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)

add_library(hax_core STATIC
  src/core/candidates.cpp
  src/core/optimizer.cpp
  src/core/statistics.cpp
)
target_include_directories(hax_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
hax_set_warnings(hax_core)

if(WIN32)
  add_library(hax_platform STATIC
    src/platform/windows/runtime_policy_win.cpp
    src/platform/windows/system_probe_win.cpp
  )
  target_include_directories(hax_platform PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_link_libraries(hax_platform PUBLIC hax_core dxgi)
  target_compile_definitions(hax_platform PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
  hax_set_warnings(hax_platform)
endif()

if(WIN32)
  add_executable(hax_system_probe tools/probe_system.cpp)
  target_link_libraries(hax_system_probe PRIVATE hax_platform)
  hax_set_warnings(hax_system_probe)

  add_executable(hax_input_pulse tools/input_pulse_win.cpp)
  target_link_libraries(hax_input_pulse PRIVATE user32)
  target_compile_definitions(
    hax_input_pulse PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
  hax_set_warnings(hax_input_pulse)
endif()

if(HAX_BUILD_TESTS)
  enable_testing()
  add_executable(hax_core_tests tests/core_tests.cpp)
  target_link_libraries(hax_core_tests PRIVATE hax_core)
  hax_set_warnings(hax_core_tests)
  add_test(NAME hax_core_tests COMMAND hax_core_tests)
endif()

if(HAX_BUILD_BENCHMARKS)
  add_executable(hax_optimizer_bench tools/benchmark_optimizer.cpp)
  target_link_libraries(hax_optimizer_bench PRIVATE hax_core)
  hax_set_warnings(hax_optimizer_bench)

  add_executable(hax_capture_analyzer tools/analyze_presentmon.cpp)
  target_link_libraries(hax_capture_analyzer PRIVATE hax_core)
  hax_set_warnings(hax_capture_analyzer)
endif()

if(HAX_BUILD_APP)
  if(NOT WIN32)
    message(FATAL_ERROR "HAX_BUILD_APP is Windows-only.")
  endif()

  if(HAX_DOWNLOAD_CEF)
    include(DownloadCEF)
    hax_download_cef("${CMAKE_CURRENT_SOURCE_DIR}/third_party/cef")
  elseif(NOT CEF_ROOT)
    message(FATAL_ERROR "Set CEF_ROOT or enable HAX_DOWNLOAD_CEF.")
  endif()

  set(USE_SANDBOX OFF CACHE BOOL "CEF sandbox mode" FORCE)
  list(APPEND CMAKE_MODULE_PATH "${CEF_ROOT}/cmake")
  find_package(CEF REQUIRED)

  add_subdirectory("${CEF_LIBCEF_DLL_WRAPPER_PATH}" libcef_dll_wrapper)
  ADD_LOGICAL_TARGET("libcef_lib" "${CEF_LIB_RELEASE}" "${CEF_LIB_DEBUG}")

  set(HAX_SOURCE_IMAGE
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/windows/haxball_app_source.png")
  set(HAX_GENERATED_ICON
    "${CMAKE_CURRENT_BINARY_DIR}/haxball_app_rc.ico")

  execute_process(
    COMMAND powershell.exe
      -NoProfile
      -ExecutionPolicy Bypass
      -File "${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate-icon.ps1"
      -SourceImage "${HAX_SOURCE_IMAGE}"
      -OutputIcon "${HAX_GENERATED_ICON}"
    RESULT_VARIABLE HAX_ICON_RESULT
  )

  if(NOT HAX_ICON_RESULT EQUAL 0 OR NOT EXISTS "${HAX_GENERATED_ICON}")
    message(FATAL_ERROR "Failed to generate RC-compatible Haxball App icon")
  endif()

  file(TO_CMAKE_PATH "${HAX_GENERATED_ICON}" HAX_APP_ICON_PATH)
  set(HAX_APP_MANIFEST_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/windows/haxball_app.manifest")
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/windows/haxball_app.rc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/haxball_app.rc"
    @ONLY)

  add_executable(hax WIN32
    src/app/app.cpp
    src/app/client.cpp
    src/app/launch_options.cpp
    src/app/main_win.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/haxball_app.rc"
  )
  target_include_directories(hax PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CEF_INCLUDE_PATH}"
  )
  target_link_libraries(hax PRIVATE
    hax_core
    hax_platform
    libcef_dll_wrapper
    libcef_lib
    ${CEF_STANDARD_LIBS}
  )
  target_compile_definitions(hax PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
  )
  SET_EXECUTABLE_TARGET_PROPERTIES(hax)
  set_target_properties(hax PROPERTIES OUTPUT_NAME "Haxball App")
  hax_set_warnings(hax)

  set(HAX_OUT_DIR "$<TARGET_FILE_DIR:hax>")
  foreach(_cef_binary ${CEF_BINARY_FILES})
    add_custom_command(TARGET hax POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CEF_BINARY_DIR}/${_cef_binary}"
        "${HAX_OUT_DIR}/${_cef_binary}"
      VERBATIM)
  endforeach()
  foreach(_cef_resource ${CEF_RESOURCE_FILES})
    if(IS_DIRECTORY "${CEF_RESOURCE_DIR}/${_cef_resource}")
      add_custom_command(TARGET hax POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CEF_RESOURCE_DIR}/${_cef_resource}"
          "${HAX_OUT_DIR}/${_cef_resource}"
        VERBATIM)
    else()
      add_custom_command(TARGET hax POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${CEF_RESOURCE_DIR}/${_cef_resource}"
          "${HAX_OUT_DIR}/${_cef_resource}"
        VERBATIM)
    endif()
  endforeach()
  add_custom_command(TARGET hax POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${CMAKE_CURRENT_SOURCE_DIR}/assets/benchmark.html"
      "${HAX_OUT_DIR}/benchmark.html"
    VERBATIM)
endif()
