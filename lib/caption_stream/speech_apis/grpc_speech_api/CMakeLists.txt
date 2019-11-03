project(caption_stream)

set(USE_PKG_CONFIG_GRPC "" CACHE BOOL "Use pkgconfig to automatically find grpc and ")
set(GRPC_ROOT_DIR "" CACHE BOOL "Path containing GRPC includes and libs")
set(PROTOBUF_ROOT_DIR "" CACHE BOOL "Path containing protobuf includes and libs")
set(GOOGLEAPIS_DIR "" CACHE STRING "Path containing grpc generated googleapis files")
set(USE_OS_CERTS "" CACHE STRING "let GRPC use the OS cert store")

if (USE_PKG_CONFIG_GRPC)
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(PROTOBOF REQUIRED protobuf)
    pkg_check_modules(GRPCPP REQUIRED grpc++)

    message("PROTOBOF_INCLUDE_DIRS ${PROTOBOF_INCLUDE_DIRS}")
    message("GRPCPP_INCLUDE_DIRS ${GRPCPP_INCLUDE_DIRS}")

    message("PROTOBOF_LIBRARY_DIRS ${PROTOBOF_LIBRARY_DIRS}")
    message("GRPCPP_LIBRARY_DIRS ${GRPCPP_LIBRARY_DIRS}")

    SET(PROTOBUF_ROOT ${PROTOBOF_LIBRARY_DIRS})
    SET(GRPC_ROOT ${GRPCPP_LIBRARY_DIRS})
else ()

    if (NOT GRPC_ROOT_DIR)
        message(FATAL_ERROR "no GRPC_ROOT_DIR given")
    endif ()
    SET(GRPC_ROOT "${GRPC_ROOT_DIR}")

    if (NOT PROTOBUF_ROOT_DIR)
        message(FATAL_ERROR "no PROTOBUF_ROOT_DIR given")
    endif ()
    SET(PROTOBUF_ROOT "${PROTOBUF_ROOT_DIR}")

    SET(SPEECH_INCLUDES
            ${GRPC_ROOT}
            ${GRPC_ROOT}/include
            ${PROTOBUF_ROOT}
            ${PROTOBUF_ROOT}/include
            )

    #    set(SPEECH_API_TARGET_LINK_DIRECTORIES_INTERFACE
    #            ${GRPC_ROOT}
    #            ${GRPC_ROOT}/lib
    #            ${PROTOBUF_ROOT}
    #            ${PROTOBUF_ROOT}/lib
    #
    #            PARENT_SCOPE
    #            )

endif ()

if (WIN32)
    set(NEEDED_LIBS
            address_sorting
            cares
            gpr
            grpc
            grpc++
            libprotobuf

            ssleay32
            libeay32
            zlib
            )
elseif (APPLE)
    set(NEEDED_LIBS
            address_sorting
            cares
            gpr
            grpc
            grpc++
            protobuf

            ssl
            crypto
            z
            )
endif ()

SET(NEEDED_LIBS_OUT
        )

if (WIN32)
    message(WARNING "BUILD_SHARED_LIB not enforced on Windows, passed .lib files need to be static/shared as BUILD_SHARED_LIB given")
endif ()

if (WIN32 AND NOT BUILD_SHARED_LIB)
    SET(SPEECH_API_WINDOWS_STATIC_RUNTIME ON
            PARENT_SCOPE)
endif ()

