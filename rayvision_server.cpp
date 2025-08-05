#include "RayVisionServiceAgent.h"
#include <iostream>
#include <memory>
#include <chrono>
#include <thread>

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

    auto listener = std::make_shared<RayVisionListener>();
    rayvision::RayVisionServiceAgent agent(listener);

    std::cout << "[MAIN] RayVision Service started. Press Enter to exit..." << std::endl;
    std::cin.get();

    std::cout << "[MAIN] Shutting down RayVision Service" << std::endl;
    return 0;
}