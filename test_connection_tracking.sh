#!/bin/bash

echo "🧪 Testing Connection Tracking Functionality"
echo "=============================================="

# Clean up any existing socket
if [ -S /tmp/image_service.sock ]; then
    rm /tmp/image_service.sock
    echo "🧹 Cleaned up existing socket"
fi

# Start server in background
echo "🚀 Starting server..."
./build/image_server > server_output.log 2>&1 &
SERVER_PID=$!

# Wait for server to start
sleep 2

echo "📊 Server started with PID: $SERVER_PID"
echo ""

# Test 1: Single client request
echo "🔍 Test 1: Single client request"
./build/image_client unix:///tmp/image_service.sock img001
echo ""

# Test 2: Multiple requests
echo "🔍 Test 2: Multiple requests"
./build/image_client unix:///tmp/image_service.sock img002 &
./build/image_client unix:///tmp/image_service.sock img003 &
wait
echo ""

# Test 3: Invalid request
echo "🔍 Test 3: Invalid request (should show error)"
./build/image_client unix:///tmp/image_service.sock img999
echo ""

# Wait a bit for any pending operations
sleep 2

# Show server output
echo "📋 Server Output (Connection Tracking Logs):"
echo "=============================================="
cat server_output.log

# Clean up
echo ""
echo "🧹 Cleaning up..."
kill $SERVER_PID 2>/dev/null
rm -f server_output.log
rm -f /tmp/image_service.sock

echo "✅ Test completed!" 