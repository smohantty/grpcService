#pragma once
#include <memory>
#include <string>

namespace vision {
struct ImageData {
    std::string image_data;
    std::string image_type;
};

struct SegmentationResult {
    std::string segmentation_result;
};

class ImageServiceAgent {
public:
    class IImageServiceListener {
    public:
        virtual ~IImageServiceListener() = default;
        virtual void onDoSegmentation() = 0;
        virtual ImageData onGetImage() = 0;
    };

    ImageServiceAgent(std::weak_ptr<IImageServiceListener> listener);
    ~ImageServiceAgent();

    void sendSegmentationResult(const SegmentationResult& segmentation_result);

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};
} // namespace vision