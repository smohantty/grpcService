#!/bin/bash

echo "🧪 Testing ImageService Segmentation Functionality"
echo "=================================================="

# Test 1: Object segmentation
echo "Test 1: Object segmentation"
echo "---------------------------"
./build/image_client --target unix:///tmp/image_service.sock --segmentation object img001
echo ""

# Test 2: Semantic segmentation
echo "Test 2: Semantic segmentation"
echo "-----------------------------"
./build/image_client --target unix:///tmp/image_service.sock --segmentation semantic img002
echo ""

# Test 3: Instance segmentation
echo "Test 3: Instance segmentation"
echo "-----------------------------"
./build/image_client --target unix:///tmp/image_service.sock --segmentation instance img003
echo ""

# Test 4: Error handling - non-existent image
echo "Test 4: Error handling - non-existent image"
echo "-------------------------------------------"
./build/image_client --target unix:///tmp/image_service.sock --segmentation object img999
echo ""

# Test 5: Comprehensive segmentation test
echo "Test 5: Comprehensive segmentation test"
echo "--------------------------------------"
./build/image_client --target unix:///tmp/image_service.sock --test-segmentation
echo ""

echo "✅ All segmentation tests completed!"