#!/bin/bash

# Development setup script for ImageService gRPC Project

echo "🔧 Setting up development environment for ImageService gRPC Project..."
echo "=========================================="

# Check if we're on macOS
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "🍎 Detected macOS"

    # Check if Homebrew is installed
    if ! command -v brew &> /dev/null; then
        echo "❌ Homebrew is not installed. Please install it first:"
        echo "   /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
        exit 1
    fi

    echo "📦 Installing dependencies with Homebrew..."

    # Install gRPC and protobuf
    if ! brew list grpc &> /dev/null; then
        echo "   Installing gRPC..."
        brew install grpc
    else
        echo "   ✅ gRPC already installed"
    fi

    if ! brew list protobuf &> /dev/null; then
        echo "   Installing protobuf..."
        brew install protobuf
    else
        echo "   ✅ protobuf already installed"
    fi

    # Install Meson and Ninja
    if ! command -v meson &> /dev/null; then
        echo "   Installing Meson..."
        brew install meson
    else
        echo "   ✅ Meson already installed"
    fi

    if ! command -v ninja &> /dev/null; then
        echo "   Installing Ninja..."
        brew install ninja
    else
        echo "   ✅ Ninja already installed"
    fi

elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "🐧 Detected Linux"

    # Check if apt is available (Ubuntu/Debian)
    if command -v apt &> /dev/null; then
        echo "📦 Installing dependencies with apt..."
        sudo apt update
        sudo apt install -y build-essential cmake pkg-config
        sudo apt install -y libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc
        sudo apt install -y meson ninja-build

    # Check if dnf is available (Fedora/RHEL)
    elif command -v dnf &> /dev/null; then
        echo "📦 Installing dependencies with dnf..."
        sudo dnf install -y cmake gcc-c++ pkg-config
        sudo dnf install -y grpc-devel protobuf-devel grpc-plugins
        sudo dnf install -y meson ninja-build

    else
        echo "❌ Unsupported Linux distribution. Please install manually:"
        echo "   - gRPC development libraries"
        echo "   - protobuf compiler"
        echo "   - meson"
        echo "   - ninja"
        exit 1
    fi

else
    echo "❌ Unsupported operating system: $OSTYPE"
    echo "   Please install dependencies manually:"
    echo "   - gRPC development libraries"
    echo "   - protobuf compiler"
    echo "   - meson"
    echo "   - ninja"
    exit 1
fi

echo ""
echo "✅ Dependencies installed successfully!"
echo ""
echo "🔍 Verifying installation..."

# Verify installations
echo "   Checking protoc..."
if command -v protoc &> /dev/null; then
    echo "   ✅ protoc: $(protoc --version | head -n1)"
else
    echo "   ❌ protoc not found"
fi

echo "   Checking grpc_cpp_plugin..."
if command -v grpc_cpp_plugin &> /dev/null; then
    echo "   ✅ grpc_cpp_plugin: available"
else
    echo "   ❌ grpc_cpp_plugin not found"
fi

echo "   Checking meson..."
if command -v meson &> /dev/null; then
    echo "   ✅ meson: $(meson --version)"
else
    echo "   ❌ meson not found"
fi

echo "   Checking ninja..."
if command -v ninja &> /dev/null; then
    echo "   ✅ ninja: $(ninja --version)"
else
    echo "   ❌ ninja not found"
fi

echo ""
echo "🚀 Ready to build! You can now use:"
echo "   ./build_meson.sh    # Build with Meson"
echo "   ./build.sh          # Build with CMake (legacy)"
echo ""
echo "📚 For more information, see README.md"