#!/bin/bash

# Build script for ImageService gRPC Project

echo "🚀 Building ImageService gRPC Project..."

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "❌ Error: CMakeLists.txt not found. Please run this script from the project root directory."
    exit 1
fi

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    echo "📁 Creating build directory..."
    mkdir build
fi

# Navigate to build directory
cd build

# Configure with CMake
echo "⚙️  Configuring with CMake..."
if ! cmake ..; then
    echo "❌ CMake configuration failed!"
    exit 1
fi

# Build the project
echo "🔨 Building project..."
# Use number of CPU cores for parallel build
CPU_CORES=$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
if ! make -j$CPU_CORES; then
    echo "❌ Build failed!"
    exit 1
fi

echo "✅ Build completed successfully!"
echo ""
echo "📋 Available executables:"
echo "   - ./build/image_server  (gRPC server)"
echo "   - ./build/image_client  (gRPC client)"
echo ""
echo "🚀 To run the service:"
echo "   1. Start server: ./build/image_server"
echo "   2. In another terminal, run client: ./build/image_client"
echo ""
echo "💡 For specific image requests: ./build/image_client unix:///tmp/image_service.sock img001"