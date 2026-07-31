# Pinned prebuilt sherpa-onnx runtime for the Windows-only local caption engine.

include_guard(GLOBAL)

if(NOT WIN32)
  message(FATAL_ERROR "The local sherpa-onnx caption engine is currently supported on Windows only.")
endif()

set(SHERPA_ONNX_VERSION "1.13.4")
set(SHERPA_ONNX_ARCHIVE "sherpa-onnx-v${SHERPA_ONNX_VERSION}-win-x64-shared-MT-Release-no-tts.tar.bz2")
set(SHERPA_ONNX_ARCHIVE_SHA256 "e33dc64195d17601879532583233d0d6ed76aa399eb863e5ca0783c5ac82b5aa")
set(SHERPA_ONNX_URL "https://github.com/k2-fsa/sherpa-onnx/releases/download/v${SHERPA_ONNX_VERSION}/${SHERPA_ONNX_ARCHIVE}")

set(_sherpa_download_dir "${CMAKE_BINARY_DIR}/_deps/sherpa-onnx")
set(_sherpa_archive "${_sherpa_download_dir}/${SHERPA_ONNX_ARCHIVE}")
set(SHERPA_ONNX_ROOT "${_sherpa_download_dir}/sherpa-onnx-v${SHERPA_ONNX_VERSION}-win-x64-shared-MT-Release-no-tts")

file(MAKE_DIRECTORY "${_sherpa_download_dir}")

if(EXISTS "${_sherpa_archive}")
  file(SHA256 "${_sherpa_archive}" _sherpa_actual_sha256)
  if(NOT "${_sherpa_actual_sha256}" STREQUAL "${SHERPA_ONNX_ARCHIVE_SHA256}")
    message(WARNING "Discarding sherpa-onnx archive with an unexpected SHA-256.")
    file(REMOVE "${_sherpa_archive}")
  endif()
endif()

if(NOT EXISTS "${_sherpa_archive}")
  message(STATUS "Downloading pinned sherpa-onnx runtime ${SHERPA_ONNX_VERSION}")
  file(
    DOWNLOAD "${SHERPA_ONNX_URL}" "${_sherpa_archive}"
    EXPECTED_HASH "SHA256=${SHERPA_ONNX_ARCHIVE_SHA256}"
    STATUS _sherpa_download_status
    SHOW_PROGRESS
  )
  list(GET _sherpa_download_status 0 _sherpa_download_code)
  list(GET _sherpa_download_status 1 _sherpa_download_message)
  if(_sherpa_download_code GREATER 0)
    file(REMOVE "${_sherpa_archive}")
    message(FATAL_ERROR "Unable to download sherpa-onnx: ${_sherpa_download_message}")
  endif()
endif()

if(NOT EXISTS "${SHERPA_ONNX_ROOT}/lib/sherpa-onnx-c-api.lib")
  message(STATUS "Extracting sherpa-onnx runtime ${SHERPA_ONNX_VERSION}")
  file(ARCHIVE_EXTRACT INPUT "${_sherpa_archive}" DESTINATION "${_sherpa_download_dir}")
endif()

if(NOT EXISTS "${SHERPA_ONNX_ROOT}/lib/sherpa-onnx-c-api.lib")
  message(FATAL_ERROR "The sherpa-onnx archive did not contain the expected C API import library.")
endif()

add_library(SherpaOnnx::CAPI SHARED IMPORTED GLOBAL)
set_target_properties(
  SherpaOnnx::CAPI
  PROPERTIES
    IMPORTED_IMPLIB "${SHERPA_ONNX_ROOT}/lib/sherpa-onnx-c-api.lib"
    IMPORTED_LOCATION "${SHERPA_ONNX_ROOT}/lib/sherpa-onnx-c-api.dll"
    INTERFACE_INCLUDE_DIRECTORIES "${SHERPA_ONNX_ROOT}/include"
)

set(
  SHERPA_ONNX_RUNTIME_FILES
  "${SHERPA_ONNX_ROOT}/lib/sherpa-onnx-c-api.dll"
  "${SHERPA_ONNX_ROOT}/lib/onnxruntime.dll"
  "${SHERPA_ONNX_ROOT}/lib/onnxruntime_providers_shared.dll"
)

function(sherpa_onnx_copy_runtime target)
  foreach(runtime_file IN LISTS SHERPA_ONNX_RUNTIME_FILES)
    add_custom_command(
      TARGET ${target}
      POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${runtime_file}" "$<TARGET_FILE_DIR:${target}>"
      VERBATIM
    )
  endforeach()
endfunction()

set(SherpaOnnx_FOUND TRUE)
