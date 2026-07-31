cmake_minimum_required(VERSION 3.28...4.3)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version} LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" ON)
option(ENABLE_QT "Use Qt functionality" ON)
option(ENABLE_CUSTOM_API_KEY "Allow the Google API key to be entered in the settings UI" OFF)
set(GOOGLE_API_KEY "" CACHE STRING "Optional Google Speech API key compiled into the plugin")

if(GOOGLE_API_KEY AND ENABLE_CUSTOM_API_KEY)
  message(FATAL_ERROR "GOOGLE_API_KEY and ENABLE_CUSTOM_API_KEY cannot both be enabled")
endif()

include(compilerconfig)
include(defaults)
include(helpers)
include(FindSherpaOnnx)

find_package(libobs REQUIRED)
find_package(obs-frontend-api REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_subdirectory(lib/caption_stream)

add_library(${CMAKE_PROJECT_NAME} MODULE)

target_sources(
  ${CMAKE_PROJECT_NAME}
  PRIVATE
    src/ui/MainCaptionWidget.cpp
    src/ui/CaptionSettingsWidget.cpp
    src/SourceAudioCaptureSession.cpp
    src/OutputAudioCaptureSession.cpp
    src/SourceCaptioner.cpp
    src/CaptionResultHandler.cpp
    src/google_s2t_caption_plugin.cpp
    src/CaptionPluginManager.cpp
    src/ui/CaptionDock.cpp
    src/ui/OpenCaptionSettingsWidget.cpp
    src/ui/MainCaptionWidget.h
    src/ui/CaptionSettingsWidget.h
    src/SourceAudioCaptureSession.h
    src/OutputAudioCaptureSession.h
    src/SourceCaptioner.h
    src/CaptionResultHandler.h
    src/caption_output_writer.h
    src/CaptionPluginManager.h
    src/ui/CaptionDock.h
    src/ui/OpenCaptionSettingsWidget.h
    src/resources/obs.qrc
)

target_compile_definitions(
  ${CMAKE_PROJECT_NAME}
  PRIVATE BUILD_CAPTIONS=1 VERSION_STRING="${PROJECT_VERSION}"
)

if(ENABLE_CUSTOM_API_KEY)
  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE ENABLE_CUSTOM_API_KEY=1)
elseif(GOOGLE_API_KEY)
  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE GOOGLE_API_KEY_STR="${GOOGLE_API_KEY}")
endif()

target_link_libraries(
  ${CMAKE_PROJECT_NAME}
  PRIVATE caption_stream Qt6::Core Qt6::Widgets OBS::libobs OBS::obs-frontend-api
)

set_target_properties(
  ${CMAKE_PROJECT_NAME}
  PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON OUTPUT_NAME ai-caption-plugin
)

set_target_properties_plugin(${CMAKE_PROJECT_NAME})
install(FILES ${SHERPA_ONNX_RUNTIME_FILES} DESTINATION "${CMAKE_PROJECT_NAME}/bin/64bit")
install(
  FILES
    installer/Install-AICaptionPlugin.ps1
    installer/local-model.json
  DESTINATION "${CMAKE_PROJECT_NAME}"
)
sherpa_onnx_copy_runtime(${CMAKE_PROJECT_NAME})

include(CTest)
if(BUILD_TESTING)
  add_executable(caption-engine-contract-test tests/caption_engine_contract_test.cpp)
  target_include_directories(caption-engine-contract-test PRIVATE lib/caption_stream)
  target_link_libraries(caption-engine-contract-test PRIVATE caption_stream)
  sherpa_onnx_copy_runtime(caption-engine-contract-test)
  add_test(NAME caption-engine-contract COMMAND caption-engine-contract-test)
endif()
