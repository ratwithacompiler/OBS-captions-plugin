project(speech_api_http_google)

message("SETTING UP SPEECH API OLD")

set(PLIBSYS_ROOT_DIR "" CACHE PATH "Path containing a local plibsys checkout")
option(PLIBSYS_TESTS "Build plibsys tests" OFF)
option(PLIBSYS_TESTS_STATIC "Build plibsys static tests" OFF)
option(PLIBSYS_BUILD_STATIC "Build the plibsys static library" ON)

if (PLIBSYS_ROOT_DIR)
    message("using plibsys PLIBSYS_ROOT_DIR: ${PLIBSYS_ROOT_DIR}")
    set(PLIBSYS_DIR ${PLIBSYS_ROOT_DIR})
else ()
    include(FetchContent)
    FetchContent_Declare(
            plibsys
            GIT_REPOSITORY https://github.com/saprykin/plibsys.git
            GIT_TAG fd23c003663c7b46b9cc50403636c86bcbc86e80
            GIT_SHALLOW FALSE
    )
    FetchContent_MakeAvailable(plibsys)
    set(PLIBSYS_DIR ${plibsys_SOURCE_DIR})
    message("using pinned plibsys checkout: ${PLIBSYS_DIR}")
endif ()

##PLIBSYS
include_directories(${PLIBSYS_DIR}/src ${CMAKE_BINARY_DIR})
if (NOT TARGET plibsysstatic)
    add_subdirectory(${PLIBSYS_DIR} plibsys EXCLUDE_FROM_ALL)
endif ()
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

target_compile_definitions(speech_api_stream PRIVATE
        GOOGLE="www.google.com"
        PORTUP=80
        PORTDOWN=80)

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
