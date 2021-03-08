project(speech_api_http_google)

message("SETTING UP SPEECH API OLD")

set(PLIBSYS_ROOT_DIR "" CACHE STRING "Path containing plibsys repo root and cmake")
if (PLIBSYS_ROOT_DIR)
    message("using plibsys PLIBSYS_ROOT_DIR: ${PLIBSYS_ROOT_DIR}")
    set(PLIBSYS_DIR ${PLIBSYS_ROOT_DIR})
else ()
    message("using default CI plibsys repo dir")
    set(PLIBSYS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../CI/http/deps/plibsys)
endif ()

##PLIBSYS
option(PLIBSYS_TESTS "" OFF)
option(PLIBSYS_TESTS_STATIC "" OFF)
option(PLIBSYS_BUILD_STATIC "" ON)
include_directories(${PLIBSYS_DIR}/src ${CMAKE_BINARY_DIR})
add_subdirectory(${PLIBSYS_DIR} plibsys EXCLUDE_FROM_ALL)
###PLIBSYS

set(SPEECH_API_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/TcpConnection.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/CaptionStream.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/CaptionStream.h
        )

include_directories(${PLIBSYS_DIR}/src)

add_library(speech_api_stream STATIC
        ${SPEECH_API_SOURCES}
        )

if (UNIX AND NOT APPLE)
    target_compile_options(plibsysstatic PRIVATE "-fPIC")
    target_compile_options(speech_api_stream PRIVATE "-fPIC")
endif ()

target_include_directories(speech_api_stream PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(speech_api_stream PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/json11
        ${PLIBSYS_DIR}/src)

target_link_libraries(speech_api_stream plibsysstatic)
