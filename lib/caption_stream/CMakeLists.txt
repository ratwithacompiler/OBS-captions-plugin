project(caption_stream)

set(caption_stream_SOURCES
        utils.h
        CaptionStream.cpp
        TcpConnection.cpp
        CaptionResultHandler.cpp
        ContinuousCaptions.cpp
        thirdparty/json11/json11.cpp
        )

set(caption_stream_HEADERS
        thirdparty/cameron314/concurrentqueue.h
        thirdparty/cameron314/blockingconcurrentqueue.h
        utils.h
        CaptionStream.h
        TcpConnection.h
        CaptionResultHandler.cpp
        ContinuousCaptions.h
        thirdparty/json11/json11.hpp
        )


add_library(caption_stream_static STATIC
        ${caption_stream_SOURCES}
        ${caption_stream_HEADERS}
        )

if (DEVMODE)
    add_executable(caption_stream_dev_main
            ${caption_stream_SOURCES}
            ${caption_stream_HEADERS}
            dev/main.cpp
            )
endif ()

include_directories(thirdparty/json11)

#PLIBSYS
set(PLIBSYS_DIR thirdparty/plibsys/)
option(PLIBSYS_TESTS "" OFF)
option(PLIBSYS_TESTS_STATIC "" OFF)
option(PLIBSYS_BUILD_STATIC "" ON)
include_directories(${PLIBSYS_DIR}/src ${CMAKE_BINARY_DIR})
add_subdirectory(${PLIBSYS_DIR} plibsys EXCLUDE_FROM_ALL)
##PLIBSYS

target_link_libraries(caption_stream_static plibsysstatic)

if (DEVMODE)
    target_link_libraries(caption_stream_dev_main caption_stream_static)
    #target_link_libraries(caption_stream_dev_main plibsys)
endif ()

target_include_directories(caption_stream_static PUBLIC ${PLIBSYS_DIR}/src)
target_include_directories(caption_stream_static PUBLIC ./)
target_include_directories(caption_stream_static PUBLIC thirdparty/cameron314)
target_include_directories(caption_stream_static PUBLIC thirdparty/json11)

