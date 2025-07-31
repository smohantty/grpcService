#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>

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
    ImageServiceClient(std::shared_ptr<Channel> channel)
        : stub_(ImageService::NewStub(channel)) {}

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
};

int main(int argc, char** argv) {
    std::cout << "Starting ImageService gRPC Client..." << std::endl;

    // Default server address - Unix domain socket for local IPC
    std::string target_str = "unix:///tmp/image_service.sock";

    // Allow user to specify server address as command line argument
    if (argc > 1) {
        target_str = argv[1];
    }

    std::cout << "Connecting to server via Unix socket: " << target_str << std::endl;

    // Instantiate the client. It requires a channel, out of which the actual RPCs
    // are created. This channel models a connection to an endpoint specified by
    // the argument "--target=" which is the only expected argument.
    ImageServiceClient client(
        grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

    // Check if specific image ID was provided as command line argument
    if (argc > 2) {
        std::string image_id = argv[2];
        std::cout << "Requesting specific image: " << image_id << std::endl;
        client.GetImage(image_id);
    } else {
        // Run multiple test requests
        client.TestMultipleRequests();
    }

    std::cout << "Client finished." << std::endl;
    return 0;
}