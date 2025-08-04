#include "ImageServiceAgent.h"
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include "image_service.grpc.pb.h"
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
using grpc::ServerReaderWriter;
using grpc::Status;
using imageservice::ImageService;
using imageservice::GetImageRequest;
using imageservice::ImageData;
using imageservice::SegmentationRequest;
using imageservice::SegmentationResult;
using imageservice::SubscriptionRequest;
using imageservice::ServerNotification;

namespace vision {

class ImageServiceAgent::Impl {
public:
    Impl(std::weak_ptr<IImageServiceListener> listener)
        : listener_(listener), stop_server_(false) {
        startServer();
    }

    ~Impl() {
        stopServer();
    }

    void sendSegmentationResult(const SegmentationResult& segmentation_result) {
        std::lock_guard<std::mutex> lock(segmentation_results_mutex_);
        pending_segmentation_results_.push_back(segmentation_result);
        segmentation_results_cv_.notify_one();
    }

private:
    void startServer() {
        server_thread_ = std::thread([this]() {
            std::string server_address("0.0.0.0:50051");

            // Create service implementation
            auto service = std::make_unique<ImageServiceImpl>(this);

            // Build server
            ServerBuilder builder;
            builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
            builder.RegisterService(service.get());

            // Add health check service
            grpc::EnableDefaultHealthCheckService(true);

            // Build and start server
            std::unique_ptr<Server> server(builder.BuildAndStart());
            std::cout << "[AGENT] ImageServiceAgent server listening on " << server_address << std::endl;

            // Wait for server to shutdown
            server->Wait();
        });
    }

    void stopServer() {
        stop_server_ = true;
        segmentation_results_cv_.notify_all();
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
    }

    // Traditional gRPC Service Implementation
    class ImageServiceImpl final : public ImageService::Service {
    public:
        ImageServiceImpl(Impl* agent_impl) : agent_impl_(agent_impl) {}

        Status GetImage(ServerContext* context, const imageservice::GetImageRequest* request,
                       imageservice::ImageData* response) override {
            std::cout << "[AGENT] GetImage request received for image_id: " << request->image_id() << std::endl;

            auto listener = agent_impl_->listener_.lock();
            if (!listener) {
                response->set_image_id(request->image_id());
                response->set_image_name("error");
                response->set_image_content("Listener not available");
                response->set_format("error");
                response->set_width(0);
                response->set_height(0);
                response->set_size(0);
                return Status(grpc::StatusCode::INTERNAL, "Listener not available");
            }

            try {
                // Call listener to get image data
                auto image_data = listener->onGetImage();

                // Convert to gRPC response
                response->set_image_id(request->image_id());
                response->set_image_name("image_from_listener");
                response->set_image_content(image_data.image_data);
                response->set_format(image_data.image_type);
                response->set_width(1920);
                response->set_height(1080);
                response->set_size(image_data.image_data.size());

                std::cout << "[AGENT] GetImage response prepared (size: " << response->size() << " bytes)" << std::endl;
                return Status::OK;
            } catch (const std::exception& e) {
                std::cerr << "[AGENT] GetImage error: " << e.what() << std::endl;
                response->set_image_id(request->image_id());
                response->set_image_name("error");
                response->set_image_content("Failed to get image: " + std::string(e.what()));
                response->set_format("error");
                response->set_width(0);
                response->set_height(0);
                response->set_size(0);
                return Status(grpc::StatusCode::INTERNAL, "Failed to get image: " + std::string(e.what()));
            }
        }

