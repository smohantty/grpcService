#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include <random>

#include <grpcpp/grpcpp.h>

#include "image_service.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using imageservice::ImageService;
using imageservice::GetImageRequest;
using imageservice::ImageData;
using imageservice::SegmentationRequest;
using imageservice::SegmentationResult;

class ImageServiceClient {
public:
    ImageServiceClient(std::shared_ptr<Channel> channel, const std::string& client_name = "default_client")
        : stub_(ImageService::NewStub(channel)), client_name_(client_name) {}

    // Assembles the client's payload, sends it and presents the response back
    // from the server.
    bool GetImage(const std::string& image_id) {
        // Data we are sending to the server.
        GetImageRequest request;
        request.set_image_id(image_id);

        // Container for the data we expect from the server.
        ImageData reply;

        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        // Set client name in metadata
        context.AddMetadata("client-name", client_name_);

        // Set a deadline for the RPC call (optional)
        auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(30);
        context.set_deadline(deadline);

        // The actual RPC.
        Status status = stub_->GetImage(&context, request, &reply);

        // Act upon its status.
        if (status.ok()) {
            std::cout << "✅ Successfully received image data:" << std::endl;
            std::cout << "   Image ID: " << reply.image_id() << std::endl;
            std::cout << "   Image Name: " << reply.image_name() << std::endl;
            std::cout << "   Format: " << reply.format() << std::endl;
            std::cout << "   Dimensions: " << reply.width() << "x" << reply.height() << std::endl;
            std::cout << "   Size: " << reply.size() << " bytes" << std::endl;
            std::cout << "   Content preview: " << reply.image_content().substr(0, 50) << "..." << std::endl;
            std::cout << std::endl;
            return true;
        } else {
            std::cout << "❌ RPC failed for image_id '" << image_id << "'" << std::endl;
            std::cout << "   Error code: " << status.error_code() << std::endl;
            std::cout << "   Error message: " << status.error_message() << std::endl;
            std::cout << std::endl;
            return false;
        }
    }

    // Segmentation method that handles streaming responses
    bool doSegmentation(const std::string& image_id, const std::string& segmentation_type = "object") {
        std::cout << "🔍 Starting segmentation for image: " << image_id << std::endl;
        std::cout << "   Type: " << segmentation_type << std::endl;
        std::cout << "   Client: " << client_name_ << std::endl;
        std::cout << "===========================================" << std::endl;

        // Create the segmentation request
        SegmentationRequest request;
        request.set_image_id(image_id);
        request.set_segmentation_type(segmentation_type);

        // Add some optional parameters
        request.mutable_parameters()->insert({"quality", "high"});
        request.mutable_parameters()->insert({"algorithm", "deep_learning"});

        // Context for the client
        ClientContext context;
        context.AddMetadata("client-name", client_name_);

        // Set a longer deadline for segmentation (it might take time)
        auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(60);
        context.set_deadline(deadline);

        // Create the streaming reader
        std::unique_ptr<grpc::ClientReader<SegmentationResult>> reader(
            stub_->doSegmentation(&context, request));

        SegmentationResult result;
        bool first_response = true;
        std::string request_id;

        // Read all responses from the stream
        while (reader->Read(&result)) {
            if (first_response) {
                request_id = result.request_id();
                std::cout << "📋 Request ID: " << request_id << std::endl;
                first_response = false;
            }

            std::cout << "📡 Received callback: " << result.status() << std::endl;

            if (result.status() == "processing") {
                // Show progress information
                if (result.metrics().find("progress") != result.metrics().end()) {
                    float progress = result.metrics().at("progress");
                    int current_step = result.metrics().at("current_step");
                    int total_steps = result.metrics().at("total_steps");

                    std::cout << "   ⏳ Progress: " << (progress * 100) << "% (Step "
                              << current_step << "/" << total_steps << ")" << std::endl;
                }

                if (result.metrics().find("processing_time_ms") != result.metrics().end()) {
                    float time_ms = result.metrics().at("processing_time_ms");
                    std::cout << "   ⏱️  Processing time: " << time_ms << " ms" << std::endl;
                }
            }
            else if (result.status() == "completed") {
                std::cout << "✅ Segmentation completed successfully!" << std::endl;
                std::cout << "   📊 Result format: " << result.result_format() << std::endl;
                std::cout << "   📏 Segmented image size: " << result.segmented_image().size() << " bytes" << std::endl;

                // Display quality metrics
                std::cout << "   📈 Quality metrics:" << std::endl;
                for (const auto& metric : result.metrics()) {
                    std::cout << "      - " << metric.first << ": " << metric.second << std::endl;
                }

                std::cout << "   📄 Content preview: " << result.segmented_image().substr(0, 50) << "..." << std::endl;
                break;
            }
            else if (result.status() == "failed") {
                std::cout << "❌ Segmentation failed!" << std::endl;
                std::cout << "   🚨 Error: " << result.error_message() << std::endl;
                return false;
            }
        }

        // Check if the stream ended successfully
        Status status = reader->Finish();
        if (!status.ok()) {
            std::cout << "❌ Stream failed: " << status.error_message() << std::endl;
            return false;
        }

        std::cout << "===========================================" << std::endl;
        std::cout << "🎉 Segmentation stream completed successfully!" << std::endl;
        std::cout << std::endl;
        return true;
    }

