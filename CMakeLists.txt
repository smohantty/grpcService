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

# Proto files
get_filename_component(image_proto_file "image_service.proto" ABSOLUTE)
get_filename_component(rayvision_proto_file "RayVision.proto" ABSOLUTE)
get_filename_component(proto_path "${image_proto_file}" PATH)

# Generated sources for ImageService
set(image_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/image_service.pb.cc")
set(image_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/image_service.pb.h")
set(image_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/image_service.grpc.pb.cc")
set(image_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/image_service.grpc.pb.h")

# Generated sources for RayVision
set(rayvision_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/RayVision.pb.cc")
set(rayvision_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/RayVision.pb.h")
set(rayvision_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/RayVision.grpc.pb.cc")
set(rayvision_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/RayVision.grpc.pb.h")

# Find protoc and grpc_cpp_plugin
find_program(PROTOC protoc)
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin)

if(NOT PROTOC)
    message(FATAL_ERROR "protoc not found")
endif()

if(NOT GRPC_CPP_PLUGIN)
    message(FATAL_ERROR "grpc_cpp_plugin not found")
endif()

# Generate ImageService protobuf files
add_custom_command(
    OUTPUT "${image_proto_srcs}" "${image_proto_hdrs}" "${image_grpc_srcs}" "${image_grpc_hdrs}"
    COMMAND ${PROTOC}
    ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
         --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
         -I "${proto_path}"
         --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
         "${image_proto_file}"
    DEPENDS "${image_proto_file}")

# Generate RayVision protobuf files
add_custom_command(
    OUTPUT "${rayvision_proto_srcs}" "${rayvision_proto_hdrs}" "${rayvision_grpc_srcs}" "${rayvision_grpc_hdrs}"
    COMMAND ${PROTOC}
    ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
         --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
         -I "${proto_path}"
         --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
         "${rayvision_proto_file}"
    DEPENDS "${rayvision_proto_file}")

# Include generated protobuf files for ImageService
add_library(image_service_proto
    ${image_grpc_srcs}
    ${image_grpc_hdrs}
    ${image_proto_srcs}
    ${image_proto_hdrs})

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

# Include generated protobuf files for RayVision
add_library(rayvision_proto
    ${rayvision_grpc_srcs}
    ${rayvision_grpc_hdrs}
    ${rayvision_proto_srcs}
    ${rayvision_proto_hdrs})

target_include_directories(rayvision_proto PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${GRPC_INCLUDE_DIRS}
    ${PROTOBUF_INCLUDE_DIRS})

target_link_directories(rayvision_proto PUBLIC
    ${GRPC_LIBRARY_DIRS}
    ${PROTOBUF_LIBRARY_DIRS})

target_link_libraries(rayvision_proto
    ${GRPC_LIBRARIES}
    ${PROTOBUF_LIBRARIES})

# ImageService Server executable
add_executable(image_server
    image_server.cpp
    ImageServiceAgent.cpp)

target_link_libraries(image_server
    image_service_proto
    Threads::Threads)

# ImageService Client executable
add_executable(image_client
    image_client.cpp)

target_link_libraries(image_client
    image_service_proto
    Threads::Threads)

# RayVision Server executable
add_executable(rayvision_server
    rayvision_server.cpp
    RayVisionServiceAgent.cpp)

target_link_libraries(rayvision_server
    rayvision_proto
    Threads::Threads)

# RayVision Client executable
add_executable(rayvision_client
    rayvision_client.cpp)

target_link_libraries(rayvision_client
    rayvision_proto
    Threads::Threads)