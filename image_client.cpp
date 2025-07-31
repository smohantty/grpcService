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

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--name" && i + 1 < argc) {
            client_name = argv[++i];
        } else if (arg == "--target" && i + 1 < argc) {
            target_str = argv[++i];
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

    // Check if specific image ID was provided
    if (!image_id.empty()) {
        std::cout << "Requesting specific image: " << image_id << std::endl;
        client.GetImage(image_id);
    } else {
        // Run multiple test requests
        client.TestMultipleRequests();
    }

    std::cout << "Client finished." << std::endl;
    return 0;
}