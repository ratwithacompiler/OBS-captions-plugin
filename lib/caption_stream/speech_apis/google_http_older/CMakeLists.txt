include(ExternalProject)

message("SETTING UP SPEECH API OLD")

set(PLIBSYS_ROOT_DIR "" CACHE STRING "Path containing plibsys repo root and cmake")
if (PLIBSYS_ROOT_DIR)
    message("using plibsys PLIBSYS_ROOT_DIR: ${PLIBSYS_ROOT_DIR}")
    set(PLIBSYS_DIR ${PLIBSYS_ROOT_DIR})
else ()
    message("using default CI plibsys repo dir")
    set(PLIBSYS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../CI/deps/plibsys)
endif ()

##PLIBSYS
option(PLIBSYS_TESTS "" OFF)
option(PLIBSYS_TESTS_STATIC "" OFF)
option(PLIBSYS_BUILD_STATIC "" ON)
include_directories(${PLIBSYS_DIR}/src ${CMAKE_BINARY_DIR})
add_subdirectory(${PLIBSYS_DIR} plibsys EXCLUDE_FROM_ALL)
###PLIBSYS

if (UNIX AND NOT APPLE)
    target_compile_options(plibsysstatic PRIVATE "-fPIC")
endif ()

set(SPEECH_API_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/TcpConnection.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/CaptionStream.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/CaptionStream.h

        PARENT_SCOPE
        )

SET(SPEECH_API_INCLUDES
        ${PLIBSYS_DIR}/src
        PARENT_SCOPE
        )

SET(SPEECH_API_TARGET_INCLUDES_PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/json11

        ${PLIBSYS_DIR}/src
        PARENT_SCOPE
        )

SET(SPEECH_API_TARGET_LINK_LIBRARIES_PRIVATE
        plibsysstatic

        PARENT_SCOPE
        )
