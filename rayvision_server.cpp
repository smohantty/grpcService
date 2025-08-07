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

int main(int argc, char** argv) {
    // Default port for RayVision service
    int port = 50052;

    // Parse command line arguments for port
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        }
    }

    std::cout << "[MAIN] Starting RayVision Service on port " << port << std::endl;

    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    auto listener = std::make_shared<RayVisionListener>();
    rayvision::RayVisionServiceAgent agent(listener, port);

    std::cout << "[MAIN] RayVision Service started on port " << port << ". Press Ctrl+C to exit..." << std::endl;

    // Keep the server running until shutdown is requested
    while (!g_shutdown_requested) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "[MAIN] Shutting down RayVision Service" << std::endl;

    return 0;
}