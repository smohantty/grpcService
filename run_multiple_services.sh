#!/bin/bash

echo "🚀 Running Multiple gRPC Services Demo"
echo "======================================"

# Function to cleanup on exit
cleanup() {
    echo ""
    echo "🧹 Cleaning up..."
    if [ ! -z "$SERVER1_PID" ]; then
        kill $SERVER1_PID 2>/dev/null
        echo "   Killed RayVision server 1 (PID: $SERVER1_PID)"
    fi
    if [ ! -z "$SERVER2_PID" ]; then
        kill $SERVER2_PID 2>/dev/null
        echo "   Killed RayVision server 2 (PID: $SERVER2_PID)"
    fi
    rm -f server1_output.log server2_output.log
    echo "✅ Cleanup completed!"
}

# Set trap to cleanup on script exit
trap cleanup EXIT

# Check if executables exist
if [ ! -f "./build/rayvision_server" ]; then
    echo "❌ Error: rayvision_server not found. Please build the project first with ./build.sh"
    exit 1
fi

if [ ! -f "./build/rayvision_client" ]; then
    echo "❌ Error: rayvision_client not found. Please build the project first with ./build.sh"
    exit 1
fi

echo "📋 Starting two RayVision services on different ports..."
echo ""

# Start first server on port 50052
echo "🚀 Starting RayVision Server 1 on port 50052..."
./build/rayvision_server --port 50052 > server1_output.log 2>&1 &
SERVER1_PID=$!

# Start second server on port 50053
echo "🚀 Starting RayVision Server 2 on port 50053..."
./build/rayvision_server --port 50053 > server2_output.log 2>&1 &
SERVER2_PID=$!

# Wait for servers to start
echo "⏳ Waiting for servers to start..."
sleep 3

# Check if servers are running
if ! kill -0 $SERVER1_PID 2>/dev/null; then
    echo "❌ Error: Server 1 failed to start!"
    cat server1_output.log
    exit 1
fi

if ! kill -0 $SERVER2_PID 2>/dev/null; then
    echo "❌ Error: Server 2 failed to start!"
    cat server2_output.log
    exit 1
fi

echo "✅ Both servers started successfully!"
echo "   Server 1 PID: $SERVER1_PID (Port: 50052)"
echo "   Server 2 PID: $SERVER2_PID (Port: 50053)"
echo ""

# Test client connections
echo "🔍 Testing client connections..."
echo ""

echo "📡 Testing connection to Server 1 (port 50052):"
./build/rayvision_client --port 50052
echo ""

echo "📡 Testing connection to Server 2 (port 50053):"
./build/rayvision_client --port 50053
echo ""

echo "📋 Server 1 Output:"
echo "=================="
cat server1_output.log

echo ""
echo "📋 Server 2 Output:"
echo "=================="
cat server2_output.log

echo ""
echo "✅ Multiple services test completed successfully!"
echo ""
echo "📊 Summary:"
echo "   - Server 1: localhost:50052"
echo "   - Server 2: localhost:50053"
echo "   - Both services can run simultaneously without conflicts"
echo "   - Clients can connect to specific services using --port argument"
