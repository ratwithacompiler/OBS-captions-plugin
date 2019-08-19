cmake_minimum_required(VERSION 3.14)
project(obs_google_caption_plugin)


#message("prefix: ${CMAKE_FIND_LIBRARY_PREFIXES}")
#message("suffix: ${CMAKE_FIND_LIBRARY_SUFFIXES}")

set(CMAKE_CXX_STANDARD 14)

set(DEVMODE OFF)
if (USE_DEVMODE)
    message(WARNING "building in DEVMODE mode")
#    set(DEVMODE ON)
endif ()

if (DEVMODE)
    add_compile_definitions(GOOGLE="127.0.0.1")
    add_compile_definitions(PORTUP=9125)
    add_compile_definitions(PORTDOWN=9125)
    add_compile_definitions(USE_DEVMODE=1)
    message("using api: localhost")
else ()
    add_compile_definitions(GOOGLE="www.google.com")
    add_compile_definitions(PORTUP=80)
    add_compile_definitions(PORTDOWN=80)
    message("using api: google")
endif ()

set(GOOGLE_API_KEY "" CACHE STRING "google api key")
if (NOT GOOGLE_API_KEY)
    message(FATAL_ERROR "no google api key set")
endif ()
add_compile_definitions(GOOGLE_API_KEY_STR="${GOOGLE_API_KEY}")

set(OBS_SOURCE_DIR "" CACHE STRING "Path to root obs source dir containing libobs/ and UI/ folders")
set(OBS_LIB_BIN_DIR "" CACHE STRING "Path containing obs and obs-fronted-api libs")

message("hmm ${OBS_SOURCE_DIR}")
message("hmm ${OBS_LIB_DIR}")

if (DEFINED QT_DIR)
    list(APPEND CMAKE_PREFIX_PATH "${QT_DIR}")
    message("added ${QT_DIR} to prefix path")
endif ()


if (NOT OBS_SOURCE_DIR)
    message(FATAL_ERROR "OBS_SOURCE_DIR not set")
endif ()

if (NOT OBS_LIB_DIR)
    message(FATAL_ERROR "OBS_LIB_DIR not set")
endif ()

add_subdirectory(lib)

add_compile_definitions(BUILD_CAPTIONS=1)
include_directories(${OBS_SOURCE_DIR}/libobs)
include_directories(${OBS_SOURCE_DIR}/UI/obs-frontend-api/)

set(obs_google_caption_plugin_SOURCES
        src/ui/MainCaptionWidget.cpp
        src/ui/CaptionSettingsWidget.cpp

        src/AudioCaptureSession.cpp
        src/SourceCaptioner.cpp
        src/CaptionResultHandler.cpp

        src/google_s2t_caption_plugin.cpp
        )

set(obs_google_caption_plugin_HEADERS
        src/AudioCaptureSession.h
        src/SourceCaptioner.h
        src/CaptionResultHandler.cpp

        src/ui/MainCaptionWidget.h
        src/ui/CaptionSettingsWidget.h
        )

set(obs_google_caption_plugin_UIS
        src/ui/MainCaptionWidget.ui
        src/ui/CaptionSettingsWidget.ui
        )

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets Core)


add_library(obs_google_caption_plugin MODULE
        ${obs_google_caption_plugin_UIS}
        ${obs_google_caption_plugin_HEADERS}
        ${obs_google_caption_plugin_SOURCES}
        )

#message("prefix: ${CMAKE_FIND_LIBRARY_PREFIXES}")
#message("suffix: ${CMAKE_FIND_LIBRARY_SUFFIXES}")


find_library(LIBOBS_LIBRARY obs
        ${OBS_LIB_DIR}
        ${OBS_LIB_DIR}/libobs/
        ${OBS_LIB_DIR}/libobs/RelWithDebInfo/
        ${OBS_LIB_DIR}/libobs/*/
        )
message("LIBOBS_LIBRARY: ${LIBOBS_LIBRARY}")

find_library(OBS_FRONTEND_LIBRARY obs-frontend-api
        ${OBS_LIB_DIR}
        ${OBS_LIB_DIR}/UI/obs-frontend-api/
        ${OBS_LIB_DIR}/UI/obs-frontend-api/RelWithDebInfo/
        ${OBS_LIB_DIR}/UI/obs-frontend-api/*/
        )
message("OBS_FRONTEND_LIBRARY: ${OBS_FRONTEND_LIBRARY}")


target_link_libraries(obs_google_caption_plugin
        caption_stream_static
        ${OBS_FRONTEND_LIBRARY}
        ${LIBOBS_LIBRARY}

        Qt5::Widgets
        )
