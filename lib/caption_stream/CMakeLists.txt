project(caption_stream)

set(BUILD_SHARED_LIB "" CACHE STRING "build as shared lib")
set(BUILD_SHARED_LIB OFF)

if (SPEECH_API_GOOGLE_GRPC_V1 AND SPEECH_API_GOOGLE_HTTP_OLD)
    message(FATAL_ERROR "both SPEECH_API_GOOGLE_GRPC_V1 AND SPEECH_API_GOOGLE_HTTP_OLD given, nope")
endif ()

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

#message(STATUS "SPEECH_API_SOURCES ${SPEECH_API_SOURCES}")
message(STATUS "SPEECH_API_INCLUDES ${SPEECH_API_INCLUDES}")
message(STATUS "SPEECH_API_TARGET_INCLUDES_PUBLIC ${SPEECH_API_TARGET_INCLUDES_PUBLIC}")
message(STATUS "SPEECH_API_TARGET_COMPILE_DEFINITIONS_PRIVATE ${SPEECH_API_TARGET_COMPILE_DEFINITIONS_PRIVATE}")
message(STATUS "SPEECH_API_TARGET_LINK_DIRECTORIES_INTERFACE ${SPEECH_API_TARGET_LINK_DIRECTORIES_INTERFACE}")
message(STATUS "SPEECH_API_COMPILE_DEFINITIONS ${SPEECH_API_COMPILE_DEFINITIONS}")
message(STATUS "SPEECH_API_DEPENDENCIES ${SPEECH_API_DEPENDENCIES}")
message(STATUS "SPEECH_API_TARGET_LINK_LIBRARIES_PRIVATE ${SPEECH_API_TARGET_LINK_LIBRARIES_PRIVATE}")

list(LENGTH SPEECH_API_SOURCES sources_cnt)
message("sources count: ${sources_cnt}")


if (BUILD_SHARED_LIB)
    add_library(caption_stream
            ${SPEECH_API_SOURCES}
            ${caption_stream_SOURCES}
            ${caption_stream_HEADERS}
            )
else ()
    add_library(caption_stream STATIC
            ${SPEECH_API_SOURCES}
            ${caption_stream_SOURCES}
            ${caption_stream_HEADERS}
            )
endif ()

if (SPEECH_API_INCLUDES)
    include_directories(${SPEECH_API_INCLUDES})
endif ()

target_include_directories(caption_stream PUBLIC ./)
target_include_directories(caption_stream PUBLIC thirdparty/cameron314)

if (SPEECH_API_TARGET_INCLUDES_PUBLIC)
    target_include_directories(caption_stream PUBLIC ${SPEECH_API_TARGET_INCLUDES_PUBLIC})
endif ()

if (SPEECH_API_TARGET_LINK_LIBRARIES_PRIVATE)
    target_link_libraries(caption_stream PRIVATE ${SPEECH_API_TARGET_LINK_LIBRARIES_PRIVATE})
endif ()

if (SPEECH_API_TARGET_LINK_DIRECTORIES_INTERFACE)
    target_link_directories(caption_stream INTERFACE ${SPEECH_API_TARGET_LINK_DIRECTORIES_INTERFACE})
endif ()

if (SPEECH_API_TARGET_COMPILE_DEFINITIONS_PRIVATE)
    target_compile_definitions(caption_stream PRIVATE ${SPEECH_API_TARGET_COMPILE_DEFINITIONS_PRIVATE})
endif ()

if (SPEECH_API_DEPENDENCIES)
    add_dependencies(caption_stream ${SPEECH_API_DEPENDENCIES})
endif ()

if (SPEECH_API_COMPILE_DEFINITIONS)
    add_compile_definitions(${SPEECH_API_COMPILE_DEFINITIONS})
endif ()

if (DEVMODE)
    add_executable(caption_stream_dev_main
            ${caption_stream_SOURCES}
            ${caption_stream_HEADERS}
            dev/main.cpp
            )

    target_link_libraries(caption_stream_dev_main caption_stream)
endif ()