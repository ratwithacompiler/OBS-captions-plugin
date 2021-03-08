project(caption_stream)

set(USE_PKG_CONFIG_GRPC "" CACHE BOOL "Use pkgconfig to automatically find grpc and ")
set(GOOGLEAPIS_DIR "" CACHE STRING "Path containing grpc generated googleapis files")
set(USE_OS_CERTS "" CACHE STRING "let GRPC use the OS cert store")
set(GRPC_CMAKE_INCLUDE "" CACHE STRING "cmake toolchain file to use for finding GRPC")

if (USE_PKG_CONFIG_GRPC)
    message("using pkgconfig")
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(Protobuf REQUIRED protobuf)
    pkg_check_modules(GRPCPP REQUIRED grpc++)
    message("FOUND gRPC!")

    message("Protobuf_INCLUDE_DIRS ${Protobuf_INCLUDE_DIRS}")
    message("GRPCPP_INCLUDE_DIRS ${GRPCPP_INCLUDE_DIRS}")

    message("Protobuf_LIBRARY_DIRS ${Protobuf_LIBRARY_DIRS}")
    message("GRPCPP_LIBRARY_DIRS ${GRPCPP_LIBRARY_DIRS}")

    SET(GRPC_NAME grpc)
else ()
    message("using find_package")

    if (GRPC_CMAKE_INCLUDE)
        message("including GRPC_CMAKE_INCLUDE: ${GRPC_CMAKE_INCLUDE}")
        include(${GRPC_CMAKE_INCLUDE})
    endif ()

    find_package(gRPC CONFIG REQUIRED)
    find_package(Protobuf CONFIG REQUIRED)
    message("FOUND gRPC!")

    message("gRPC_DIR ${gRPC_DIR}")
    message("Protobuf_DIR ${Protobuf_DIR}")

    SET(GRPC_NAME gRPC::grpc++)
endif ()

if (WIN32 AND NOT BUILD_SHARED_LIBS)
    message(BUILD_SHARED_LIBS: setting win flags)
    foreach (flag_var
            CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)

        if (${flag_var} MATCHES "/MD")
            message("before: ${flag_var}: " ${${flag_var}}"")
            string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
            message("after : ${flag_var}: " ${${flag_var}}"")
        endif ()
    endforeach ()
endif ()

#if (BUILD_SHARED_LIB)
#    #TODO: add SPEECH_API_COPY_LIBRARY_FILES PARENT to provide list of required dlls/dylibs to copy to bin?
#    IF (APPLE)
#
#    elseif (WIN32)
#    else ()
#        message(FATAL_ERROR "unsupported platform")
#    endif ()
#endif ()


if (WIN32)
    set(PLATFORM_LINK_LIBRARIES ws2_32)
    SET(SPEECH_API_TARGET_COMPILE_DEFINITIONS_PRIVATE _WIN32_WINNT=0x0600)
endif ()

if (GOOGLEAPIS_DIR)
    # check if the main cloud speech file exists to check if the given dir is correct
    set(_SPEECH_FILE "google/cloud/speech/v1/cloud_speech.grpc.pb.h")
    if (EXISTS "${GOOGLEAPIS_DIR}/${_SPEECH_FILE}")
        SET(GOOGLEAPIS_PATH "${GOOGLEAPIS_DIR}")
    elseif (EXISTS "${GOOGLEAPIS_DIR}/gens/${_SPEECH_FILE}")
        SET(GOOGLEAPIS_PATH "${GOOGLEAPIS_DIR}/gens")
    else ()
        message(FATAL_ERROR "GOOGLEAPIS_PATH appears invalid, no ${_SPEECH_FILE} found: ${GOOGLEAPIS_DIR} ${CMAKE_CURRENT_SOURCE_DIR}")
    endif ()
else ()
    message(FATAL_ERROR "no GOOGLEAPIS_DIR")
endif ()

message("using GOOGLEAPIS_PATH: ${GOOGLEAPIS_PATH}")
include_directories(${GOOGLEAPIS_PATH})


file(GLOB_RECURSE GOOGLE_API ${GOOGLEAPIS_PATH}/google/api/*.cc)
file(GLOB_RECURSE GOOGLE_RPC ${GOOGLEAPIS_PATH}/google/rpc/*.cc)
file(GLOB_RECURSE GOOGLE_TYPE ${GOOGLEAPIS_PATH}/google/type/*.cc)
file(GLOB_RECURSE GOOGLE_LOGGING ${GOOGLEAPIS_PATH}/google/logging/*.cc)
file(GLOB_RECURSE GOOGLE_LONGRUNNING ${GOOGLEAPIS_PATH}/google/longrunning/*.cc)
file(GLOB_RECURSE GOOGLE_SPEECH ${GOOGLEAPIS_PATH}/google/cloud/speech/v1/*.cc)

set(_GOOGLE_API_FILES_NAT
        ${GOOGLE_API}
        ${GOOGLE_RPC}
        ${GOOGLE_TYPE}
        ${GOOGLE_LOGGING}
        ${GOOGLE_LONGRUNNING}
        ${GOOGLE_SPEECH}
        )

# fix all globbed Windows path backslashes '\' to '/' otherwise add_library() gives an error
set(GOOGLE_API_FILES "")
foreach (_a_file_nat ${_GOOGLE_API_FILES_NAT})
    FILE(TO_CMAKE_PATH ${_a_file_nat} _a_file)
    LIST(APPEND GOOGLE_API_FILES "${_a_file}")
endforeach ()

#message("API FILES: ${GOOGLE_API_FILES}")
list(LENGTH GOOGLE_API_FILES sources_cnt)
message("Google API sources count: ${sources_cnt}")

if (USE_OS_CERTS)
    message("grpc: USE_OS_CERTS, attempting OS certs")
else ()
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/certs/roots.h" AND DEVMODE)
        message("roots.h exists, not recreating")
    else ()
        message("creating roots.h")
        include(certs/bin2h.cmake)
        bin2h(SOURCE_FILE certs/roots.pem HEADER_FILE certs/roots.h VARIABLE_NAME roots_pem)
    endif ()
    message("grpc: using included certs")
    SET(SPEECH_API_COMPILE_DEFINITIONS GRPC_USE_INCLUDED_CERTS=1)
endif ()

set(SPEECH_API_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/CaptionStream.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/CaptionStream.h

        ${GOOGLE_API_FILES}
        )

add_library(speech_api_stream STATIC
        ${SPEECH_API_SOURCES}
        )

if (UNIX AND NOT APPLE)
    target_compile_options(speech_api_stream PRIVATE "-fPIC")
endif ()

target_include_directories(speech_api_stream PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(speech_api_stream PRIVATE ${Protobuf_INCLUDE_DIRS})

target_link_libraries(speech_api_stream PRIVATE ${GRPC_NAME})
target_link_libraries(speech_api_stream PUBLIC ${PLATFORM_LINK_LIBRARIES})

target_compile_definitions(speech_api_stream PRIVATE
        ${SPEECH_API_COMPILE_DEFINITIONS}
        ${SPEECH_API_TARGET_COMPILE_DEFINITIONS_PRIVATE})