        Status doSegmentation(ServerContext* context, const imageservice::SegmentationRequest* request,
                             ServerWriter<imageservice::SegmentationResult>* writer) override {
            std::cout << "[AGENT] doSegmentation request received for image_id: " << request->image_id()
                      << ", type: " << request->segmentation_type() << std::endl;

            auto listener = agent_impl_->listener_.lock();
            if (!listener) {
                return Status(grpc::StatusCode::INTERNAL, "Listener not available");
            }

            try {
                // Call listener to perform segmentation
                listener->onDoSegmentation();

                // Send initial processing status
                imageservice::SegmentationResult processing_result;
                processing_result.set_request_id(request->image_id());
                processing_result.set_status("processing");
                processing_result.set_result_format("raw");

                if (!writer->Write(processing_result)) {
                    return Status(grpc::StatusCode::INTERNAL, "Failed to write processing status");
                }

                // Wait for segmentation result
                std::unique_lock<std::mutex> lock(agent_impl_->segmentation_results_mutex_);
                agent_impl_->segmentation_results_cv_.wait(lock, [this]() {
                    return !agent_impl_->pending_segmentation_results_.empty() || agent_impl_->stop_server_;
                });

                if (agent_impl_->stop_server_) {
                    return Status(grpc::StatusCode::CANCELLED, "Server shutting down");
                }

                // Send the segmentation result
                if (!agent_impl_->pending_segmentation_results_.empty()) {
                    const auto& result = agent_impl_->pending_segmentation_results_.front();

                    imageservice::SegmentationResult grpc_result;
                    grpc_result.set_request_id(request->image_id());
                    grpc_result.set_status("completed");
                    grpc_result.set_segmented_image(result.segmentation_result);
                    grpc_result.set_result_format("raw");

                    if (!writer->Write(grpc_result)) {
                        return Status(grpc::StatusCode::INTERNAL, "Failed to write segmentation result");
                    }
                    agent_impl_->pending_segmentation_results_.pop_front();

                    std::cout << "[AGENT] Segmentation result sent successfully" << std::endl;
                }

                return Status::OK;
            } catch (const std::exception& e) {
                std::cerr << "[AGENT] Segmentation error: " << e.what() << std::endl;
                imageservice::SegmentationResult error_result;
                error_result.set_request_id(request->image_id());
                error_result.set_status("failed");
                error_result.set_error_message(e.what());

                writer->Write(error_result);
                return Status(grpc::StatusCode::INTERNAL, "Segmentation failed: " + std::string(e.what()));
            }
        }

        Status subscribeToNotifications(ServerContext* context,
                                       ServerReaderWriter<imageservice::ServerNotification, imageservice::SubscriptionRequest>* stream) override {
            std::cout << "[AGENT] Notification subscription request received" << std::endl;

            imageservice::SubscriptionRequest request;
            while (stream->Read(&request)) {
                std::cout << "[AGENT] Client " << request.client_name() << " subscribed to topics: ";
                for (const auto& topic : request.topics()) {
                    std::cout << topic << " ";
                }
                std::cout << std::endl;

                // Send welcome notification
                imageservice::ServerNotification welcome_notification;
                welcome_notification.set_notification_id("welcome");
                welcome_notification.set_topic("system");
                welcome_notification.set_message("Welcome to ImageService notifications");
                welcome_notification.set_notification_type("info");
                welcome_notification.set_timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count());

                if (!stream->Write(welcome_notification)) {
                    return Status(grpc::StatusCode::INTERNAL, "Failed to write notification");
                }
            }

            std::cout << "[AGENT] Client disconnected from notifications" << std::endl;
            return Status::OK;
        }

    private:
        Impl* agent_impl_;
    };

    std::weak_ptr<IImageServiceListener> listener_;
    std::thread server_thread_;
    std::atomic<bool> stop_server_;

    // Segmentation results queue
    std::mutex segmentation_results_mutex_;
    std::condition_variable segmentation_results_cv_;
    std::deque<SegmentationResult> pending_segmentation_results_;
};

// Public interface implementation
ImageServiceAgent::ImageServiceAgent(std::weak_ptr<IImageServiceListener> listener)
    : mImpl(std::make_unique<Impl>(listener)) {
    std::cout << "[AGENT] ImageServiceAgent created" << std::endl;
}

ImageServiceAgent::~ImageServiceAgent() {
    std::cout << "[AGENT] ImageServiceAgent destroyed" << std::endl;
}

void ImageServiceAgent::sendSegmentationResult(const SegmentationResult& segmentation_result) {
    mImpl->sendSegmentationResult(segmentation_result);
}

} // namespace vision