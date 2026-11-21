// ============================================================
// [ELEC-IF] 相机驱动实现
// ============================================================
#include "camera.h"
#include <iostream>

CameraCapture::CameraCapture() : idx_(-1) {}

CameraCapture::~CameraCapture() { close(); }

bool CameraCapture::open(int index) {
    idx_ = index;
    cap_.open(index);
    if (!cap_.isOpened()) {
        std::cerr << "[CAMERA] 无法打开相机 " << index << std::endl;
        return false;
    }
    running_.store(true);
    thread_ = std::thread(&CameraCapture::captureLoop, this);
    return true;
}

bool CameraCapture::open(const std::string& path) {
    cap_.open(path);
    if (!cap_.isOpened()) {
        std::cerr << "[CAMERA] 无法打开视频文件: " << path << std::endl;
        return false;
    }
    running_.store(true);
    thread_ = std::thread(&CameraCapture::captureLoop, this);
    return true;
}

void CameraCapture::close() {
    running_.store(false);
    if (thread_.joinable()) thread_.join();
    cap_.release();
}

bool CameraCapture::isOpen() const { return cap_.isOpened(); }

int CameraCapture::getFPS() const {
    return static_cast<int>(cap_.get(cv::CAP_PROP_FPS));
}

cv::Size CameraCapture::getResolution() const {
    return cv::Size(
        static_cast<int>(cap_.get(cv::CAP_PROP_FRAME_WIDTH)),
        static_cast<int>(cap_.get(cv::CAP_PROP_FRAME_HEIGHT))
    );
}

void CameraCapture::setOnFrameCallback(std::function<void(cv::Mat&)> callback) {
    std::lock_guard<std::mutex> lock(mtx_);
    on_frame_ = std::move(callback);
}

bool CameraCapture::grabFrame(cv::Mat& out) {
    return cap_.read(out);
}

void CameraCapture::captureLoop() {
    cv::Mat frame;
    while (running_.load()) {
        if (!cap_.read(frame) || frame.empty()) continue;
        std::lock_guard<std::mutex> lock(mtx_);
        if (on_frame_) on_frame_(frame);
    }
}
