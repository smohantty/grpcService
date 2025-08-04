cmake_minimum_required(VERSION 3.16)
project(ImageServiceGRPC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

# Use pkg-config to find gRPC and protobuf
pkg_check_modules(GRPC REQUIRED grpc++)
pkg_check_modules(PROTOBUF REQUIRED protobuf)

# Include generated *.pb.h files
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Proto file
get_filename_component(proto_file "image_service.proto" ABSOLUTE)
get_filename_component(proto_path "${proto_file}" PATH)

# Generated sources
set(proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/image_service.pb.cc")
set(proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/image_service.pb.h")
set(grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/image_service.grpc.pb.cc")
set(grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/image_service.grpc.pb.h")

# Find protoc and grpc_cpp_plugin
find_program(PROTOC protoc)
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin)

if(NOT PROTOC)
    message(FATAL_ERROR "protoc not found")
endif()

if(NOT GRPC_CPP_PLUGIN)
    message(FATAL_ERROR "grpc_cpp_plugin not found")
endif()

add_custom_command(
    OUTPUT "${proto_srcs}" "${proto_hdrs}" "${grpc_srcs}" "${grpc_hdrs}"
    COMMAND ${PROTOC}
    ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
         --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
         -I "${proto_path}"
         --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
         "${proto_file}"
    DEPENDS "${proto_file}")

# Include generated protobuf files
add_library(image_service_proto
    ${grpc_srcs}
    ${grpc_hdrs}
    ${proto_srcs}
    ${proto_hdrs})

target_include_directories(image_service_proto PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${GRPC_INCLUDE_DIRS}
    ${PROTOBUF_INCLUDE_DIRS})

target_link_directories(image_service_proto PUBLIC
    ${GRPC_LIBRARY_DIRS}
    ${PROTOBUF_LIBRARY_DIRS})

target_link_libraries(image_service_proto
    ${GRPC_LIBRARIES}
    ${PROTOBUF_LIBRARIES})

# Server executable
add_executable(image_server
    image_server.cpp
    ImageServiceAgent.cpp)

target_link_libraries(image_server
    image_service_proto
    ${GRPC_LIBRARIES}
    ${PROTOBUF_LIBRARIES}
    Threads::Threads)

# Client executable
add_executable(image_client
    image_client.cpp)

target_link_libraries(image_client
    image_service_proto
    ${GRPC_LIBRARIES}
    ${PROTOBUF_LIBRARIES}
    Threads::Threads)