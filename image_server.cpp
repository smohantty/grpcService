#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <thread>
#include <vector>
#include <unordered_map>
#include <unistd.h>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <atomic>
#include <random>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "image_service.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using imageservice::ImageService;
using imageservice::GetImageRequest;
using imageservice::ImageData;
using imageservice::SegmentationRequest;
using imageservice::SegmentationResult;

// Global connection tracking
class ConnectionTracker {
public:
    static ConnectionTracker& getInstance() {
        static ConnectionTracker instance;
        return instance;
    }

    void clientConnected() {
        int current = active_connections_.fetch_add(1) + 1;
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::cout << "[CONNECTION] New client connected. Active connections: "
                  << current << " at "
                  << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << std::endl;
    }

    void clientDisconnected() {
        int current = active_connections_.fetch_sub(1) - 1;
        if (current < 0) {
            active_connections_.store(0);
            current = 0;
        }
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::cout << "[CONNECTION] Client disconnected. Active connections: "
                  << current << " at "
                  << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << std::endl;
    }

    int getActiveConnections() const {
        return active_connections_.load();
    }

private:
    ConnectionTracker() = default;
    std::atomic<int> active_connections_{0};
};

// Logic and data behind the server's behavior.
class ImageServiceImpl final : public ImageService::Service {
public:
    ImageServiceImpl() {
        // Initialize sample image data
        initializeSampleImages();
    }

    Status GetImage(ServerContext* context, const GetImageRequest* request,
                   ImageData* reply) override {
        // Track connection (simplified approach)
        ConnectionTracker::getInstance().clientConnected();

        // Get client information from metadata
        std::string client_name = "unknown_client";
        auto client_name_metadata = context->client_metadata().find("client-name");
        if (client_name_metadata != context->client_metadata().end()) {
            client_name = std::string(client_name_metadata->second.begin(), client_name_metadata->second.end());
        }

        // Get client peer information as fallback
        std::string peer = context->peer();

        std::cout << "[REQUEST] Received GetImage request from: " << client_name << std::endl;
        std::cout << "[REQUEST] Request for image_id: " << request->image_id() << std::endl;
        std::cout << "[STATUS] Active connections: " << ConnectionTracker::getInstance().getActiveConnections() << std::endl;

        // Find the requested image
        auto it = sample_images_.find(request->image_id());
        if (it == sample_images_.end()) {
            std::cout << "[ERROR] Image not found: " << request->image_id() << std::endl;
            // Track disconnection on error
            ConnectionTracker::getInstance().clientDisconnected();
            return Status(grpc::StatusCode::NOT_FOUND, "Image not found");
        }

        // Set the reply with the found image data
        *reply = it->second;

        std::cout << "[RESPONSE] Sending image: " << reply->image_name()
                  << " (size: " << reply->size() << " bytes) to " << client_name << std::endl;

        // Track disconnection after successful response
        ConnectionTracker::getInstance().clientDisconnected();

        return Status::OK;
    }

    Status doSegmentation(ServerContext* context, const SegmentationRequest* request,
                         ServerWriter<SegmentationResult>* writer) override {
        // Track connection
        ConnectionTracker::getInstance().clientConnected();

        // Get client information from metadata
        std::string client_name = "unknown_client";
        auto client_name_metadata = context->client_metadata().find("client-name");
        if (client_name_metadata != context->client_metadata().end()) {
            client_name = std::string(client_name_metadata->second.begin(), client_name_metadata->second.end());
        }

        std::cout << "[SEGMENTATION] Received segmentation request from: " << client_name << std::endl;
        std::cout << "[SEGMENTATION] Image ID: " << request->image_id() << std::endl;
        std::cout << "[SEGMENTATION] Type: " << request->segmentation_type() << std::endl;

        // Generate a unique request ID
        std::string request_id = generateRequestId();

        // Check if the image exists
        auto it = sample_images_.find(request->image_id());
        if (it == sample_images_.end()) {
            SegmentationResult error_result;
            error_result.set_request_id(request_id);
            error_result.set_status("failed");
            error_result.set_error_message("Image not found: " + request->image_id());

            writer->Write(error_result);
            ConnectionTracker::getInstance().clientDisconnected();
            return Status::OK;
        }

        // Send initial processing status
        SegmentationResult processing_result;
        processing_result.set_request_id(request_id);
        processing_result.set_status("processing");
        processing_result.set_result_format("PNG");

        if (!writer->Write(processing_result)) {
            std::cout << "[ERROR] Failed to send processing status to " << client_name << std::endl;
            ConnectionTracker::getInstance().clientDisconnected();
            return Status(grpc::StatusCode::INTERNAL, "Failed to send processing status");
        }

        std::cout << "[SEGMENTATION] Started processing for " << client_name << std::endl;

        // Simulate segmentation processing with multiple steps
        std::vector<std::string> steps = {"Loading image", "Preprocessing", "Feature extraction", "Segmentation", "Post-processing"};

        for (size_t i = 0; i < steps.size(); ++i) {
            // Simulate processing time
            std::this_thread::sleep_for(std::chrono::milliseconds(500 + (i * 200)));

            // Send progress update
            SegmentationResult progress_result;
            progress_result.set_request_id(request_id);
            progress_result.set_status("processing");
            progress_result.set_result_format("PNG");

            // Add progress metrics
            float progress = (float)(i + 1) / steps.size();
            progress_result.mutable_metrics()->insert({"progress", progress});
            progress_result.mutable_metrics()->insert({"current_step", i + 1});
            progress_result.mutable_metrics()->insert({"total_steps", steps.size()});

            if (!writer->Write(progress_result)) {
                std::cout << "[ERROR] Failed to send progress update to " << client_name << std::endl;
                ConnectionTracker::getInstance().clientDisconnected();
                return Status(grpc::StatusCode::INTERNAL, "Failed to send progress update");
            }

            std::cout << "[SEGMENTATION] Progress for " << client_name << ": " << steps[i] << " (" << (progress * 100) << "%)" << std::endl;
        }

        // Generate segmented image content (simulated)
        std::string original_content = it->second.image_content();
        std::string segmented_content = "SEGMENTED_" + original_content + "_" + request->segmentation_type();

        // Send final result
        SegmentationResult final_result;
        final_result.set_request_id(request_id);
        final_result.set_status("completed");
        final_result.set_segmented_image(segmented_content);
        final_result.set_result_format("PNG");

        // Add quality metrics
        final_result.mutable_metrics()->insert({"accuracy", 0.95f});
        final_result.mutable_metrics()->insert({"iou_score", 0.87f});
        final_result.mutable_metrics()->insert({"processing_time_ms", 2000.0f});
        final_result.mutable_metrics()->insert({"segments_count", 5.0f});

        if (!writer->Write(final_result)) {
            std::cout << "[ERROR] Failed to send final result to " << client_name << std::endl;
            ConnectionTracker::getInstance().clientDisconnected();
            return Status(grpc::StatusCode::INTERNAL, "Failed to send final result");
        }

        std::cout << "[SEGMENTATION] Completed segmentation for " << client_name
                  << " (result size: " << segmented_content.size() << " bytes)" << std::endl;

        // Track disconnection
        ConnectionTracker::getInstance().clientDisconnected();
        return Status::OK;
    }

