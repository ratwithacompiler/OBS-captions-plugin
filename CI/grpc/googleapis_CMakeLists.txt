cmake_minimum_required(VERSION 3.12)

message("globbbing")

if (NOT GOOGLEAPIS_PATH)
    message(FATAL_ERROR "no GOOGLEAPIS_PATH path")
endif ()

if (NOT PROTO_INCLUDE_PATH)
    message(FATAL_ERROR "no PROTO_INCLUDE_PATH path")
endif ()

if (NOT PROTOC_PATH)
    message(FATAL_ERROR "no PROTOC_PATH path")
endif ()

if (NOT PROTOC_CPP_PATH)
    message(FATAL_ERROR "no PROTOC_CPP_PATH path")
endif ()

file(GLOB_RECURSE PROTO_FILES_GOOGLE
        RELATIVE ${GOOGLEAPIS_PATH}
        ${GOOGLEAPIS_PATH}/google/*.proto
        )
file(GLOB_RECURSE PROTO_FILES_INCLUDE
        RELATIVE ${PROTO_INCLUDE_PATH}
        ${PROTO_INCLUDE_PATH}/google/protobuf/*.proto
        )

message("PROTO_FILES_GOOGLE: ${PROTO_FILES_GOOGLE}")
#message("PROTO_FILES_INCLUDE: ${PROTO_FILES_INCLUDE}")


foreach (proto_file
        ${PROTO_FILES_GOOGLE}
        )
    string(REGEX REPLACE ".proto$" ".pb.cc" proto_out_file ${proto_file})
    message("proto_file: ${proto_file} -> ${proto_out_file}")

    #    message("${PROTOC_PATH} -I. -I${PROTO_INCLUDE_PATH} --plugin=protoc-gen-grpc=${PROTOC_CPP_PATH} --cpp_out=gens --grpc_out=gens ${proto_file}")

    execute_process(
            COMMAND ${PROTOC_PATH} -I. -I${PROTO_INCLUDE_PATH} --plugin=protoc-gen-grpc=${PROTOC_CPP_PATH} --cpp_out=gens --grpc_out=gens ${proto_file}
            WORKING_DIRECTORY ${GOOGLEAPIS_PATH}
            RESULT_VARIABLE RES_CODE
    )

    if(NOT RES_CODE EQUAL 0)
        message(FATAL_ERROR "protoc failed ${proto_file}")
        break()
    endif()
endforeach ()