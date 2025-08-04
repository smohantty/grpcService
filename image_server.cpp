#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <signal.h>

#include "ImageServiceAgent.h"

using namespace vision;

// Global flag for graceful shutdown
std::atomic<bool> g_shutdown_requested(false);

// Signal handler for graceful shutdown
void signalHandler(int signal) {
    std::cout << "\n[SHUTDOWN] Received signal " << signal << ", shutting down gracefully..." << std::endl;
    g_shutdown_requested = true;
}

// Segmentation processor that handles the actual segmentation work
class SegmentationProcessor {
public:
    SegmentationProcessor() {
        std::cout << "[PROCESSOR] SegmentationProcessor initialized" << std::endl;
    }

    SegmentationResult processSegmentation(const std::string& image_id, const std::string& segmentation_type) {
        std::cout << "[PROCESSOR] Processing segmentation for image: " << image_id
                  << ", type: " << segmentation_type << std::endl;

        // Simulate segmentation processing
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));

        // Create segmentation result
        SegmentationResult result;
        result.segmentation_result = "SEGMENTED_RESULT_FOR_" + image_id + "_" + segmentation_type;

        std::cout << "[PROCESSOR] Segmentation completed for image: " << image_id << std::endl;

        return result;
    }
};

// Connector class that bridges between the agent and the segmentation processor
class VisionConnector : public ImageServiceAgent::IImageServiceListener,
                       public std::enable_shared_from_this<VisionConnector> {
public:
    VisionConnector() {
        std::cout << "[CONNECTOR] VisionConnector initialized" << std::endl;

        // Create the segmentation processor
        processor_ = std::make_unique<SegmentationProcessor>();

        std::cout << "[CONNECTOR] SegmentationProcessor created" << std::endl;
    }

    // Method to initialize the agent after the object is created as shared_ptr
    void initializeAgent() {
        // Create the agent with this connector as the listener
        agent_ = std::make_unique<ImageServiceAgent>(std::weak_ptr<ImageServiceAgent::IImageServiceListener>(shared_from_this()));
        std::cout << "[CONNECTOR] ImageServiceAgent created and connected" << std::endl;
    }

    void onDoSegmentation() override {
        std::cout << "[CONNECTOR] Segmentation requested, delegating to processor..." << std::endl;

        // Store the current request info for processing
        current_image_id_ = "img001"; // In a real implementation, this would come from the request
        current_segmentation_type_ = "object"; // In a real implementation, this would come from the request

        // Start processing in a separate thread to avoid blocking
        std::thread([this]() {
            try {
                // Delegate to the segmentation processor
                auto result = processor_->processSegmentation(current_image_id_, current_segmentation_type_);

                // Send the result back to the agent
                agent_->sendSegmentationResult(result);

                std::cout << "[CONNECTOR] Segmentation result sent back to agent" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "[CONNECTOR] Error during segmentation: " << e.what() << std::endl;

                // Send error result
                SegmentationResult error_result;
                error_result.segmentation_result = "ERROR: " + std::string(e.what());
                agent_->sendSegmentationResult(error_result);
            }
        }).detach();
    }

    ImageData onGetImage() override {
        std::cout << "[CONNECTOR] Image requested, returning sample data..." << std::endl;

        // Return sample image data
        ImageData image_data;
        image_data.image_data = "SAMPLE_IMAGE_DATA_FROM_CONNECTOR";
        image_data.image_type = "JPEG";

        std::cout << "[CONNECTOR] Returning image data (size: " << image_data.image_data.size() << " bytes)" << std::endl;

        return image_data;
    }

    // Method to get the agent (for external access if needed)
    ImageServiceAgent* getAgent() const {
        return agent_.get();
    }

private:
    std::unique_ptr<SegmentationProcessor> processor_;
    std::unique_ptr<ImageServiceAgent> agent_;
    std::string current_image_id_;
    std::string current_segmentation_type_;
};

// Main VisionApp class that manages the entire application
class VisionApp {
public:
    VisionApp() {
        std::cout << "[VISION_APP] VisionApp initialized" << std::endl;

        // Create the connector as a shared_ptr first
        connector_ = std::make_shared<VisionConnector>();

        // Initialize the agent after the connector is created as shared_ptr
        connector_->initializeAgent();

        std::cout << "[VISION_APP] VisionConnector created and agent is running" << std::endl;
    }

    ~VisionApp() {
        std::cout << "[VISION_APP] VisionApp shutting down..." << std::endl;
    }

    // Method to get the agent if needed for external operations
    ImageServiceAgent* getAgent() const {
        return connector_->getAgent();
    }

    // Method to check if the app is running properly
    bool isRunning() const {
        return connector_ != nullptr;
    }

private:
    std::shared_ptr<VisionConnector> connector_;
};



void RunServer() {
    std::cout << "[SERVER] Starting VisionApp..." << std::endl;

    // Create the VisionApp (which will create the connector and agent)
    auto vision_app = std::make_unique<VisionApp>();

    if (!vision_app->isRunning()) {
        throw std::runtime_error("Failed to initialize VisionApp");
    }

    std::cout << "[SERVER] VisionApp is running and ready to handle requests" << std::endl;

    // Keep the server running until shutdown is requested
    while (!g_shutdown_requested) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "[SERVER] Shutting down..." << std::endl;
}

int main(int argc, char** argv) {
    std::cout << "Starting VisionApp with ImageServiceAgent..." << std::endl;

    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    try {
        RunServer();
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Server failed: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "[SERVER] Server shutdown complete" << std::endl;
    return 0;
}