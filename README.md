# ImageService gRPC Project

A C++17 gRPC service that provides an image retrieval interface. The project includes both a server that can handle multiple concurrent clients and a client that can request image data.

## Features

- **gRPC Service Interface**:
  - `GetImage()` method that returns `ImageData`
  - `doSegmentation()` method with streaming callbacks for real-time progress updates
  - `subscribeToNotifications()` method for server push notifications to all connected clients
- **Unix Domain Socket IPC**: Client and server communicate via Unix domain sockets for efficient local IPC
- **Multi-client Support**: Server can handle multiple concurrent client connections
- **C++17 Implementation**: Modern C++ with proper error handling
- **Sample Data**: Pre-loaded with sample images for testing
- **Real-time Callbacks**: Server-side streaming for segmentation progress updates
- **Server Push Notifications**: Bidirectional streaming for broadcasting messages to all subscribers

## Project Structure

```
grpcService/
├── image_service.proto      # Protocol buffer definition
├── image_server.cpp         # Server implementation
├── image_client.cpp         # Client implementation
├── CMakeLists.txt          # CMake build configuration
├── meson.build              # Meson build configuration
├── meson_options.txt        # Meson build options
├── build.sh                 # CMake build script
├── build_meson.sh           # Meson build script
├── setup_dev.sh             # Development environment setup
├── test_segmentation.sh     # Segmentation test script
├── test_notifications.sh    # Notification test script
├── README.md               # This file
└── requirement.txt         # Original requirements
```

## Prerequisites

Before building this project, you need to install the following dependencies:

### macOS (using Homebrew)

```bash
# Install required packages
brew install grpc protobuf cmake pkg-config

# Verify installation
protoc --version
grpc_cpp_plugin --help
```

### Ubuntu/Debian

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install -y build-essential cmake pkg-config
sudo apt-get install -y libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc

# For newer versions, you might need:
sudo apt-get install -y libgrpc++1-dev
```

### CentOS/RHEL/Fedora

```bash
# Install dependencies
sudo dnf install -y cmake gcc-c++ pkg-config
sudo dnf install -y grpc-devel protobuf-devel grpc-plugins

# For older versions use yum instead of dnf
```

## Building the Project

### Option 1: Using the build script (Recommended)
```bash
./build.sh
```

### Option 2: Manual build

1. **Create build directory:**
   ```bash
   mkdir build
   cd build
   ```

2. **Configure with CMake:**
   ```bash
   cmake ..
   ```

3. **Build the project:**
   ```bash
   # On macOS:
   make -j$(sysctl -n hw.ncpu)

   # On Linux:
   make -j$(nproc)
   ```

   This will generate:
   - `image_server` - The gRPC server executable
   - `image_client` - The gRPC client executable
   - Generated protobuf files in the build directory

### Option 3: Using Meson (Alternative Build System)

The project also supports Meson as an alternative build system, which offers faster builds and better dependency management.

#### Prerequisites for Meson

```bash
# Install Meson and Ninja
pip install meson ninja

# Or on macOS:
brew install meson ninja
```

#### Building with Meson

1. **Using the Meson build script (Recommended):**
   ```bash
   ./build_meson.sh
   ```

2. **Manual Meson build:**
   ```bash
   # Configure with Meson
   meson setup build_meson

   # Build the project
   cd build_meson
   ninja
   ```

3. **Development setup (installs dependencies):**
   ```bash
   ./setup_dev.sh
   ```

#### Meson Build Options

You can customize the build with various options:

```bash
# Configure with specific options
meson setup build_meson \
  --buildtype=release \
  --warning-level=2 \
  -Dtests=true \
  -Dwerror=false

