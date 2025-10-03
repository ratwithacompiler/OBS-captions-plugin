cmake_minimum_required(VERSION 3.12)
project(obs_google_caption_plugin)

set(CMAKE_CXX_STANDARD 17)
set(VERSION_STRING "0.32")

set(OBS_BUILD_DIR "" CACHE STRING "Path containing obs and obs-fronted-api libs")
set(OBS_DEPS_DIR "" CACHE STRING "Path to OBS deps and Qt deps")

message("OBS_BUILD_DIR: ${OBS_BUILD_DIR}")
message("OBS_DEPS_DIR: ${OBS_DEPS_DIR}")

IF (WIN32)
    set(CMAKE_PREFIX_PATH "${OBS_BUILD_DIR};${OBS_DEPS_DIR};${CMAKE_PREFIX_PATH}")
else ()
    # add UI to be able to build from non installed build dir directly during dev
    set(CMAKE_PREFIX_PATH "${OBS_BUILD_DIR};${OBS_BUILD_DIR}/UI;${OBS_DEPS_DIR};${CMAKE_PREFIX_PATH}")
endif ()

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

if (NOT OBS_BUILD_DIR)
    message(FATAL_ERROR "OBS_BUILD_DIR not set")
endif ()

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

find_package(Qt6 COMPONENTS Widgets Core)
IF (Qt6_FOUND)
    message("using Qt6: ${Qt6_DIR}")
    qt_add_resources(obs_QRC_SOURCES ${obs_QRC})
    set(_qt_widgets Qt6::Widgets)
elseif ()
    find_package(Qt5 REQUIRED COMPONENTS Widgets Core)
    message("using Qt5: ${Qt5_DIR}")
    qt5_add_resources(obs_QRC_SOURCES ${obs_QRC})
    set(_qt_widgets Qt5::Widgets)
ENDIF ()

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

install(TARGETS obs_google_caption_plugin DESTINATION lib)

#message("CMAKE_FRAMEWORK_PATH: ${CMAKE_FRAMEWORK_PATH}")
#set(CMAKE_FRAMEWORK_PATH "${OBS_BUILD_DIR}/Frameworks")
set(CMAKE_FRAMEWORK_PATH "${CMAKE_FRAMEWORK_PATH};${OBS_BUILD_DIR}/Frameworks")
message("CMAKE_FRAMEWORK_PATH: ${CMAKE_FRAMEWORK_PATH}")
message("CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
message("OBS_BUILD_DIR: ${OBS_BUILD_DIR}")

find_package(libobs REQUIRED
        PATHS
        ${OBS_BUILD_DIR}/Frameworks
        ${OBS_BUILD_DIR}/lib
        ${OBS_BUILD_DIR}
        NO_DEFAULT_PATH
        )
message("using libOBS: ${libobs_DIR}")

find_package(obs-frontend-api REQUIRED
        CONFIG
        PATHS
        ${OBS_BUILD_DIR}
        ${OBS_BUILD_DIR}/lib
        ${OBS_BUILD_DIR}/UI/obs-frontend-api
        NO_DEFAULT_PATH
        )
message("using obs_frontend_library: ${obs-frontend-api_DIR}")

if (APPLE)
    target_include_directories(obs_google_caption_plugin PRIVATE "${OBS_BUILD_DIR}/Frameworks/libobs.framework/Headers")
endif ()

target_link_libraries(obs_google_caption_plugin PRIVATE caption_stream ${_qt_widgets} OBS::libobs OBS::obs-frontend-api)


#get_property(dirs TARGET obs_google_caption_plugin PROPERTY INCLUDE_DIRECTORIES)
#foreach (dir ${dirs})
#    message("dir='${dir}'")
#endforeach ()
#get_property(dirs2 TARGET obs_google_caption_plugin PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
#foreach (dir ${dirs2})
#    message("dir2='${dir}'")
#endforeach ()
