#!/bin/bash

echo "🧪 Testing Meaningful Client Names"
echo "=================================="

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

# Test 1: Client with custom name
echo "🔍 Test 1: Client with custom name 'web_client'"
./build/image_client --name web_client img001
echo ""

# Test 2: Another client with different name
echo "🔍 Test 2: Client with custom name 'mobile_app'"
./build/image_client --name mobile_app img002
echo ""

# Test 3: Client with auto-generated name
echo "🔍 Test 3: Client with auto-generated name"
./build/image_client img003
echo ""

# Test 4: Multiple clients with different names (concurrent)
echo "🔍 Test 4: Multiple clients with different names (concurrent)"
./build/image_client --name admin_panel img001 &
./build/image_client --name api_gateway img002 &
./build/image_client --name batch_processor img003 &
wait
echo ""

# Test 5: Invalid request with meaningful name
echo "🔍 Test 5: Invalid request with meaningful name"
./build/image_client --name error_test_client img999
echo ""

# Wait a bit for any pending operations
sleep 2

# Show server output
echo "📋 Server Output (Meaningful Client Names):"
echo "============================================"
cat server_output.log

# Clean up
echo ""
echo "🧹 Cleaning up..."
kill $SERVER_PID 2>/dev/null
rm -f server_output.log
rm -f /tmp/image_service.sock

echo "✅ Test completed!"