# Build with specific options
cd build_meson
ninja -C . --verbose
```

#### Meson vs CMake

| Feature | CMake | Meson |
|---------|-------|-------|
| Build Speed | Standard | Faster |
| Dependency Management | Manual | Automatic |
| Configuration | Complex | Simple |
| Cross-platform | Yes | Yes |
| IDE Support | Excellent | Good |

## Running the Service

### 1. Start the Server

In one terminal, run the server:

```bash
./image_server
```

You should see output like:
```
Starting ImageService gRPC Server...
Initialized 3 sample images
ImageService server listening on Unix socket: unix:///tmp/image_service.sock
Available images: img001, img002, img003
Server supports multiple concurrent clients via Unix domain socket
```

### 2. Run the Client

In another terminal, run the client:

```bash
# Run with default test (requests multiple images)
./image_client

# Request a specific image
./image_client unix:///tmp/image_service.sock img001

# Connect to a different Unix socket
./image_client unix:///path/to/other/socket img002
```

## API Reference

### GetImage API

#### ImageData Message

The `ImageData` message contains the following fields:

- `image_id` (string): Unique identifier for the image
- `image_name` (string): Display name of the image
- `image_content` (bytes): Binary image data
- `format` (string): Image format (JPEG, PNG, GIF, etc.)
- `width` (int32): Image width in pixels
- `height` (int32): Image height in pixels
- `size` (int64): Size of image data in bytes

### doSegmentation API

#### SegmentationRequest Message

The `SegmentationRequest` message contains:

- `image_id` (string): ID of the image to segment
- `segmentation_type` (string): Type of segmentation (e.g., "object", "semantic", "instance")
- `parameters` (map<string, string>): Optional parameters for segmentation

#### SegmentationResult Message

The `SegmentationResult` message contains:

- `request_id` (string): Unique identifier for tracking the request
- `status` (string): Current status ("processing", "completed", "failed")
- `segmented_image` (bytes): The processed image data
- `result_format` (string): Format of the segmented image
- `metrics` (map<string, float>): Quality metrics for the segmentation
- `error_message` (string): Error details if the operation fails

#### Streaming Callback Flow

The `doSegmentation` API uses server-side streaming to provide real-time updates:

1. **Initial Response**: Server sends "processing" status with request ID
2. **Progress Updates**: Multiple "processing" responses with progress metrics
3. **Final Result**: "completed" status with segmented image and quality metrics
4. **Error Handling**: "failed" status with error message if something goes wrong

### subscribeToNotifications API

#### SubscriptionRequest Message

The `SubscriptionRequest` message contains:

- `client_id` (string): Unique identifier for the client
- `client_name` (string): Display name of the client
- `topics` (repeated string): Topics the client wants to subscribe to
- `preferences` (map<string, string>): Client preferences for notifications

#### ServerNotification Message

The `ServerNotification` message contains:

- `notification_id` (string): Unique identifier for the notification
- `topic` (string): Topic the notification belongs to
- `message` (string): The notification message
- `notification_type` (string): Type of notification ("info", "warning", "error", "update")
- `timestamp` (int64): Timestamp when the notification was created
- `metadata` (map<string, string>): Additional metadata
- `data` (bytes): Optional binary data

#### Bidirectional Streaming Flow

The `subscribeToNotifications` API uses bidirectional streaming:

1. **Client Subscription**: Client sends subscription request with topics and preferences
2. **Server Registration**: Server registers client and sends welcome notification
3. **Real-time Notifications**: Server can push notifications to all subscribed clients
4. **Topic-based Filtering**: Clients receive only notifications for topics they subscribed to
5. **Automatic Cleanup**: Server removes disconnected clients automatically

### Available Sample Images

The server comes pre-loaded with these sample images:

- `img001`: sample_image_1.jpg (1920x1080 JPEG)
- `img002`: sample_image_2.png (800x600 PNG)
- `img003`: sample_image_3.gif (640x480 GIF)

## Example Usage

### Segmentation Examples

```bash
# Perform object segmentation on img001
./image_client --segmentation object img001

# Perform semantic segmentation on img002
./image_client --segmentation semantic img002

# Perform instance segmentation on img003
./image_client --segmentation instance img003

# Test all segmentation types
./image_client --test-segmentation

# Subscribe to notifications
./image_client --test-notifications

