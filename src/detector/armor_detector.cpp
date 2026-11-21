// ============================================================
// 装甲板检测器实现
// 算法流程：红色区域提取 → LED筛选 → 装甲板候选 → 透视验证
// ============================================================
#include "armor_detector.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/features2d.hpp>
#include <iostream>
#include <algorithm>

ArmorDetector::ArmorDetector(const CameraInfo& camInfo,
                              const ArmorSpec&  spec)
    : camInfo_(camInfo), spec_(spec) {
    camInfo_.update();
}

void ArmorDetector::setCameraInfo(const CameraInfo& info) {
    camInfo_ = info;
    camInfo_.update();
}

std::vector<cv::Mat> ArmorDetector::extractRedRegions(const cv::Mat& bgr) {
    std::vector<cv::Mat> regions;
    if (bgr.empty() || bgr.channels() != 3) return regions;

    // 转换为 HSV
    cv::Mat hsv;
    cv::cvtColor(bgr, hsv, cv::COLOR_BGR2HSV);

    // 红色范围（HSV 中红色跨越 0° 和 180°，需要两个区间）
    // [VISION] 下限：H∈[0,10], S∈[50,255], V∈[50,255]
    // [VISION] 上限：H∈[160,180], S∈[50,255], V∈[50,255]
    cv::Mat lower1 = cv::Scalar(0, 50, 50);
    cv::Mat upper1 = cv::Scalar(10, 255, 255);
    cv::Mat lower2 = cv::Scalar(160, 50, 50);
    cv::Mat upper2 = cv::Scalar(180, 255, 255);

    cv::Mat mask1, mask2;
    cv::inRange(hsv, lower1, upper1, mask1);
    cv::inRange(hsv, lower2, upper2, mask2);

    cv::Mat mask;
    cv::bitwise_or(mask1, mask2, mask);

    // 形态学去噪
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel);
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);

    // 连通域分析
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    int W = bgr.cols, H = bgr.rows;
    double minArea = W * H * config_.minAreaRatio;
    double maxArea = W * H * config_.maxAreaRatio;

    for (const auto& cnt : contours) {
        double area = cv::contourArea(cnt);
        if (area < minArea || area > maxArea) continue;

        cv::RotatedRect rect = cv::minAreaRect(cnt);
        double aspect = rect.size.width / std::max(rect.size.height, 1.0);
        if (aspect < config_.minAspect || aspect > config_.maxAspect) continue;

        // 圆度过滤（LED 应为近似圆形）
        double perimeter = cv::arcLength(cnt, true);
        if (perimeter == 0) continue;
        double circularity = 4.0 * CV_PI * area / (perimeter * perimeter);
        if (circularity < config_.minCircularity) continue;

        // 提取 ROI
        cv::Rect br = rect.boundingRect();
        br = br & cv::Rect(0, 0, W, H);
        regions.push_back(bgr(br).clone());
    }
    return regions;
}

bool ArmorDetector::isLEDRegion(const cv::Mat& roi, const cv::Vec4i& h,
                                 double minArea, double maxArea) {
    // h = (x, y, w, h)
    if (roi.empty()) return false;
    double area = static_cast<double>(h[2]) * h[3];
    return area >= minArea && area <= maxArea;
}

std::vector<cv::Point2f> ArmorDetector::filterLEDs(
    const cv::Mat& mask,
    const std::vector<cv::Vec4i>& rects) {

    int W = mask.cols, H = mask.rows;
    double minArea = W * H * config_.minAreaRatio * 0.5;
    double maxArea = W * H * config_.maxAreaRatio * 0.5;

    std::vector<cv::Point2f> leds;
    for (const auto& r : rects) {
        if (!isLEDRegion(mask, r, minArea, maxArea)) continue;
        // 取矩形中心作为 LED 位置
        leds.push_back(cv::Point2f(r[0] + r[2]/2.0, r[1] + r[3]/2.0));
    }
    return leds;
}

std::pair<cv::Point2f, cv::Point2f> ArmorDetector::findLEDPair(
    const std::vector<cv::Point2f>& leds, size_t i) {

    if (i + 1 >= leds.size()) return {leds[i], leds[i]};
    return {leds[i], leds[i+1]};
}

