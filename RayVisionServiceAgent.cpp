#include "RayVisionServiceAgent.h"
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include "RayVision.grpc.pb.h"
#include <iostream>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <set>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/xattr.h>
#include <cstring>
#include <cerrno>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::CallbackServerContext;
using grpc::ServerWriter;
using grpc::Status;
using grpc::ServerUnaryReactor;
using grpc::ServerWriteReactor;
using rayvisiongrpc::RayVisionGrpc;
using rayvisiongrpc::GetImageRequest;
using rayvisiongrpc::ImageData;
using rayvisiongrpc::SegmentationResult;
using rayvisiongrpc::CameraType;
using rayvisiongrpc::ColorSpace;

namespace rayvision {

class RayVisionServiceAgent::Impl {
private:
    // Forward declaration of nested class
    class DoSegmentationReactor;

public:
    Impl(std::weak_ptr<IRayVisionServiceListener> listener)
        : mListener(listener), mStopServer(false) {
        startServer();
    }

    ~Impl() {
        stopServer();
    }

    void sendSegmentationResult(const rayvision::SegmentationResult& segmentation_result) {
        // Process the result immediately using gRPC's thread pool
        std::cout << "[RAYVISION] Processing segmentation result with "
                  << segmentation_result.segments.size() << " segments" << std::endl;

        // Send the result to any waiting clients
        std::lock_guard<std::mutex> lock(mSegmentationReactorsMutex);
        for (auto* reactor_ptr : mActiveSegmentationReactors) {
            auto* reactor = static_cast<DoSegmentationReactor*>(reactor_ptr);
            if (reactor && !reactor->IsFinished()) {
                // Convert to gRPC response
                rayvisiongrpc::SegmentationResult grpc_result;
                for (const auto& segment : segmentation_result.segments) {
                    auto* grpc_segment = grpc_result.add_segments();
                    grpc_segment->set_width(segment.width);
                    grpc_segment->set_height(segment.height);
                    grpc_segment->set_colorspace(static_cast<rayvisiongrpc::ColorSpace>(segment.colorspace));
                    grpc_segment->set_buffer(segment.buffer);
                }

                reactor->StartWrite(&grpc_result);
            }
        }
    }

    void registerSegmentationReactor(DoSegmentationReactor* reactor) {
        std::lock_guard<std::mutex> lock(mSegmentationReactorsMutex);
        mActiveSegmentationReactors.insert(static_cast<void*>(reactor));
    }

    void unregisterSegmentationReactor(DoSegmentationReactor* reactor) {
        std::lock_guard<std::mutex> lock(mSegmentationReactorsMutex);
        mActiveSegmentationReactors.erase(static_cast<void*>(reactor));
    }

private:
    void startServer() {
        mServerThread = std::thread([this]() {
            std::string server_address("unix:///tmp/rayvision_service.sock"); // Unix socket path
            std::string socket_path("/tmp/rayvision_service.sock");

            // Clean up any existing socket first
            unlink(socket_path.c_str());

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
                std::lock_guard<std::mutex> lock(mServerMutex);
                mServer = builder.BuildAndStart();
            }

            if (mServer) {
                // Set socket permissions to allow all users to connect
                if (chmod(socket_path.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == 0) {
                    std::cout << "[RAYVISION] Socket permissions set to 777" << std::endl;
                } else {
                    std::cerr << "[RAYVISION] Warning: Failed to set socket permissions: " << strerror(errno) << std::endl;
                }

                // Set SMACK label for Tizen using file attributes
                // This is the proper way to set SMACK labels in Tizen
                const char* smack_label = "_";
                if (setxattr(socket_path.c_str(), "security.SMACK64", smack_label, strlen(smack_label), 0) == 0) {
                    std::cout << "[RAYVISION] SMACK label set to: " << smack_label << std::endl;
                } else {
                    std::cerr << "[RAYVISION] Warning: Failed to set SMACK label: " << strerror(errno) << std::endl;
                    std::cerr << "[RAYVISION] This is normal if SMACK is not enabled or not available" << std::endl;
                }

                std::cout << "[RAYVISION] RayVisionServiceAgent server listening on " << server_address << std::endl;
                std::cout << "[RAYVISION] Socket path: " << socket_path << " with permissions 777" << std::endl;

                // Wait for server to shutdown
                mServer->Wait();
            } else {
                std::cerr << "[RAYVISION] Failed to start server" << std::endl;
            }
        });
    }

    void stopServer() {
        mStopServer = true;

        // Clean up Unix socket
        if (unlink("/tmp/rayvision_service.sock") == 0) {
            std::cout << "[RAYVISION] Unix socket cleaned up" << std::endl;
        }

        // Shutdown the server
        {
            std::lock_guard<std::mutex> lock(mServerMutex);
            if (mServer) {
                std::cout << "[RAYVISION] Shutting down server..." << std::endl;
                mServer->Shutdown();
            }
        }

        if (mServerThread.joinable()) {
            mServerThread.join();
        }

        // Clear server reference
        {
            std::lock_guard<std::mutex> lock(mServerMutex);
            mServer.reset();
        }
    }