    // Method to get current connection statistics
    void PrintConnectionStats() const {
        std::cout << "[STATS] Current active connections: "
                  << ConnectionTracker::getInstance().getActiveConnections() << std::endl;
    }

private:
    void initializeSampleImages() {
        // Create sample image data
        ImageData image1;
        image1.set_image_id("img001");
        image1.set_image_name("sample_image_1.jpg");
        image1.set_format("JPEG");
        image1.set_width(1920);
        image1.set_height(1080);

        // Create dummy image content (small pattern for demonstration)
        std::string dummy_content = "DUMMY_JPEG_CONTENT_FOR_TESTING_PURPOSES";
        image1.set_image_content(dummy_content);
        image1.set_size(dummy_content.size());

        sample_images_["img001"] = image1;

        ImageData image2;
        image2.set_image_id("img002");
        image2.set_image_name("sample_image_2.png");
        image2.set_format("PNG");
        image2.set_width(800);
        image2.set_height(600);

        std::string dummy_content2 = "DUMMY_PNG_CONTENT_FOR_TESTING_PURPOSES";
        image2.set_image_content(dummy_content2);
        image2.set_size(dummy_content2.size());

        sample_images_["img002"] = image2;

        ImageData image3;
        image3.set_image_id("img003");
        image3.set_image_name("sample_image_3.gif");
        image3.set_format("GIF");
        image3.set_width(640);
        image3.set_height(480);

        std::string dummy_content3 = "DUMMY_GIF_CONTENT_FOR_TESTING_PURPOSES";
        image3.set_image_content(dummy_content3);
        image3.set_size(dummy_content3.size());

        sample_images_["img003"] = image3;

        std::cout << "Initialized " << sample_images_.size() << " sample images" << std::endl;
    }

    std::string generateRequestId() {
        static std::atomic<int> counter{0};
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(1000, 9999);

        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::stringstream ss;
        ss << "req_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S")
           << "_" << std::setfill('0') << std::setw(3) << ms.count()
           << "_" << dis(gen);

        return ss.str();
    }

    std::unordered_map<std::string, ImageData> sample_images_;
};

void RunServer() {
    // Use Unix domain socket for local IPC
    std::string server_address("unix:///tmp/image_service.sock");

    // Clean up any existing socket file
    std::string socket_path = "/tmp/image_service.sock";
    if (access(socket_path.c_str(), F_OK) == 0) {
        unlink(socket_path.c_str());
        std::cout << "Removed existing socket file: " << socket_path << std::endl;
    }

    ImageServiceImpl service;

    grpc::EnableDefaultHealthCheckService(true);
    ServerBuilder builder;

    // Listen on Unix domain socket for local IPC
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);

    // Finally assemble the server.
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "ImageService server listening on Unix socket: " << server_address << std::endl;
    std::cout << "Available images: img001, img002, img003" << std::endl;
    std::cout << "Server supports multiple concurrent clients via Unix domain socket" << std::endl;
    std::cout << "Connection tracking enabled - you'll see connection/disconnection events" << std::endl;

    // Start a background thread to periodically print connection stats
    std::thread stats_thread([&service]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(30));
            service.PrintConnectionStats();
        }
    });
    stats_thread.detach();

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}

int main(int argc, char** argv) {
    std::cout << "Starting ImageService gRPC Server with Connection Tracking..." << std::endl;
    RunServer();
    return 0;
}