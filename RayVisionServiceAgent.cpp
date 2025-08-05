#include "RayVisionServiceAgent.h"
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include "RayVision.grpc.pb.h"
#include <iostream>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <chrono>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using rayvisiongrpc::RayVisionGrpc;
using rayvisiongrpc::GetImageRequest;
using rayvisiongrpc::ImageData;
using rayvisiongrpc::SegmentationResult;
using rayvisiongrpc::CameraType;
using rayvisiongrpc::ColorSpace;

namespace rayvision {

class RayVisionServiceAgent::Impl {
public:
    Impl(std::weak_ptr<IRayVisionServiceListener> listener)
        : listener_(listener), stop_server_(false) {
        startServer();
    }

    ~Impl() {
        stopServer();
    }

    void sendSegmentationResult(const rayvision::SegmentationResult& segmentation_result) {
        std::lock_guard<std::mutex> lock(segmentation_results_mutex_);
        pending_segmentation_results_.push_back(segmentation_result);
        segmentation_results_cv_.notify_one();
    }

private:
    void startServer() {
        server_thread_ = std::thread([this]() {
            std::string server_address("0.0.0.0:50052"); // Different port from ImageService

            // Create service implementation
            auto service = std::make_unique<RayVisionServiceImpl>(this);

            // Build server
            ServerBuilder builder;
            builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
            builder.RegisterService(service.get());

            // Add health check service
            grpc::EnableDefaultHealthCheckService(true);

            // Build and start server
            {
                std::lock_guard<std::mutex> lock(server_mutex_);
                server_ = builder.BuildAndStart();
            }
            std::cout << "[RAYVISION] RayVisionServiceAgent server listening on " << server_address << std::endl;

            // Wait for server to shutdown
            server_->Wait();
        });
    }

    void stopServer() {
        stop_server_ = true;
        segmentation_results_cv_.notify_all();

        // Shutdown the server
        {
            std::lock_guard<std::mutex> lock(server_mutex_);
            if (server_) {
                std::cout << "[RAYVISION] Shutting down server..." << std::endl;
                server_->Shutdown();
            }
        }

        if (server_thread_.joinable()) {
            server_thread_.join();
        }

        // Clear server reference
        {
            std::lock_guard<std::mutex> lock(server_mutex_);
            server_.reset();
        }
    }

    // gRPC Service Implementation
    class RayVisionServiceImpl final : public RayVisionGrpc::Service {
    public:
        RayVisionServiceImpl(Impl* agent_impl) : agent_impl_(agent_impl) {}

        Status GetImage(ServerContext* context, const GetImageRequest* request,
                       rayvisiongrpc::ImageData* response) override {
            std::cout << "[RAYVISION] GetImage request received for camera type: " << request->type() << std::endl;

            auto listener = agent_impl_->listener_.lock();
            if (!listener) {
                response->set_width(0);
                response->set_height(0);
                response->set_colorspace(rayvisiongrpc::ColorSpace::RGB);
                response->set_buffer("");
                return Status(grpc::StatusCode::INTERNAL, "Listener not available");
            }

            try {
                // Call listener to get image data
                auto image_data = listener->onGetImage(request->type());

                // Convert to gRPC response
                response->set_width(image_data.width);
                response->set_height(image_data.height);
                response->set_colorspace(static_cast<rayvisiongrpc::ColorSpace>(image_data.colorspace));
                response->set_buffer(image_data.buffer);

                std::cout << "[RAYVISION] GetImage response prepared (size: " << response->buffer().size() << " bytes)" << std::endl;
                return Status::OK;
            } catch (const std::exception& e) {
                std::cerr << "[RAYVISION] GetImage error: " << e.what() << std::endl;
                response->set_width(0);
                response->set_height(0);
                response->set_colorspace(rayvisiongrpc::ColorSpace::RGB);
                response->set_buffer("");
                return Status(grpc::StatusCode::INTERNAL, "Failed to get image: " + std::string(e.what()));
            }
        }

        Status doSegmentation(ServerContext* context, const rayvisiongrpc::Empty* request,
                             ServerWriter<rayvisiongrpc::SegmentationResult>* writer) override {
            std::cout << "[RAYVISION] doSegmentation request received" << std::endl;

            auto listener = agent_impl_->listener_.lock();
            if (!listener) {
                return Status(grpc::StatusCode::INTERNAL, "Listener not available");
            }

            try {
                // Call listener to perform segmentation
                auto segmentation_result = listener->onDoSegmentation();

                // Convert to gRPC response
                rayvisiongrpc::SegmentationResult grpc_result;
                for (const auto& segment : segmentation_result.segments) {
                    auto* grpc_segment = grpc_result.add_segments();
                    grpc_segment->set_width(segment.width);
                    grpc_segment->set_height(segment.height);
                    grpc_segment->set_colorspace(static_cast<rayvisiongrpc::ColorSpace>(segment.colorspace));
                    grpc_segment->set_buffer(segment.buffer);
                }

                if (!writer->Write(grpc_result)) {
                    return Status(grpc::StatusCode::INTERNAL, "Failed to write segmentation result");
                }

                std::cout << "[RAYVISION] Segmentation result sent successfully" << std::endl;
                return Status::OK;
            } catch (const std::exception& e) {
                std::cerr << "[RAYVISION] Segmentation error: " << e.what() << std::endl;
                return Status(grpc::StatusCode::INTERNAL, "Segmentation failed: " + std::string(e.what()));
            }
        }

    private:
        Impl* agent_impl_;
    };

    std::weak_ptr<IRayVisionServiceListener> listener_;
    std::thread server_thread_;
    std::atomic<bool> stop_server_;
    std::unique_ptr<Server> server_; // Store server reference for shutdown
    std::mutex server_mutex_; // Protect server access

    // Segmentation results queue
    std::mutex segmentation_results_mutex_;
    std::condition_variable segmentation_results_cv_;
    std::deque<rayvision::SegmentationResult> pending_segmentation_results_;
};

// Public interface implementation
RayVisionServiceAgent::RayVisionServiceAgent(std::weak_ptr<IRayVisionServiceListener> listener)
    : mImpl(std::make_unique<Impl>(listener)) {
    std::cout << "[RAYVISION] RayVisionServiceAgent created" << std::endl;
}

RayVisionServiceAgent::~RayVisionServiceAgent() {
    std::cout << "[RAYVISION] RayVisionServiceAgent destroyed" << std::endl;
}

void RayVisionServiceAgent::sendSegmentationResult(const rayvision::SegmentationResult& segmentation_result) {
    mImpl->sendSegmentationResult(segmentation_result);
}

} // namespace rayvision