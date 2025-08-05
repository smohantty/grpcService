#pragma once
#include <memory>
#include <string>
#include <vector>

namespace rayvision {

struct ImageData {
    int width;
    int height;
    int colorspace; // 1 = RGB, 2 = GRAY
    std::string buffer;
};

struct SegmentationResult {
    std::vector<ImageData> segments;
};

class RayVisionServiceAgent {
public:
    class IRayVisionServiceListener {
    public:
        virtual ~IRayVisionServiceListener() = default;
        virtual ImageData onGetImage(int cameraType) = 0; // 1 = HEAD, 2 = BODY, 3 = IR
        virtual void onDoSegmentation() = 0; // Notify segmentation request
    };

    RayVisionServiceAgent(std::weak_ptr<IRayVisionServiceListener> listener);
    ~RayVisionServiceAgent();

    void sendSegmentationResult(const SegmentationResult& segmentation_result);

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

} // namespace rayvision