    // Test multiple requests to demonstrate concurrent capability
    void TestMultipleRequests() {
        std::vector<std::string> test_images = {"img001", "img002", "img003", "img999"}; // img999 doesn't exist

        std::cout << "🚀 Testing multiple image requests..." << std::endl;
        std::cout << "===========================================" << std::endl;

        for (const auto& image_id : test_images) {
            std::cout << "🔍 Requesting image: " << image_id << std::endl;
            GetImage(image_id);

            // Small delay between requests for readability
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    // Test segmentation functionality
    void TestSegmentation() {
        std::vector<std::pair<std::string, std::string>> test_segmentations = {
            {"img001", "object"},
            {"img002", "semantic"},
            {"img003", "instance"},
            {"img999", "object"}  // This should fail
        };

        std::cout << "🚀 Testing segmentation functionality..." << std::endl;
        std::cout << "===========================================" << std::endl;

        for (const auto& test : test_segmentations) {
            doSegmentation(test.first, test.second);

            // Small delay between requests for readability
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }

private:
    std::unique_ptr<ImageService::Stub> stub_;
    std::string client_name_;
};

// Generate a random client name
std::string generateClientName() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1000, 9999);

    return "client_" + std::to_string(dis(gen));
}

int main(int argc, char** argv) {
    std::cout << "Starting ImageService gRPC Client..." << std::endl;

    // Default server address - Unix domain socket for local IPC
    std::string target_str = "unix:///tmp/image_service.sock";
    std::string client_name = generateClientName();
    std::string image_id = "";
    std::string segmentation_type = "";
    bool test_segmentation = false;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--name" && i + 1 < argc) {
            client_name = argv[++i];
        } else if (arg == "--target" && i + 1 < argc) {
            target_str = argv[++i];
        } else if (arg == "--segmentation" && i + 1 < argc) {
            segmentation_type = argv[++i];
        } else if (arg == "--test-segmentation") {
            test_segmentation = true;
        } else if (arg[0] != '-') {
            // Non-flag argument - treat as image_id if we don't have one yet
            if (image_id.empty()) {
                image_id = arg;
            }
        }
    }

    std::cout << "Connecting to server via: " << target_str << std::endl;
    std::cout << "Client name: " << client_name << std::endl;

    // Instantiate the client
    ImageServiceClient client(
        grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()),
        client_name);

    // Check what operation to perform
    if (test_segmentation) {
        // Run segmentation tests
        client.TestSegmentation();
    } else if (!segmentation_type.empty() && !image_id.empty()) {
        // Perform specific segmentation
        std::cout << "Performing segmentation on image: " << image_id << std::endl;
        std::cout << "Segmentation type: " << segmentation_type << std::endl;
        client.doSegmentation(image_id, segmentation_type);
    } else if (!image_id.empty()) {
        // Get specific image
        std::cout << "Requesting specific image: " << image_id << std::endl;
        client.GetImage(image_id);
    } else {
        // Run multiple test requests
        client.TestMultipleRequests();
    }

    std::cout << "Client finished." << std::endl;
    return 0;
}