# Run the comprehensive test scripts
./test_segmentation.sh
./test_notifications.sh
```

### Server Output
```
Starting ImageService gRPC Server...
Initialized 3 sample images
ImageService server listening on Unix socket: unix:///tmp/image_service.sock
Available images: img001, img002, img003
Server supports multiple concurrent clients via Unix domain socket
Received GetImage request for image_id: img001
Sending image: sample_image_1.jpg (size: 39 bytes)
```

### Client Output
```
Starting ImageService gRPC Client...
Connecting to server via Unix socket: unix:///tmp/image_service.sock
🚀 Testing multiple image requests...
===========================================
🔍 Requesting image: img001
✅ Successfully received image data:
   Image ID: img001
   Image Name: sample_image_1.jpg
   Format: JPEG
   Dimensions: 1920x1080
   Size: 39 bytes
   Content preview: DUMMY_JPEG_CONTENT_FOR_TESTING_PURPOSES...
```

## Testing

### Test Segmentation API
```bash
./test_segmentation.sh
```

### Test Notification API
```bash
./test_notifications.sh
```

### Running Tests with Meson

If you built the project with Meson, you can also run tests using Meson's test framework:

```bash
# Run all tests
cd build_meson
ninja test

# Run specific tests
meson test basic_functionality
meson test segmentation_test
meson test notification_test

# Run tests with verbose output
meson test --verbose
```

## Testing Multiple Clients

You can test the server's ability to handle multiple clients by running several client instances simultaneously:

```bash
# Terminal 1
./image_client unix:///tmp/image_service.sock img001

# Terminal 2 (simultaneously)
./image_client unix:///tmp/image_service.sock img002

# Terminal 3 (simultaneously)
./image_client unix:///tmp/image_service.sock img003
```

## Error Handling

The service includes proper error handling:

- **Image Not Found**: Returns `NOT_FOUND` status when requesting non-existent images
- **Connection Timeout**: Client sets a 30-second deadline for requests
- **Server Errors**: Graceful error reporting with status codes and messages

## Extending the Service

To add more functionality:

1. **Add new RPC methods** in `image_service.proto`
2. **Implement server logic** in `ImageServiceImpl` class
3. **Update client** to call new methods
4. **Regenerate protobuf files** by rebuilding

## Unix Domain Socket IPC

This implementation uses **Unix domain sockets** for inter-process communication, which provides several advantages over TCP/IP for local communication:

### Benefits of Unix Domain Sockets:

- **Performance**: Faster than TCP/IP for local communication (no network stack overhead)
- **Security**: File system permissions control access to the socket
- **Simplicity**: No need to manage port numbers or network interfaces
- **Efficiency**: Direct kernel-to-kernel communication

### Socket File Location:

The server creates a Unix domain socket at `/tmp/image_service.sock`. You can verify it exists when the server is running:

```bash
ls -la /tmp/image_service.sock
```

### Connection String Format:

- **Default**: `unix:///tmp/image_service.sock`
- **Custom path**: `unix:///path/to/your/socket`

## Protocol Buffer Definition

The service is defined in `image_service.proto`:

```protobuf
service ImageService {
  rpc GetImage(GetImageRequest) returns (ImageData);
}
```

## Troubleshooting

### Build Issues

- **gRPC not found**: Ensure gRPC is properly installed and CMake can find it
- **Protobuf errors**: Verify protobuf compiler version compatibility
- **C++17 errors**: Ensure your compiler supports C++17

### Runtime Issues

- **Connection refused**: Make sure the server is running and the socket file exists
- **Permission denied**: Check if the socket file has proper permissions
- **Socket not found**: Verify the server created `/tmp/image_service.sock`
- **Timeout errors**: Check if the server process is still running

## Performance Notes

- The server uses synchronous gRPC which handles multiple clients efficiently
- For high-throughput scenarios, consider implementing asynchronous server
- Image data is kept in memory; for production use, implement proper storage backend

## Security Considerations

- Current implementation uses insecure credentials (no TLS/SSL)
- For production deployment, implement proper authentication and encryption
- Validate image IDs to prevent potential security issues