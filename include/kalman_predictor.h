// ============================================================
// 卡尔曼预测器
// 对装甲板角度进行时间序列平滑与下一帧预测
// 状态: [yaw, yaw_dot, pitch, pitch_dot]^T
// ============================================================
#pragma once
#include "common.h"
#include <deque>

class KalmanPredictor {
public:
    struct Config {
        double processNoise  = 0.01;  // [VISION] 过程噪声方差
        double measureNoise  = 0.05;  // [VISION] 观测噪声方差
        double initPosVar    = 1.0;   // [VISION] 初始位置方差
        double initVelVar    = 10.0;  // [VISION] 初始速度方差
        double dt            = 0.033; // [VISION] 采样周期 (s), 默认30fps
        int    maxHistory    = 10;    // [VISION] 历史缓冲长度
    };

    explicit KalmanPredictor(const Config& cfg = Config{});
    ~KalmanPredictor() = default;

    // 输入新观测，返回平滑后的状态
    // 首次调用时无历史，直接返回观测值
    std::pair<double, double> update(double yaw, double pitch);

    // 预测下一帧的角度（无新观测时使用）
    std::pair<double, double> predict();

    // 重置预测器（如目标丢失后重新初始化）
    void reset();

    // 获取历史数据（用于调试）
    const std::deque<std::pair<double,double>>& getHistory() const { return history_; }

private:
    Config  cfg_;
    cv::KalmanFilter kf_;
    cv::Mat state_;     // [yaw, yaw_dot, pitch, pitch_dot]
    cv::Mat measurement_; // [yaw, pitch]
    std::deque<std::pair<double,double>> history_;
    bool firstUpdate_ = true;
};
