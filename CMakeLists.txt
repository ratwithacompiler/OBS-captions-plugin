cmake_minimum_required(VERSION 3.12)
project(obs_google_caption_plugin)


#message("prefix: ${CMAKE_FIND_LIBRARY_PREFIXES}")
#message("suffix: ${CMAKE_FIND_LIBRARY_SUFFIXES}")

set(CMAKE_CXX_STANDARD 14)
set(VERSION_STRING "0.20b")

message("BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS}")

set(DEVMODE OFF)
if (USE_DEVMODE)
    #    message(WARNING "building in DEVMODE mode")
    set(DEVMODE ON)
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
    message(FATAL_ERROR "no GOOGLE_API_KEY set and no ENABLE_CUSTOM_API_KEY=ON either")
endif ()

add_compile_definitions(VERSION_STRING="${VERSION_STRING}")

set(OBS_SOURCE_DIR "" CACHE STRING "Path to root obs source dir containing libobs/ and UI/ folders")
set(OBS_LIB_BIN_DIR "" CACHE STRING "Path containing obs and obs-fronted-api libs")
set(QT_DEP_DIR "" CACHE STRING "Path to Qt dir")

message("OBS_SOURCE_DIR: ${OBS_SOURCE_DIR}")
message("OBS_LIB_DIR: ${OBS_LIB_DIR}")
message("QT_DEP_DIR: ${QT_DEP_DIR}")

if (NOT OBS_SOURCE_DIR)
    message(FATAL_ERROR "OBS_SOURCE_DIR not set")
endif ()

if (NOT OBS_LIB_DIR)
    message(FATAL_ERROR "OBS_LIB_DIR not set")
endif ()

configure_file(CI/http/win-install-script.cmd CI/http/win-install-script.cmd)
configure_file(CI/http/osx-install-script.sh CI/http/osx-install-script.sh)
configure_file(CI/http/linux-install-script.sh CI/http/linux-install-script.sh)

configure_file(CI/grpc/win-install-script.cmd CI/grpc/win-install-script.cmd)
configure_file(CI/grpc/osx-install-script.sh CI/grpc/osx-install-script.sh)
configure_file(CI/grpc/linux-install-script.sh CI/grpc/linux-install-script.sh)

add_subdirectory(lib/caption_stream)

if (DEVMODE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wno-deprecated-declarations")
    message("CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}")

    target_compile_definitions(speech_api_stream PRIVATE GOOGLE="www.google.com" PORTUP=80 PORTDOWN=80)
    #    target_compile_definitions(speech_api_stream PRIVATE GOOGLE="127.0.0.1" PORTUP=9125 PORTDOWN=9125)
    message("using api: localhost")

    add_compile_definitions(USE_DEVMODE=1)
else ()
    target_compile_definitions(speech_api_stream PRIVATE GOOGLE="www.google.com" PORTUP=80 PORTDOWN=80)
    message("using api: google")
endif ()

add_compile_definitions(BUILD_CAPTIONS=1)
include_directories(${OBS_SOURCE_DIR}/libobs)
include_directories(${OBS_SOURCE_DIR}/UI/obs-frontend-api/)

set(obs_google_caption_plugin_SOURCES
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
        )

set(obs_google_caption_plugin_HEADERS
        src/SourceAudioCaptureSession.h
        src/OutputAudioCaptureSession.h
        src/SourceCaptioner.h
        src/CaptionResultHandler.cpp

        src/ui/MainCaptionWidget.h
        src/ui/CaptionSettingsWidget.h
        src/caption_output_writer.h
        src/CaptionPluginManager.h
        src/ui/CaptionDock.h
        src/ui/OpenCaptionSettingsWidget.h
        )

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(obs_QRC src/resources/obs.qrc)


if (DEFINED QT_DEP_DIR AND NOT "${QT_DEP_DIR}" STREQUAL "")
    FILE(TO_CMAKE_PATH "${QT_DEP_DIR}" QT_DEP_DIR_CMAKE)
    message("USING QT_DEP_DIR: ${QT_DEP_DIR_CMAKE}")

    #    set(Qt6_DIR ${QT_DEP_DIR_CMAKE})
    #    set(Qt6WidgetsTools_DIR ${QT_DEP_DIR_CMAKE})

    message("CMAKE_PREFIX_PATH before: ${CMAKE_PREFIX_PATH}")
    set(CMAKE_PREFIX_PATH_ORG "${CMAKE_PREFIX_PATH}")
    set(CMAKE_PREFIX_PATH "${QT_DEP_DIR_CMAKE}")
    #    find_qt(COMPONENTS Widgets Core)

    find_package(Qt6
            COMPONENTS Widgets Core
            PATHS ${QT_DEP_DIR_CMAKE}

            NO_DEFAULT_PATH
            NO_CMAKE_PATH
            )

    set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH_ORG}")
    message("CMAKE_PREFIX_PATH after: ${CMAKE_PREFIX_PATH}")

    message(" huhhh Qt_DIR: ${Qt_DIR} ;Qt6_DIR: ${Qt6_DIR} ${Qt6_FOUND}")
    IF (Qt6_FOUND)
        message("found qt6: ${Qt_DIR} ; ${Qt6_DIR}")
        qt_add_resources(obs_QRC_SOURCES ${obs_QRC})
    else ()
        find_package(Qt5 REQUIRED COMPONENTS Widgets Core
                PATHS ${QT_DEP_DIR_CMAKE}
                NO_DEFAULT_PATH
                )

        message("found qt5: ${Qt_DIR} ; ${Qt5_DIR}")
        qt5_add_resources(obs_QRC_SOURCES ${obs_QRC})
    ENDIF ()

