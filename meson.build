project('ImageServiceGRPC', 'cpp',
  version : '1.0.0',
  default_options : [
    'cpp_std=c++17',
    'warning_level=2',
    'werror=false'
  ]
)

# Find dependencies
grpc_dep = dependency('grpc++')
protobuf_dep = dependency('protobuf')
thread_dep = dependency('threads')

# Use existing generated protobuf files from CMake build
proto_sources = [
  'build/image_service.pb.cc',
  'build/image_service.grpc.pb.cc'
]

proto_headers = [
  'build/image_service.pb.h',
  'build/image_service.grpc.pb.h'
]

# Create static library for generated protobuf files
proto_lib = static_library('image_service_proto',
  proto_sources,
  dependencies : [grpc_dep, protobuf_dep],
  include_directories : [include_directories('.'), include_directories('build')]
)

# Create server executable
image_server = executable('image_server',
  'image_server.cpp',
  link_with : proto_lib,
  dependencies : [grpc_dep, protobuf_dep, thread_dep],
  include_directories : [include_directories('.'), include_directories('build')],
  install : true,
  cpp_args : get_option('werror') ? ['-Werror'] : []
)

# Create client executable
image_client = executable('image_client',
  'image_client.cpp',
  link_with : proto_lib,
  dependencies : [grpc_dep, protobuf_dep, thread_dep],
  include_directories : [include_directories('.'), include_directories('build')],
  install : true,
  cpp_args : get_option('werror') ? ['-Werror'] : []
)

# Install proto file
install_data('image_service.proto', install_dir : 'share/proto')

# Tests
if get_option('tests')
  # Test for basic functionality
  test('basic_functionality', image_client,
    args : ['img001'],
    timeout : 30,
    workdir : meson.current_build_dir()
  )

  # Test for segmentation
  test('segmentation_test', image_client,
    args : ['--segmentation', 'object', 'img001'],
    timeout : 60,
    workdir : meson.current_build_dir()
  )

  # Test for notifications
  test('notification_test', image_client,
    args : ['--test-notifications'],
    timeout : 30,
    workdir : meson.current_build_dir()
  )
endif

# Summary
summary({
  'Build type': get_option('buildtype'),
  'C++ standard': get_option('cpp_std'),
  'Install prefix': get_option('prefix'),
  'Tests enabled': get_option('tests'),
  'Warning level': get_option('warning_level'),
  'Warnings as errors': get_option('werror'),
  'Server executable': image_server.full_path(),
  'Client executable': image_client.full_path(),
  'Proto file': 'image_service.proto',
}, section : 'Configuration')