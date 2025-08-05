#include "RayVisionServiceAgent.h"
#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <signal.h>
#include <unistd.h>
#include <atomic>

// Global flag for graceful shutdown
std::atomic<bool> g_shutdown_requested(false);

// Signal handler for graceful shutdown
void signalHandler(int signal) {
    std::cout << "\n[SHUTDOWN] Received signal " << signal << ", shutting down gracefully..." << std::endl;
    g_shutdown_requested = true;

    // Clean up Unix socket
    if (unlink("/tmp/rayvision_service.sock") == 0) {
        std::cout << "[SHUTDOWN] Unix socket cleaned up" << std::endl;
    }
}

class RayVisionListener : public rayvision::RayVisionServiceAgent::IRayVisionServiceListener {
public:
    rayvision::ImageData onGetImage(int cameraType) override {
        std::cout << "[LISTENER] Getting image for camera type: " << cameraType << std::endl;

        // Simulate getting image data
        rayvision::ImageData image_data;
        image_data.width = 1920;
        image_data.height = 1080;
        image_data.colorspace = 0; // RGB
        image_data.buffer = "simulated_image_data_" + std::to_string(cameraType);

        return image_data;
    }

    void onDoSegmentation() override {
        std::cout << "[LISTENER] Performing segmentation" << std::endl;

    }
};

int main() {
    std::cout << "[MAIN] Starting RayVision Service" << std::endl;

    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    auto listener = std::make_shared<RayVisionListener>();
    rayvision::RayVisionServiceAgent agent(listener);

    std::cout << "[MAIN] RayVision Service started. Press Ctrl+C to exit..." << std::endl;

    // Keep the server running until shutdown is requested
    while (!g_shutdown_requested) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "[MAIN] Shutting down RayVision Service" << std::endl;

    // Clean up Unix socket
    if (unlink("/tmp/rayvision_service.sock") == 0) {
        std::cout << "[MAIN] Unix socket cleaned up" << std::endl;
    }

    return 0;
}