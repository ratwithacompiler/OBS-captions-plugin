project(caption_stream)

set(BUILD_SHARED_LIB "" CACHE STRING "build as shared lib")
set(BUILD_SHARED_LIB OFF)

message("BUILD_SHARED_LIB: ${BUILD_SHARED_LIB}")

if (SPEECH_API_GOOGLE_GRPC_V1 AND SPEECH_API_GOOGLE_HTTP_OLD)
    message(FATAL_ERROR "both SPEECH_API_GOOGLE_GRPC_V1 AND SPEECH_API_GOOGLE_HTTP_OLD given, nope")
endif ()

include_directories("thirdparty")
include_directories(".")

if (SPEECH_API_GOOGLE_GRPC_V1)
    message("using SPEECH_API_GOOGLE_GRPC_V1!")
    add_subdirectory(speech_apis/grpc_speech_api)
elseif (SPEECH_API_GOOGLE_HTTP_OLD)
    message("using SPEECH_API_GOOGLE_HTTP_OLD!")
    add_subdirectory(speech_apis/google_http_older)
else ()
    message("using SPEECH_API_GOOGLE_HTTP_OLD by default")
    add_subdirectory(speech_apis/google_http_older)
    #    message(FATAL_ERROR "either SPEECH_API_GOOGLE_GRPC_V1 or SPEECH_API_GOOGLE_HTTP_OLD required")
endif ()

set(caption_stream_SOURCES
        utils.h
        ContinuousCaptions.cpp
        )

set(caption_stream_HEADERS
        thirdparty/cameron314/concurrentqueue.h
        thirdparty/cameron314/blockingconcurrentqueue.h
        utils.h
        CaptionResult.h
        ContinuousCaptions.h
        )

if (BUILD_SHARED_LIB)
    message("caption_stream as shared lib")
    add_library(caption_stream
            ${SPEECH_API_SOURCES}
            ${caption_stream_SOURCES}
            ${caption_stream_HEADERS}
            )
else ()
    message("caption_stream as static lib")
    add_library(caption_stream STATIC
            ${SPEECH_API_SOURCES}
            ${caption_stream_SOURCES}
            ${caption_stream_HEADERS}
            )
endif ()
target_include_directories(caption_stream PUBLIC ./)
target_include_directories(caption_stream PUBLIC thirdparty/)
target_link_libraries(caption_stream speech_api_lib)

if (ENABLE_WINDOWS_STATIC_RUNTIME)
    message("ENABLE_WINDOWS_STATIC_RUNTIME ENABLED")
    #        set_property(TARGET caption_stream PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
    SET(ENABLE_WINDOWS_STATIC_RUNTIME ON
            PARENT_SCOPE)

    foreach (flag_var
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)

        message("flag var: ${flag_var}: " ${${flag_var}}"")
        if (${flag_var} MATCHES "/MD")
            string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
            message("new: ${flag_var}: " ${${flag_var}}"")
        endif (${flag_var} MATCHES "/MD")
    endforeach (flag_var)
endif ()

if (UNIX AND NOT APPLE)
    target_compile_options(caption_stream PRIVATE "-fPIC")
endif ()

if (DEVMODE)
    if (DEFINED QT_DEP_DIR AND NOT "${QT_DEP_DIR}" STREQUAL "")
        find_package(Qt5 REQUIRED COMPONENTS Widgets Core
                PATHS ${QT_DEP_DIR}
                NO_DEFAULT_PATH
                )
    else ()
        find_package(Qt5 REQUIRED COMPONENTS Widgets Core)
    endif ()

    add_executable(caption_stream_dev_main
            dev/main.cpp
            )
    target_link_libraries(caption_stream_dev_main
            caption_stream
            Qt5::Core
            )
endif ()