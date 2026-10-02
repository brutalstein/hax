set(HAX_CEF_VERSION "154.0.28+g564dd6c+chromium-154.0.8037.58" CACHE STRING "Pinned CEF version")
set(HAX_CEF_PLATFORM "windows64" CACHE STRING "CEF platform")

function(hax_download_cef download_dir)
  set(_distribution "cef_binary_${HAX_CEF_VERSION}_${HAX_CEF_PLATFORM}")
  set(_archive "${download_dir}/${_distribution}.tar.bz2")
  set(_root "${download_dir}/${_distribution}")

  if(NOT IS_DIRECTORY "${_root}")
    file(MAKE_DIRECTORY "${download_dir}")
    string(REPLACE "+" "%2B" _url
      "https://cef-builds.spotifycdn.com/${_distribution}.tar.bz2")

    if(NOT EXISTS "${_archive}")
      message(STATUS "Downloading CEF ${HAX_CEF_VERSION}...")
      file(DOWNLOAD "${_url}.sha1" "${_archive}.sha1"
        STATUS _sha_status TLS_VERIFY ON)
      list(GET _sha_status 0 _sha_code)
      if(NOT _sha_code EQUAL 0)
        message(FATAL_ERROR "Failed to download CEF SHA1: ${_sha_status}")
      endif()

      file(READ "${_archive}.sha1" _sha1)
      string(STRIP "${_sha1}" _sha1)

      file(DOWNLOAD "${_url}" "${_archive}"
        EXPECTED_HASH "SHA1=${_sha1}"
        SHOW_PROGRESS
        STATUS _download_status
        TLS_VERIFY ON)
      list(GET _download_status 0 _download_code)
      if(NOT _download_code EQUAL 0)
        message(FATAL_ERROR "Failed to download CEF: ${_download_status}")
      endif()
    endif()

    message(STATUS "Extracting CEF...")
    execute_process(
      COMMAND ${CMAKE_COMMAND} -E tar xzf "${_archive}"
      WORKING_DIRECTORY "${download_dir}"
      RESULT_VARIABLE _extract_result
    )
    if(NOT _extract_result EQUAL 0)
      message(FATAL_ERROR "CEF extraction failed with code ${_extract_result}")
    endif()
  endif()

  set(CEF_ROOT "${_root}" CACHE PATH "CEF root" FORCE)
  set(CEF_ROOT "${_root}" PARENT_SCOPE)
endfunction()
