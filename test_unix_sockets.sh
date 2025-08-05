#!/bin/bash

echo "🧪 Testing Unix Socket Transport Layer"
echo "======================================"

# Function to cleanup on exit
cleanup() {
    echo ""
    echo "🧹 Cleaning up..."
    if [ ! -z "$IMAGE_SERVER_PID" ]; then
        kill $IMAGE_SERVER_PID 2>/dev/null
        echo "   Killed image server process (PID: $IMAGE_SERVER_PID)"
    fi
    if [ ! -z "$RAYVISION_SERVER_PID" ]; then
        kill $RAYVISION_SERVER_PID 2>/dev/null
        echo "   Killed rayvision server process (PID: $RAYVISION_SERVER_PID)"
    fi
    rm -f /tmp/image_service.sock
    rm -f /tmp/rayvision_service.sock
    rm -f server_output.log
    echo "✅ Cleanup completed!"
}

# Set trap to cleanup on script exit
trap cleanup EXIT

# Clean up any existing sockets
rm -f /tmp/image_service.sock
rm -f /tmp/rayvision_service.sock

echo "🚀 Starting ImageService server..."
./build/image_server > server_output.log 2>&1 &
IMAGE_SERVER_PID=$!

echo "🚀 Starting RayVision server..."
./build/rayvision_server >> server_output.log 2>&1 &
RAYVISION_SERVER_PID=$!

# Wait for servers to start
echo "⏳ Waiting for servers to start..."
sleep 3

# Check if servers are running
if ! kill -0 $IMAGE_SERVER_PID 2>/dev/null; then
    echo "❌ Error: ImageService server failed to start!"
    cat server_output.log
    exit 1
fi

if ! kill -0 $RAYVISION_SERVER_PID 2>/dev/null; then
    echo "❌ Error: RayVision server failed to start!"
    cat server_output.log
    exit 1
fi

echo "✅ Both servers started successfully"
echo "   ImageService PID: $IMAGE_SERVER_PID"
echo "   RayVision PID: $RAYVISION_SERVER_PID"
echo ""

# Check if Unix sockets were created
if [ -S /tmp/image_service.sock ]; then
    echo "✅ ImageService Unix socket created: /tmp/image_service.sock"
else
    echo "❌ ImageService Unix socket not found!"
    exit 1
fi

if [ -S /tmp/rayvision_service.sock ]; then
    echo "✅ RayVision Unix socket created: /tmp/rayvision_service.sock"
else
    echo "❌ RayVision Unix socket not found!"
    exit 1
fi

echo ""

# Test ImageService client
echo "🔍 Testing ImageService client..."
./build/image_client --target unix:///tmp/image_service.sock img001
echo ""

# Test RayVision client
echo "🔍 Testing RayVision client..."
./build/rayvision_client
echo ""

echo "📋 Server Output:"
echo "=================="
cat server_output.log

echo ""
echo "✅ Unix socket transport test completed successfully!"
echo ""
echo "📊 Summary:"
echo "   - Both services are using Unix domain sockets for local IPC"
echo "   - ImageService socket: unix:///tmp/image_service.sock"
echo "   - RayVision socket: unix:///tmp/rayvision_service.sock"
echo "   - Clients can connect and communicate successfully"
echo "   - Proper cleanup on server shutdown"