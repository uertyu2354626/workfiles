// ============================================================
// 公共头文件 — RoboMaster 装甲板识别视觉系统
// ============================================================
#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <cmath>
#include <cstdint>

// ============================================================
// [VISION] 装甲板 2D 角点（图像坐标系）
// 顺序: 左上, 右上, 右下, 左下
// ============================================================
struct ArmorPoint2D {
    cv::Point2f pts[4];
};

// ============================================================
// [VISION] 装甲板 3D 角点（机器人坐标系，单位: mm）
// 相对于装甲板中心，Z 轴向外
// ============================================================
struct ArmorPoint3D {
    cv::Point3f pts[4];
};

// ============================================================
// [VISION] 单个装甲板检测结果
// ============================================================
struct ArmorDetection {
    int  id;                          // [ELEC-IF] 敌我识别码
    double yaw;                       // [ELEC-IF] 水平角度 (rad)
    double pitch;                     // [ELEC-IF] 垂直角度 (rad)
    double distance;                  // [ELEC-IF] 距离 (mm)
    ArmorPoint2D points_2d;           // 图像角点
    cv::RotatedRect rect;             // 外接旋转矩形
    cv::Mat rvec;                     // 旋转向量 (Rodrigues)
    cv::Mat tvec;                     // 平移向量
    bool   valid;                     // 是否有效
};

// ============================================================
// [VISION] 相机内参
// ============================================================
struct CameraInfo {
    double fx, fy;        // 焦距 (px)
    double cx, cy;        // 主点 (px)
    cv::Mat cameraMatrix; // 3×3 内参矩阵
    cv::Mat distCoeffs;   // 畸变系数 [k1,k2,p1,p2,k3]

    void update() {
        cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
        cameraMatrix.at<double>(0, 0) = fx;
        cameraMatrix.at<double>(0, 2) = cx;
        cameraMatrix.at<double>(1, 1) = fy;
        cameraMatrix.at<double>(1, 2) = cy;
        if (distCoeffs.empty())
            distCoeffs = cv::Mat::zeros(5, 1, CV_64F);
    }
};

// ============================================================
// [VISION] 装甲板标准 3D 尺寸（单位: mm）
// 宽 W, 高 H, 中间LED间距 gap
// ============================================================
struct ArmorSpec {
    double W;   // 总宽度
    double H;   // 高度
    double gap; // 中间LED间隙
};

// 标准 67 型装甲板尺寸
constexpr ArmorSpec DEFAULT_ARMOR_SPEC{150.0, 80.0, 10.0};

// ============================================================
// [VISION] 帧回调函数签名
// ============================================================
using FrameCallback = std::function<void(const cv::Mat& frame,
                                         const std::vector<ArmorDetection>& detections)>;