foreach (_a_needed_lib_name ${NEEDED_LIBS})
    # check if all dependency libs are found

    unset(needed_lib_path_${_a_needed_lib_name})
    unset(needed_lib_path_${_a_needed_lib_name} CACHE)

    if (APPLE)
        # check if it's the wanted type, dynamic or static
        IF (BUILD_SHARED_LIB)
            SET(_a_needed_lib_name "lib${_a_needed_lib_name}.dylib")
            message("BUILD_SHARED_LIB=ON, using shared grpc")
        else ()
            message("using static grpc")
            SET(_a_needed_lib_name "lib${_a_needed_lib_name}.a")
        endif ()
    elseif (WIN32)
        # everythhings .lib on Win
        IF (BUILD_SHARED_LIB)
        endif ()
    else ()
        message(FATAL_ERROR "unsupported platform")
    endif ()

    #    message("searching in ${PROTOBUF_ROOT} ${GRPC_ROOT}")
    message("checking for ${_a_needed_lib_name}")
    find_library(needed_lib_path_${_a_needed_lib_name}
            NAME ${_a_needed_lib_name}
            PATHS ${PROTOBUF_ROOT} ${GRPC_ROOT}
            PATH_SUFFIXES lib

            NO_DEFAULT_PATH NO_PACKAGE_ROOT_PATH NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH
            )

    STRING(COMPARE EQUAL "${needed_lib_path_${_a_needed_lib_name}}" "needed_lib_path_${_a_needed_lib_name}-NOTFOUND" LIB_NOT_FOUND)
    if (LIB_NOT_FOUND)
        message(FATAL_ERROR "required lib ${_a_needed_lib_name} not found")
    endif ()
    message("found lib ${_a_needed_lib_name} at ${needed_lib_path_${_a_needed_lib_name}}")

    FILE(TO_CMAKE_PATH ${needed_lib_path_${_a_needed_lib_name}} ${needed_lib_path_${_a_needed_lib_name}})
    LIST(APPEND NEEDED_LIBS_OUT "${needed_lib_path_${_a_needed_lib_name}}")
endforeach ()

message("NEEDED_LIBS_OUT ${NEEDED_LIBS_OUT}")

if (BUILD_SHARED_LIB)
    #TODO: add SPEECH_API_COPY_LIBRARY_FILES to provide list of required dlls/dylibs to copy to bin?
    IF (APPLE)

    elseif (WIN32)
    else ()
        message(FATAL_ERROR "unsupported platform")
    endif ()
endif ()


if (WIN32)
    set(PLATFORM_LINK_LIBRARIES
            ws2_32
            )

    SET(SPEECH_API_TARGET_COMPILE_DEFINITIONS_PRIVATE
            _WIN32_WINNT=0x0600
            PARENT_SCOPE
            )
endif ()

set(SPEECH_API_TARGET_LINK_LIBRARIES_PRIVATE
        ${NEEDED_LIBS_OUT}
        #        ${NEEDED_LIBS}
        ${PLATFORM_LINK_LIBRARIES}

        PARENT_SCOPE
        )


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
    message("using GOOGLEAPIS path: ${GOOGLEAPIS_PATH}")
else ()
    message(FATAL_ERROR "no GOOGLEAPIS_DIR")
endif ()

message("using GOOGLEAPIS_PATH: ${GOOGLEAPIS_PATH}")


file(GLOB_RECURSE GOOGLE_API ${GOOGLEAPIS_PATH}/google/api/*.cc)
file(GLOB_RECURSE GOOGLE_RPC ${GOOGLEAPIS_PATH}/google/rpc/*.cc)
#file(GLOB_RECURSE GOOGLE_TYPE ${GOOGLEAPIS_PATH}/google/type/*.cc)
#file(GLOB_RECURSE GOOGLE_LOGGING ${GOOGLEAPIS_PATH}/google/logging/*.cc)
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

set(SPEECH_API_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/CaptionStream.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/CaptionStream.h

        ${GOOGLE_API_FILES}

        PARENT_SCOPE
        )

SET(SPEECH_API_INCLUDES
        ${PROTOBOF_INCLUDE_DIRS}
        ${GRPC_INCLUDE_DIRS}
        ${GOOGLEAPIS_PATH}

        ${SPEECH_INCLUDES}

        PARENT_SCOPE
        )

SET(SPEECH_API_TARGET_INCLUDES_PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}

        PARENT_SCOPE
        )

if (NOT USE_OS_CERTS)
    SET(SPEECH_API_COMPILE_DEFINITIONS
            GRPC_USE_INCLUDED_CERTS=1

            PARENT_SCOPE
            )
endif ()
