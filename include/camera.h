// ============================================================
// [ELEC-IF] 相机驱动接口
// 回调模式: onFrame(frame) 在独立线程中调用
// ============================================================
#pragma once
#include <opencv2/opencv.hpp>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <queue>

class CameraCapture {
public:
    CameraCapture();
    ~CameraCapture();

    // 打开相机, index 为相机索引 (0,1,...) 或视频文件路径
    bool open(int index);
    bool open(const std::string& path);
    void close();
    bool isOpen() const;
    int  getFPS()  const;
    cv::Size getResolution() const;

    // 注册帧回调, 回调在相机线程中执行, 请勿阻塞
    void setOnFrameCallback(std::function<void(cv::Mat&)> callback);

    // 手动获取当前帧
    bool grabFrame(cv::Mat& out);

private:
    void captureLoop();

    int              idx_;
    cv::VideoCapture cap_;
    std::atomic<bool> running_{false};
    std::thread      thread_;
    std::function<void(cv::Mat&)> on_frame_;
    std::mutex       mtx_;
};