else ()
    find_package(Qt6 REQUIRED COMPONENTS Widgets Core)
    IF (Qt6_FOUND)
        message("found qt6: ${Qt_DIR} ; ${Qt6_DIR}")
        qt_add_resources(obs_QRC_SOURCES ${obs_QRC})
    elseif ()
        find_package(Qt5 REQUIRED COMPONENTS Widgets Core)
        message("found qt5: ${Qt_DIR} ; ${Qt5_DIR}")
        qt5_add_resources(obs_QRC_SOURCES ${obs_QRC})
    ENDIF ()

endif ()

IF (Qt6_FOUND)
    set(_qt_widgets Qt6::Widgets)
else ()
    set(_qt_widgets Qt5::Widgets)
endif ()

#configure_file(src/ui/CaptionSettingsWidget.ui src/ui/CaptionSettingsWidget_out.ui)
#set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_BINARY_DIR})


add_library(obs_google_caption_plugin MODULE
        ${obs_google_caption_plugin_HEADERS}
        ${obs_google_caption_plugin_SOURCES}
        ${obs_QRC_SOURCES}
        )


if (WIN32 AND NOT BUILD_SHARED_LIBS)
    message(BUILD_SHARED_LIBS: setting win flags)
    #        set_property(TARGET obs_google_caption_plugin PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
    foreach (flag_var
            CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)

        if (${flag_var} MATCHES "/MD")
            message("before: ${flag_var}: " ${${flag_var}}" ")
            string(REGEX REPLACE " /MD" "/MT" ${flag_var} "${${flag_var}}")
            message("after : ${flag_var}: " ${${flag_var}}" ")
        endif ()
    endforeach ()
endif ()

if (PLUGIN_COMPILE_DEFINITIONS)
    #    message(" PLUGIN_COMPILE_DEFINITIONS ${PLUGIN_COMPILE_DEFINITIONS}")
    target_compile_definitions(obs_google_caption_plugin PRIVATE ${PLUGIN_COMPILE_DEFINITIONS})
endif ()

#message("suffix: ${CMAKE_FIND_LIBRARY_SUFFIXES}")

find_package(libobs REQUIRED
        CONFIG
        PATHS
        ${OBS_LIB_DIR}
        ${OBS_LIB_DIR}/libobs/
        ${OBS_LIB_DIR}/libobs/RelWithDebInfo/
        ${OBS_LIB_DIR}/libobs/Relase/
        ${OBS_LIB_DIR}/libobs/*/
        NO_DEFAULT_PATH
        )

message("using libOBS: ${LIBOBS_LIBRARY}")
if (NOT libobs_FOUND)
    message(FATAL_ERROR "libobs not found: ${OBS_LIB_DIR}")
endif ()
message("using libOBS: ${libobs_DIR}")

find_package(obs-frontend-api REQUIRED
        CONFIG
        PATHS
        ${OBS_LIB_DIR}
        ${OBS_LIB_DIR}/UI/obs-frontend-api/
        ${OBS_LIB_DIR}/UI/obs-frontend-api/RelWithDebInfo/
        ${OBS_LIB_DIR}/UI/obs-frontend-api/Release/
        ${OBS_LIB_DIR}/UI/obs-frontend-api/*/
        NO_DEFAULT_PATH
        )

message("using obs_frontend_library: ${obs-frontend-api_DIR}")
if (NOT obs-frontend-api_FOUND)
    message(FATAL_ERROR "obs_frontend_library not found: ${OBS_LIB_DIR}")
endif ()
message("using obs_frontend_library: ${obs-frontend-api_DIR} ")


target_link_libraries(obs_google_caption_plugin
        caption_stream
        OBS::libobs OBS::obs-frontend-api

        ${_qt_widgets}
        )
