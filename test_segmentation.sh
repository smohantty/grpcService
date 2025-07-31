#!/bin/bash

# Test script for the new doSegmentation API

echo "🧪 Testing ImageService Segmentation API"
echo "=========================================="

# Check if server is running
if ! pgrep -f "image_server" > /dev/null; then
    echo "❌ Server is not running. Please start the server first:"
    echo "   ./build/image_server"
    exit 1
fi

echo "✅ Server is running"
echo ""

# Test 1: Basic segmentation
echo "🔍 Test 1: Basic object segmentation"
echo "----------------------------------------"
./build/image_client --segmentation object img001
echo ""

# Test 2: Different segmentation types
echo "🔍 Test 2: Semantic segmentation"
echo "----------------------------------------"
./build/image_client --segmentation semantic img002
echo ""

echo "🔍 Test 3: Instance segmentation"
echo "----------------------------------------"
./build/image_client --segmentation instance img003
echo ""

# Test 3: Error case (non-existent image)
echo "🔍 Test 4: Error case - non-existent image"
echo "----------------------------------------"
./build/image_client --segmentation object img999
echo ""

# Test 4: Run all segmentation tests
echo "🔍 Test 5: Running all segmentation tests"
echo "----------------------------------------"
./build/image_client --test-segmentation
echo ""

echo "✅ All segmentation tests completed!"
echo ""
echo "📊 Summary:"
echo "   - The doSegmentation API uses server-side streaming for real-time callbacks"
echo "   - Progress updates are sent during processing"
echo "   - Final results include quality metrics"
echo "   - Error handling for invalid image IDs"
echo "   - Support for different segmentation types (object, semantic, instance)"