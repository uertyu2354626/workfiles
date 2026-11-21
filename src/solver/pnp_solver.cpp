// ============================================================
// PnP 角度解算器实现
// 使用 cv::solvePnP 求取位姿，转换为欧拉角
// ============================================================
#include "pnp_solver.h"
#include <opencv2/calib3d.hpp>
#include <iostream>

PnPSolver::PnPSolver(const CameraInfo& camInfo)
    : camInfo_(camInfo) {
    camInfo_.update();

    // 初始化 3D 模型点（以装甲板中心为原点）
    double W = camInfo_.fx > 0 ? 150.0 : 150.0;  // [VISION] 装甲板宽度 mm
    double H = camInfo_.fy > 0 ? 80.0  : 80.0;   // [VISION] 装甲板高度 mm
    modelPoints_ = {
        cv::Point3f(-W/2.0,  H/2.0, 0.0),  // 左上
        cv::Point3f( W/2.0,  H/2.0, 0.0),  // 右上
        cv::Point3f( W/2.0, -H/2.0, 0.0),  // 右下
        cv::Point3f(-W/2.0, -H/2.0, 0.0),  // 左下
    };
}

void PnPSolver::setCameraInfo(const CameraInfo& info) {
    camInfo_ = info;
    camInfo_.update();
}

bool PnPSolver::solve(const std::vector<cv::Point2f>& pts2d,
                       const std::vector<cv::Point3f>& pts3d,
                       double& yaw, double& pitch, double& distance) {
    if (pts2d.size() < 4 || pts3d.size() < 4) return false;
    if (camInfo_.fx == 0) return false;

    cv::Mat rvec, tvec;
    // [VISION] 使用 solvePnP 求解位姿（EPNP 算法，无需迭代初值）
    bool ok = cv::solvePnP(pts3d, pts2d,
                           camInfo_.cameraMatrix,
                           camInfo_.distCoeffs,
                           rvec, tvec,
                           false, cv::SOLVEPNP_EPNP);
    if (!ok) return false;

    // 将旋转向量转换为旋转矩阵
    cv::Mat R;
    cv::Rodrigues(rvec, R);

    // 从旋转矩阵提取欧拉角（ZYX 顺序：yaw → pitch → roll）
    // pitch = atan2(-R[2,0], sqrt(R[2,1]^2 + R[2,2]^2))
    // yaw   = atan2(R[1,0],  R[0,0])
    pitch = std::atan2(-R.at<double>(2, 0),
                        std::sqrt(R.at<double>(2,1)*R.at<double>(2,1) +
                                  R.at<double>(2,2)*R.at<double>(2,2)));
    yaw = std::atan2(R.at<double>(1, 0),
                      R.at<double>(0, 0));

    // 计算距离（tvec 的 Z 分量，单位 mm）
    distance = std::abs(tvec.at<double>(2, 0));

    return true;
}

void PnPSolver::updateDetection(ArmorDetection& det) {
    if (!det.valid) return;

    std::vector<cv::Point2f> pts2d = {
        det.points_2d.pts[0], det.points_2d.pts[1],
        det.points_2d.pts[2], det.points_2d.pts[3]
    };

    double yaw, pitch, dist;
    if (solve(pts2d, modelPoints_, yaw, pitch, dist)) {
        det.yaw     = yaw;
        det.pitch   = pitch;
        det.distance = dist;
    }
}
