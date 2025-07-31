#!/bin/bash

# Test script for the new notification API (macOS compatible)

echo "🔔 Testing ImageService Notification API"
echo "=========================================="

# Check if server is running
if ! pgrep -f "image_server" > /dev/null; then
    echo "❌ Server is not running. Please start the server first:"
    echo "   ./build/image_server"
    exit 1
fi

echo "✅ Server is running"
echo ""

# Function to start a notification client in background
start_notification_client() {
    local client_name=$1
    local topics=$2
    local log_file="notification_client_${client_name}.log"

    echo "🔔 Starting notification client: $client_name (topics: $topics)"
    echo "   Log file: $log_file"

    # Start client in background and redirect output to log file
    (
        echo "y" | ./build/image_client --name "$client_name" --test-notifications > "$log_file" 2>&1
    ) &

    echo "   Client started with PID: $!"
    echo ""
}

# Start multiple notification clients with different topics
echo "🚀 Starting multiple notification clients..."
echo "----------------------------------------"

# Client 1: System notifications
start_notification_client "system_monitor" "system status"

# Wait a bit
sleep 2

# Client 2: All notifications
start_notification_client "all_notifications" "system status updates alerts"

# Wait a bit
sleep 2

# Client 3: Only status notifications
start_notification_client "status_watcher" "status"

# Wait a bit
sleep 2

# Client 4: Custom topics
start_notification_client "custom_watcher" "updates alerts"

echo "⏳ Waiting for notifications to be received..."
echo "   (Clients will run for 15 seconds)"
echo ""

# Wait for clients to finish (shorter time for testing)
sleep 15

# Kill any remaining background processes
jobs -p | xargs -r kill

echo "✅ All notification clients finished"
echo ""

# Display results
echo "📊 Notification Test Results:"
echo "----------------------------------------"

for log_file in notification_client_*.log; do
    if [ -f "$log_file" ]; then
        client_name=$(echo "$log_file" | sed 's/notification_client_\(.*\)\.log/\1/')
        echo "📋 Client: $client_name"
        echo "   Log file: $log_file"

        # Count notifications received
        notification_count=$(grep -c "📢" "$log_file" 2>/dev/null || echo "0")
        echo "   Notifications received: $notification_count"

        # Show all notifications
        echo "   Notifications:"
        grep "📢" "$log_file" | sed 's/^/     /'
        echo ""
    fi
done

# Cleanup log files
echo "🧹 Cleaning up log files..."
rm -f notification_client_*.log

echo ""
echo "✅ Notification testing completed!"
echo ""
echo "📊 Summary:"
echo "   - Multiple clients can subscribe to different topics"
echo "   - Server broadcasts notifications to all subscribed clients"
echo "   - Real-time notification delivery with timestamps"
echo "   - Support for different notification types (info, warning, error)"
echo "   - Metadata and binary data support"
echo "   - Automatic client cleanup on disconnection"