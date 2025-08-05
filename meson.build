project('grpcService', 'cpp',
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

# Find protoc compiler and grpc plugin
protoc = find_program('protoc')
grpc_cpp_plugin = find_program('grpc_cpp_plugin')

# Generate protobuf and gRPC files for image_service.proto
image_service_proto_gen = custom_target('image_service_proto_gen',
  input : 'image_service.proto',
  output : ['image_service.pb.cc', 'image_service.pb.h', 'image_service.grpc.pb.cc', 'image_service.grpc.pb.h'],
  command : [
    protoc,
    '--proto_path=@CURRENT_SOURCE_DIR@',
    '--cpp_out=@OUTDIR@',
    '--grpc_out=@OUTDIR@',
    '--plugin=protoc-gen-grpc=' + grpc_cpp_plugin.full_path(),
    '@INPUT@'
  ]
)

# Generate protobuf and gRPC files for RayVision.proto
rayvision_proto_gen = custom_target('rayvision_proto_gen',
  input : 'RayVision.proto',
  output : ['RayVision.pb.cc', 'RayVision.pb.h', 'RayVision.grpc.pb.cc', 'RayVision.grpc.pb.h'],
  command : [
    protoc,
    '--proto_path=@CURRENT_SOURCE_DIR@',
    '--cpp_out=@OUTDIR@',
    '--grpc_out=@OUTDIR@',
    '--plugin=protoc-gen-grpc=' + grpc_cpp_plugin.full_path(),
    '@INPUT@'
  ]
)

# Create static library for image_service protobuf files
image_service_proto_lib = static_library('image_service_proto',
  [image_service_proto_gen[0], image_service_proto_gen[2]],
  dependencies : [grpc_dep, protobuf_dep],
  include_directories : include_directories('.')
)

# Create static library for RayVision protobuf files
rayvision_proto_lib = static_library('rayvision_proto',
  [rayvision_proto_gen[0], rayvision_proto_gen[2]],
  dependencies : [grpc_dep, protobuf_dep],
  include_directories : include_directories('.')
)

# Create library for ImageServiceAgent
image_service_agent_lib = static_library('image_service_agent',
  'ImageServiceAgent.cpp',
  link_with : image_service_proto_lib,
  dependencies : [grpc_dep, protobuf_dep],
  include_directories : include_directories('.')
)

# Create library for RayVisionServiceAgent
rayvision_service_agent_lib = static_library('rayvision_service_agent',
  'RayVisionServiceAgent.cpp',
  link_with : rayvision_proto_lib,
  dependencies : [grpc_dep, protobuf_dep],
  include_directories : include_directories('.')
)

# Create image_server executable
image_server = executable('image_server',
  'image_server.cpp',
  link_with : [image_service_proto_lib, image_service_agent_lib],
  dependencies : [grpc_dep, protobuf_dep, thread_dep],
  include_directories : include_directories('.'),
  install : true,
  cpp_args : get_option('werror') ? ['-Werror'] : []
)

# Create image_client executable
image_client = executable('image_client',
  'image_client.cpp',
  link_with : [image_service_proto_lib, image_service_agent_lib],
  dependencies : [grpc_dep, protobuf_dep, thread_dep],
  include_directories : include_directories('.'),
  install : true,
  cpp_args : get_option('werror') ? ['-Werror'] : []
)

# Create rayvision_server executable
rayvision_server = executable('rayvision_server',
  'rayvision_server.cpp',
  link_with : [rayvision_proto_lib, rayvision_service_agent_lib],
  dependencies : [grpc_dep, protobuf_dep, thread_dep],
  include_directories : include_directories('.'),
  install : true,
  cpp_args : get_option('werror') ? ['-Werror'] : []
)

# Create rayvision_client executable
rayvision_client = executable('rayvision_client',
  'rayvision_client.cpp',
  link_with : rayvision_proto_lib,
  dependencies : [grpc_dep, protobuf_dep, thread_dep],
  include_directories : include_directories('.'),
  install : true,
  cpp_args : get_option('werror') ? ['-Werror'] : []
)

# Install proto files
install_data(['image_service.proto', 'RayVision.proto'], install_dir : 'share/proto')

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

  # Test for RayVision client
  test('rayvision_basic', rayvision_client,
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
  'Image server executable': image_server.full_path(),
  'Image client executable': image_client.full_path(),
  'RayVision server executable': rayvision_server.full_path(),
  'RayVision client executable': rayvision_client.full_path(),
  'Proto files': 'image_service.proto, RayVision.proto',
}, section : 'Configuration')