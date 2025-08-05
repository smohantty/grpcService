#include "RayVision.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using rayvisiongrpc::RayVisionGrpc;
using rayvisiongrpc::GetImageRequest;
using rayvisiongrpc::ImageData;
using rayvisiongrpc::SegmentationResult;
using rayvisiongrpc::CameraType;
using rayvisiongrpc::Empty;

class RayVisionClient {
public:
    RayVisionClient(std::shared_ptr<Channel> channel)
        : stub_(RayVisionGrpc::NewStub(channel)) {}

    void GetImage(int cameraType) {
        GetImageRequest request;
        request.set_type(static_cast<CameraType>(cameraType));

        ImageData response;
        ClientContext context;

        Status status = stub_->GetImage(&context, request, &response);

        if (status.ok()) {
            std::cout << "GetImage successful:" << std::endl;
            std::cout << "  Width: " << response.width() << std::endl;
            std::cout << "  Height: " << response.height() << std::endl;
            std::cout << "  Colorspace: " << response.colorspace() << std::endl;
            std::cout << "  Buffer size: " << response.buffer().size() << " bytes" << std::endl;
        } else {
            std::cout << "GetImage failed: " << status.error_message() << std::endl;
        }
    }

    void DoSegmentation() {
        Empty request;
        ClientContext context;

        std::unique_ptr<grpc::ClientReader<SegmentationResult>> reader(
            stub_->doSegmentation(&context, request));

        SegmentationResult response;
        while (reader->Read(&response)) {
            std::cout << "Segmentation result received:" << std::endl;
            std::cout << "  Number of segments: " << response.segments_size() << std::endl;

            for (int i = 0; i < response.segments_size(); ++i) {
                const auto& segment = response.segments(i);
                std::cout << "  Segment " << i << ":" << std::endl;
                std::cout << "    Width: " << segment.width() << std::endl;
                std::cout << "    Height: " << segment.height() << std::endl;
                std::cout << "    Colorspace: " << segment.colorspace() << std::endl;
                std::cout << "    Buffer size: " << segment.buffer().size() << " bytes" << std::endl;
            }
        }

        Status status = reader->Finish();
        if (!status.ok()) {
            std::cout << "Segmentation failed: " << status.error_message() << std::endl;
        }
    }

private:
    std::unique_ptr<RayVisionGrpc::Stub> stub_;
};

int main() {
    std::string target_address("unix:///tmp/rayvision_service.sock");
    auto channel = grpc::CreateChannel(target_address, grpc::InsecureChannelCredentials());
    RayVisionClient client(channel);

    std::cout << "Testing GetImage for HEAD camera..." << std::endl;
    client.GetImage(1); // HEAD camera

    std::cout << "\nTesting GetImage for BODY camera..." << std::endl;
    client.GetImage(2); // BODY camera

    std::cout << "\nTesting doSegmentation..." << std::endl;
    client.DoSegmentation();

    return 0;
}