// ============================================================
// PnP 角度解算器
// 使用 cv::solvePnP 求位姿，转换为基础欧拉角 yaw/pitch
// ============================================================
#pragma once
#include "common.h"
#include <vector>

class PnPSolver {
public:
    explicit PnPSolver(const CameraInfo& camInfo);
    ~PnPSolver() = default;

    // 根据2D角点和3D模型点解算位姿
    // 输入: 4个对应点 (order: 左上, 右上, 右下, 左下)
    // 输出: yaw, pitch (rad), distance (mm)
    bool solve(const std::vector<cv::Point2f>& pts2d,
               const std::vector<cv::Point3f>& pts3d,
               double& yaw, double& pitch, double& distance);

    // 直接从 ArmorDetection 更新 rvec/tvec
    void updateDetection(ArmorDetection& det);

    const CameraInfo& getCameraInfo() const { return camInfo_; }
    void setCameraInfo(const CameraInfo& info);

private:
    CameraInfo camInfo_;
    // 预计算的3D模型点（以装甲板中心为原点）
    std::vector<cv::Point3f> modelPoints_;
};
