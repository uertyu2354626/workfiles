// ============================================================
// 装甲板检测器 — 基于颜色+形状+透视的识别算法
// ============================================================
#pragma once
#include "common.h"
#include <vector>

class ArmorDetector {
public:
    // 参数配置
    struct Config {
        double minAreaRatio     = 0.005;  // [VISION] 最小面积比例
        double maxAreaRatio     = 0.15;   // [VISION] 最大面积比例
        double minAspect        = 0.8;    // [VISION] 最小宽高比
        double maxAspect        = 3.0;    // [VISION] 最大宽高比
        double minCircularity   = 0.3;    // [VISION] 最小圆度（LED筛选）
        double ledGapRatioMin   = 0.05;   // [VISION] LED间隙比例下限
        double ledGapRatioMax   = 0.5;    // [VISION] LED间隙比例上限
        double minRectAngle     = 30.0;   // [VISION] 最小旋转角度（度）
        double maxRectAngle     = 60.0;   // [VISION] 最大旋转角度（度）
        int    minLEDCount      = 8;      // [VISION] 最少LED数量
        int    maxLEDCount      = 20;     // [VISION] 最多LED数量
    };

    explicit ArmorDetector(const CameraInfo& camInfo,
                           const ArmorSpec&  spec = DEFAULT_ARMOR_SPEC);
    ~ArmorDetector() = default;

    // 检测装甲板，返回检测结果列表
    std::vector<ArmorDetection> detect(const cv::Mat& bgr);

    // 更新相机参数
    void setCameraInfo(const CameraInfo& info);
    const Config& getConfig() const { return config_; }
    void setConfig(const Config& cfg) { config_ = cfg; }

private:
    // 步骤1: 红色LED区域提取
    std::vector<cv::Mat> extractRedRegions(const cv::Mat& bgr);
    // 步骤2: 筛选符合条件的LED连通域
    std::vector<cv::Point2f> filterLEDs(const cv::Mat& mask,
                                        const std::vector<cv::Vec4i>& contours);
    // 步骤3: 组合LED对形成装甲板候选
    std::vector<ArmorDetection> formArmorCandidates(
        const std::vector<cv::Point2f>& leds,
        const cv::Mat& bgr);
    // 步骤4: 验证候选并筛选最佳结果
    std::vector<ArmorDetection> validateCandidates(
        const std::vector<ArmorDetection>& candidates,
        const cv::Mat& bgr);
    // 计算LED的几何特征
    bool isLEDRegion(const cv::Mat& roi, const cv::Vec4i& h,
                     double minArea, double maxArea);
    // 寻找LED对
    std::pair<cv::Point2f, cv::Point2f> findLEDPair(
        const std::vector<cv::Point2f>& leds, size_t i);

    CameraInfo camInfo_;
    ArmorSpec  spec_;
    Config     config_;
};
