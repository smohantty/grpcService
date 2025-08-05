#!/bin/bash

echo "🧪 Testing RayVision Client and Server"
echo "======================================"

# Check if executables exist
if [ ! -f "./build/rayvision_server" ]; then
    echo "❌ Error: rayvision_server not found. Please build the project first with ./build.sh"
    exit 1
fi

if [ ! -f "./build/rayvision_client" ]; then
    echo "❌ Error: rayvision_client not found. Please build the project first with ./build.sh"
    exit 1
fi

# Function to cleanup on exit
cleanup() {
    echo ""
    echo "🧹 Cleaning up..."
    if [ ! -z "$SERVER_PID" ]; then
        kill $SERVER_PID 2>/dev/null
        echo "   Killed server process (PID: $SERVER_PID)"
    fi
    rm -f server_output.log
    echo "✅ Cleanup completed!"
}

# Set trap to cleanup on script exit
trap cleanup EXIT

# Start server in background
echo "🚀 Starting RayVision server..."
./build/rayvision_server > server_output.log 2>&1 &
SERVER_PID=$!

# Wait for server to start
echo "⏳ Waiting for server to start..."
sleep 3

# Check if server is running
if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo "❌ Error: Server failed to start!"
    echo "Server output:"
    cat server_output.log
    exit 1
fi

echo "📊 Server started with PID: $SERVER_PID"
echo ""

# Test 1: Basic client functionality
echo "🔍 Test 1: Testing RayVision client functionality"
echo "   - GetImage for HEAD camera"
echo "   - GetImage for BODY camera"
echo "   - doSegmentation streaming"
echo ""

./build/rayvision_client

echo ""
echo "📋 Server Output:"
echo "=================="
cat server_output.log

echo ""
echo "✅ RayVision test completed successfully!"