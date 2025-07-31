#!/bin/bash

# Build script for ImageService gRPC Project using Meson

echo "🚀 Building ImageService gRPC Project with Meson..."

# Check if we're in the right directory
if [ ! -f "meson.build" ]; then
    echo "❌ Error: meson.build not found. Please run this script from the project root directory."
    exit 1
fi

# Check if meson is installed
if ! command -v meson &> /dev/null; then
    echo "❌ Error: meson is not installed."
    echo "   Install it with: pip install meson"
    echo "   Or on macOS: brew install meson"
    exit 1
fi

# Check if ninja is installed
if ! command -v ninja &> /dev/null; then
    echo "❌ Error: ninja is not installed."
    echo "   Install it with: pip install ninja"
    echo "   Or on macOS: brew install ninja"
    exit 1
fi

# Create build directory if it doesn't exist
if [ ! -d "build_meson" ]; then
    echo "📁 Creating build directory..."
    mkdir build_meson
fi

# Navigate to build directory
cd build_meson

# Configure with Meson
echo "⚙️  Configuring with Meson..."
if ! meson setup --buildtype=release ..; then
    echo "❌ Meson configuration failed!"
    exit 1
fi

# Build the project
echo "🔨 Building project..."
# Use number of CPU cores for parallel build
CPU_CORES=$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
if ! ninja -j$CPU_CORES; then
    echo "❌ Build failed!"
    exit 1
fi

echo "✅ Build completed successfully!"
echo ""
echo "📋 Available executables:"
echo "   - ./build_meson/image_server  (gRPC server)"
echo "   - ./build_meson/image_client  (gRPC client)"
echo ""
echo "🚀 To run the service:"
echo "   1. Start server: ./build_meson/image_server"
echo "   2. In another terminal, run client: ./build_meson/image_client"
echo ""
echo "💡 For specific image requests: ./build_meson/image_client unix:///tmp/image_service.sock img001"
echo ""
echo "🔧 Additional Meson commands:"
echo "   - Clean build: ninja clean"
echo "   - Reconfigure: meson setup --reconfigure .."
echo "   - Install: ninja install"