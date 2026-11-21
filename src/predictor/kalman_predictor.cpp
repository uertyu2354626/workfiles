// ============================================================
// 卡尔曼预测器实现
// 状态空间: x = [yaw, yaw_dot, pitch, pitch_dot]^T
// 观测: z = [yaw, pitch]^T
// ============================================================
#include "kalman_predictor.h"
#include <iostream>

KalmanPredictor::KalmanPredictor(const Config& cfg)
    : cfg_(cfg), firstUpdate_(true) {

    // 初始化卡尔曼滤波器
    kf_.create(4, 2, CV_64F);  // 4 维状态, 2 维观测

    // 状态转移矩阵 F (匀速运动模型)
    // [1 dt 0  0 ]
    // [0 1  0  0 ]
    // [0 0  1  dt]
    // [0 0  0  1 ]
    kf_.transitionMatrix = cv::Mat::eye(4, 4, CV_64F);
    kf_.transitionMatrix.at<double>(0, 1) = cfg_.dt;
    kf_.transitionMatrix.at<double>(2, 3) = cfg_.dt;

    // 观测矩阵 H = [1 0 0 0; 0 0 1 0]
    kf_.measurementMatrix = cv::Mat::eye(2, 4, CV_64F);
    kf_.measurementMatrix.at<double>(1, 2) = 1.0;

    // 过程噪声协方差 Q
    kf_.processNoiseCov = cv::Mat::eye(4, 4, CV_64F) * cfg_.processNoise;

    // 观测噪声协方差 R
    kf_.measurementNoiseCov = cv::Mat::eye(2, 2, CV_64F) * cfg_.measureNoise;

    // 初始状态方差 P
    kf_.errorCovPost = cv::Mat::eye(4, 4, CV_64F);
    kf_.errorCovPost.at<double>(0, 0) = cfg_.initPosVar;   // yaw 方差
    kf_.errorCovPost.at<double>(1, 1) = cfg_.initVelVar;   // yaw_dot 方差
    kf_.errorCovPost.at<double>(2, 2) = cfg_.initPosVar;   // pitch 方差
    kf_.errorCovPost.at<double>(3, 3) = cfg_.initVelVar;   // pitch_dot 方差

    // 初始状态为零
    state_ = cv::Mat::zeros(4, 1, CV_64F);
    measurement_ = cv::Mat::zeros(2, 1, CV_64F);
}

std::pair<double, double> KalmanPredictor::update(double yaw, double pitch) {
    // 第一次更新直接返回观测值
    if (firstUpdate_) {
        state_.at<double>(0) = yaw;
        state_.at<double>(2) = pitch;
        measurement_.at<double>(0) = yaw;
        measurement_.at<double>(1) = pitch;
        firstUpdate_ = false;
        history_.clear();
        history_.emplace_back(yaw, pitch);
        return {yaw, pitch};
    }

    // 预测步骤
    cv::Mat predicted = kf_.predict();

    // 更新步骤
    measurement_.at<double>(0) = yaw;
    measurement_.at<double>(1) = pitch;
    cv::Mat corrected = kf_.correct(measurement_);

    double estYaw    = corrected.at<double>(0);
    double estPitch  = corrected.at<double>(1);

    history_.emplace_back(estYaw, estPitch);
    if (static_cast<int>(history_.size()) > cfg_.maxHistory)
        history_.pop_front();

    return {estYaw, estPitch};
}

std::pair<double, double> KalmanPredictor::predict() {
    cv::Mat pred = kf_.predict();
    return {pred.at<double>(0), pred.at<double>(1)};
}

void KalmanPredictor::reset() {
    state_ = cv::Mat::zeros(4, 1, CV_64F);
    measurement_ = cv::Mat::zeros(2, 1, CV_64F);
    kf_.errorCovPost = cv::Mat::eye(4, 4, CV_64F);
    kf_.errorCovPost.at<double>(0, 0) = cfg_.initPosVar;
    kf_.errorCovPost.at<double>(1, 1) = cfg_.initVelVar;
    kf_.errorCovPost.at<double>(2, 2) = cfg_.initPosVar;
    kf_.errorCovPost.at<double>(3, 3) = cfg_.initVelVar;
    history_.clear();
    firstUpdate_ = true;
}