    // gRPC Service Implementation
    class GetImageReactor : public grpc::ServerUnaryReactor {
    public:
        GetImageReactor(Impl* agent_impl, const GetImageRequest* request, rayvisiongrpc::ImageData* response)
            : agent_impl_(agent_impl), request_(request), response_(response) {
            // Start processing in background
            StartProcessing();
        }

        void StartProcessing() {
            // Process the request asynchronously
            try {
                auto listener = agent_impl_->mListener.lock();
                if (!listener) {
                    response_->set_width(0);
                    response_->set_height(0);
                    response_->set_colorspace(rayvisiongrpc::ColorSpace::RGB);
                    response_->set_buffer("");
                    Finish(grpc::Status(grpc::StatusCode::INTERNAL, "Listener not available"));
                    return;
                }

                // Call listener to get image data
                auto image_data = listener->onGetImage(request_->type());

                // Convert to gRPC response
                response_->set_width(image_data.width);
                response_->set_height(image_data.height);
                response_->set_colorspace(static_cast<rayvisiongrpc::ColorSpace>(image_data.colorspace));
                response_->set_buffer(image_data.buffer);

                std::cout << "[RAYVISION] GetImage response prepared (size: " << response_->buffer().size() << " bytes)" << std::endl;
                Finish(grpc::Status::OK);
            } catch (const std::exception& e) {
                std::cerr << "[RAYVISION] GetImage error: " << e.what() << std::endl;
                response_->set_width(0);
                response_->set_height(0);
                response_->set_colorspace(rayvisiongrpc::ColorSpace::RGB);
                response_->set_buffer("");
                Finish(grpc::Status(grpc::StatusCode::INTERNAL, "Failed to get image: " + std::string(e.what())));
            }
        }

        void OnDone() override {
            // Cleanup when the reactor is done
            delete this;
        }

    private:
        Impl* agent_impl_;
        const GetImageRequest* request_;
        rayvisiongrpc::ImageData* response_;
    };

    class DoSegmentationReactor : public grpc::ServerWriteReactor<rayvisiongrpc::SegmentationResult> {
    public:
        DoSegmentationReactor(Impl* agent_impl, const rayvisiongrpc::Empty* request)
            : agent_impl_(agent_impl), request_(request), finished_(false) {
            // Register this reactor with the agent
            agent_impl_->registerSegmentationReactor(this);

            // Start processing in background
            StartProcessing();
        }

        void StartProcessing() {
            // Check if server is shutting down
            if (agent_impl_->mStopServer) {
                Finish(grpc::Status(grpc::StatusCode::CANCELLED, "Server shutting down"));
                return;
            }

            auto listener = agent_impl_->mListener.lock();
            if (!listener) {
                Finish(grpc::Status(grpc::StatusCode::INTERNAL, "Listener not available"));
                return;
            }

            try {
                // Notify the listener about the segmentation request
                // The listener should process this asynchronously and call sendSegmentationResult when ready
                listener->onDoSegmentation();

                std::cout << "[RAYVISION] Segmentation request notified to listener" << std::endl;

                // Don't finish here - wait for sendSegmentationResult to be called
                // The reactor will stay active until the result is ready

            } catch (const std::exception& e) {
                std::cerr << "[RAYVISION] Segmentation error: " << e.what() << std::endl;
                Finish(grpc::Status(grpc::StatusCode::INTERNAL, "Segmentation failed: " + std::string(e.what())));
            }
        }

        void OnDone() override {
            // Cleanup when the reactor is done
            agent_impl_->unregisterSegmentationReactor(this);
            delete this;
        }

        void OnWriteDone(bool ok) override {
            if (ok) {
                std::cout << "[RAYVISION] Segmentation result sent successfully" << std::endl;
                finished_ = true;
                Finish(grpc::Status::OK);
            } else {
                Finish(grpc::Status(grpc::StatusCode::INTERNAL, "Failed to write segmentation result"));
            }
        }

        bool IsFinished() const { return finished_; }

    private:
        Impl* agent_impl_;
        const rayvisiongrpc::Empty* request_;
        bool finished_;
    };

    class RayVisionServiceImpl final : public RayVisionGrpc::CallbackService {
    public:
        RayVisionServiceImpl(Impl* agent_impl) : agent_impl_(agent_impl) {}

                ServerUnaryReactor* GetImage(CallbackServerContext* context, const GetImageRequest* request,
                       rayvisiongrpc::ImageData* response) override {
            std::cout << "[RAYVISION] GetImage request received for camera type: " << request->type() << std::endl;

            return new GetImageReactor(agent_impl_, request, response);
        }

                ServerWriteReactor<rayvisiongrpc::SegmentationResult>* doSegmentation(CallbackServerContext* context, const rayvisiongrpc::Empty* request) override {
            std::cout << "[RAYVISION] doSegmentation request received" << std::endl;

            return new DoSegmentationReactor(agent_impl_, request);
        }

    private:
        Impl* agent_impl_;
    };

    std::weak_ptr<IRayVisionServiceListener> mListener;
    std::thread mServerThread;
    std::atomic<bool> mStopServer;
    std::unique_ptr<Server> mServer; // Store server reference for shutdown
    std::mutex mServerMutex; // Protect server access
    std::mutex mSegmentationReactorsMutex; // Protect active segmentation reactors
            std::set<void*> mActiveSegmentationReactors; // Track active segmentation requests
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