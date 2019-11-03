cmake_minimum_required(VERSION 3.12)
project(obs_google_caption_plugin)


#message("prefix: ${CMAKE_FIND_LIBRARY_PREFIXES}")
#message("suffix: ${CMAKE_FIND_LIBRARY_SUFFIXES}")

set(CMAKE_CXX_STANDARD 14)
set(VERSION_STRING "0.0.11")


set(DEVMODE OFF)
if (USE_DEVMODE)
    message(WARNING "building in DEVMODE mode")
    set(DEVMODE ON)
endif ()

if (DEVMODE)
    add_compile_definitions(GOOGLE="www.google.com")
    add_compile_definitions(PORTUP=80)
    add_compile_definitions(PORTDOWN=80)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
    message("CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}")

    #    add_compile_definitions(GOOGLE="127.0.0.1")
    #    add_compile_definitions(PORTUP=9125)
    #    add_compile_definitions(PORTDOWN=9125)

    add_compile_definitions(USE_DEVMODE=1)
    message("using api: localhost")
else ()
    add_compile_definitions(GOOGLE="www.google.com")
    add_compile_definitions(PORTUP=80)
    add_compile_definitions(PORTDOWN=80)
    message("using api: google")
endif ()

set(ENABLE_CUSTOM_API_KEY OFF CACHE BOOL "set API key via settings UI field")
set(GOOGLE_API_KEY "" CACHE STRING "google api key with speech API enabled")

if (GOOGLE_API_KEY AND ENABLE_CUSTOM_API_KEY)
    message(FATAL_ERROR "both GOOGLE_API_KEY and ENABLE_CUSTOM_API_KEY set, not supported")
endif ()

if (ENABLE_CUSTOM_API_KEY)
    message("ENABLE_CUSTOM_API_KEY on, using UI api key")
    set(PLUGIN_COMPILE_DEFINITIONS ENABLE_CUSTOM_API_KEY=1)
elseif (GOOGLE_API_KEY)
    message("using fixed GOOGLE_API_KEY")
    set(PLUGIN_COMPILE_DEFINITIONS GOOGLE_API_KEY_STR="${GOOGLE_API_KEY}")
else ()
    message(FATAL_ERROR "no GOOGLE_API_KEY set")
endif ()

add_compile_definitions(VERSION_STRING="${VERSION_STRING}")

set(OBS_SOURCE_DIR "" CACHE STRING "Path to root obs source dir containing libobs/ and UI/ folders")
set(OBS_LIB_BIN_DIR "" CACHE STRING "Path containing obs and obs-fronted-api libs")
set(QT_DIR "" CACHE STRING "Path to Qt dir")

message("hmm ${OBS_SOURCE_DIR}")
message("hmm ${OBS_LIB_DIR}")
message("hmm ${QT_DIR}")

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

configure_file(CI/post-install-script-win.cmd CI/post-install-script-win.cmd)

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
        src/CaptionPluginManager.cpp
        src/ui/CaptionDock.cpp
        )

set(obs_google_caption_plugin_HEADERS
        src/AudioCaptureSession.h
        src/SourceCaptioner.h
        src/CaptionResultHandler.cpp

        src/ui/MainCaptionWidget.h
        src/ui/CaptionSettingsWidget.h
        src/caption_output_writer.h
        src/CaptionPluginManager.h
        src/ui/CaptionDock.h
        )

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)


find_package(Qt5 REQUIRED COMPONENTS Widgets Core)

configure_file(src/ui/CaptionSettingsWidget.ui src/ui/CaptionSettingsWidget_out.ui)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_BINARY_DIR})


set(obs_QRC src/resources/obs.qrc)
qt5_add_resources(obs_QRC_SOURCES ${obs_QRC})

add_library(obs_google_caption_plugin MODULE
        ${obs_google_caption_plugin_HEADERS}
        ${obs_google_caption_plugin_SOURCES}
        ${obs_QRC_SOURCES}
        )

if (PLUGIN_COMPILE_DEFINITIONS)
    #    message("PLUGIN_COMPILE_DEFINITIONS ${PLUGIN_COMPILE_DEFINITIONS}")
    target_compile_definitions(obs_google_caption_plugin PRIVATE ${PLUGIN_COMPILE_DEFINITIONS})
endif ()

#message("suffix: ${CMAKE_FIND_LIBRARY_SUFFIXES}")

find_library(LIBOBS_LIBRARY obs
        ${OBS_LIB_DIR}
        ${OBS_LIB_DIR}/libobs/
        ${OBS_LIB_DIR}/libobs/RelWithDebInfo/
        ${OBS_LIB_DIR}/libobs/Relase/
        ${OBS_LIB_DIR}/libobs/*/
        )

if (NOT LIBOBS_LIBRARY)
    message(FATAL_ERROR "libOBS not found: ${OBS_LIB_DIR}")
endif ()
message("using libOBS: ${LIBOBS_LIBRARY}")

find_library(OBS_FRONTEND_LIBRARY obs-frontend-api
        ${OBS_LIB_DIR}
        ${OBS_LIB_DIR}/UI/obs-frontend-api/
        ${OBS_LIB_DIR}/UI/obs-frontend-api/RelWithDebInfo/
        ${OBS_LIB_DIR}/UI/obs-frontend-api/Release/
        ${OBS_LIB_DIR}/UI/obs-frontend-api/*/
        )

if (NOT OBS_FRONTEND_LIBRARY)
    message(FATAL_ERROR "obs_frontend_library not found: ${OBS_LIB_DIR}")
endif ()
message("using obs_frontend_library: ${OBS_FRONTEND_LIBRARY}")

if (NOT OBS_FRONTEND_LIBRARY)
    message(FATAL_ERROR "obs_frontend_library not found")
endif ()

target_link_libraries(obs_google_caption_plugin
        caption_stream
        ${OBS_FRONTEND_LIBRARY}
        ${LIBOBS_LIBRARY}

        Qt5::Widgets
        )