std::vector<ArmorDetection> ArmorDetector::formArmorCandidates(
    const std::vector<cv::Point2f>& leds,
    const cv::Mat& bgr) {

    std::vector<ArmorDetection> candidates;
    if (leds.size() < 4) return candidates;

    int W = bgr.cols, H = bgr.rows;
    double minDist = std::min(W, H) * 0.02;
    double maxDist = std::min(W, H) * 0.5;

    // 遍历所有可能的 LED 对（上下两组各两个 LED）
    for (size_t i = 0; i + 3 < leds.size(); ++i) {
        for (size_t j = i + 1; j + 1 < leds.size(); ++j) {
            // 上排两个 LED
            cv::Point2f topL = leds[i], topR = leds[i+1];
            // 下排两个 LED
            cv::Point2f botL = leds[j], botR = leds[j+1];

            // 确保左右顺序正确
            if (topL.x > topR.x) std::swap(topL, topR);
            if (botL.x > botR.x) std::swap(botL, botR);

            // 计算水平间距
            double topGap   = cv::norm(topR - topL);
            double botGap   = cv::norm(botR - botL);
            double avgGap   = (topGap + botGap) / 2.0;

            // 验证间距是否在合理范围内（与装甲板规格对应）
            double minGap = spec_.gap * 0.5;
            double maxGap = spec_.gap * 2.0;
            if (avgGap < minGap || avgGap > maxGap) continue;

            // 验证上下对应关系
            double verticalDist = std::abs((topL.y + topR.y) / 2.0 -
                                           (botL.y + botR.y) / 2.0);
            double minH = spec_.H * 0.3;
            double maxH = spec_.H * 3.0;
            if (verticalDist < minH || verticalDist > maxH) continue;

            // 构建四个角点（顺时针：左上、右上、右下、左下）
            ArmorDetection det;
            det.points_2d.pts[0] = topL;  // 左上
            det.points_2d.pts[1] = topR;  // 右上
            det.points_2d.pts[2] = botR;  // 右下
            det.points_2d.pts[3] = botL;  // 左下

            // 计算外接旋转矩形
            std::vector<cv::Point2f> pts = {topL, topR, botR, botL};
            det.rect = cv::minAreaRect(pts);
            det.valid = true;
            det.id    = -1;
            candidates.push_back(det);
        }
    }
    return candidates;
}

std::vector<ArmorDetection> ArmorDetector::validateCandidates(
    const std::vector<ArmorDetection>& candidates,
    const cv::Mat& bgr) {

    if (candidates.empty()) return {};

    // 按面积排序，取最大的几个
    auto sorted = candidates;
    std::sort(sorted.begin(), sorted.end(),
              [](const ArmorDetection& a, const ArmorDetection& b) {
                  return a.rect.size.area() > b.rect.size.area();
              });

    // 取 Top-N（通常只有最显著的一个）
    int take = std::min(static_cast<int>(sorted.size()), 3);
    std::vector<ArmorDetection> results;
    for (int i = 0; i < take; ++i) {
        results.push_back(sorted[i]);
    }

    // [VISION] 对候选结果执行 PnP 解算验证
    // 建立 2D-3D 对应点
    std::vector<cv::Point3f> modelPts = {
        cv::Point3f(-spec_.W/2.0,  spec_.H/2.0, 0.0),  // 左上
        cv::Point3f( spec_.W/2.0,  spec_.H/2.0, 0.0),  // 右上
        cv::Point3f( spec_.W/2.0, -spec_.H/2.0, 0.0),  // 右下
        cv::Point3f(-spec_.W/2.0, -spec_.H/2.0, 0.0),  // 左下
    };

    PnPSolver solver(camInfo_);
    for (auto& det : results) {
        double yaw, pitch, dist;
        if (solver.solve(
                {det.points_2d.pts[0], det.points_2d.pts[1],
                 det.points_2d.pts[2], det.points_2d.pts[3]},
                modelPts, yaw, pitch, dist)) {
            det.yaw     = yaw;
            det.pitch   = pitch;
            det.distance = dist;
            det.valid   = true;
        } else {
            det.valid = false;
        }
    }

    // 只返回有效的检测结果
    std::vector<ArmorDetection> valid;
    for (const auto& d : results) {
        if (d.valid) valid.push_back(d);
    }
    return valid;
}

std::vector<ArmorDetection> ArmorDetector::detect(const cv::Mat& bgr) {
    if (bgr.empty()) return {};

    // Step 1: 提取红色区域
    auto redRegions = extractRedRegions(bgr);

    // Step 2: 获取所有轮廓的 bounding rect
    cv::Mat gray;
    cv::cvtColor(bgr, gray, cv::COLOR_BGR2GRAY);
    cv::Mat thresh;
    cv::threshold(gray, thresh, 0, 255, cv::THRESH_BINARY_INV + cv::THRESH_OTSU);

    std::vector<cv::Vec4i> rects;
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    for (const auto& cnt : contours) {
        cv::Rect br = cv::boundingRect(cnt);
        double area = br.area();
        double imgArea = bgr.cols * bgr.rows;
        if (area < imgArea * config_.minAreaRatio * 0.3) continue;
        rects.push_back(cv::Vec4i(br.x, br.y, br.width, br.height));
    }

    // Step 3: 筛选 LED 并组合成装甲板候选
    auto leds = filterLEDs(thresh, rects);
    auto candidates = formArmorCandidates(leds, bgr);

    // Step 4: 验证并返回最终结果
    return validateCandidates(candidates, bgr);
}
