// ============================================================
// [ELEC-IF] 调试可视化工具
// 发布 topic 数据给其他模块（如 UI 面板）
// ============================================================
#pragma once
#include <opencv2/opencv.hpp>
#include <string>
#include <map>
#include <variant>
#include <nlohmann/json.hpp>  // 假设已有 nlohmann/json，或用自定义 JSON

// 兼容头，不依赖第三方 JSON 库
#include <sstream>
#include <iomanip>

// 简化版 JSON 构建（不依赖 nlohmann）
class DebugPublisher {
public:
    // 发布 topic 数据
    void publish(const std::string& topic, double yaw, double pitch,
                 double distance, int id, bool locked);
    // 发布检测到的所有装甲板信息
    void publishAll(const std::string& topic,
                    const std::string& yaw,
                    const std::string& pitch,
                    const std::string& distance,
                    const std::string& id,
                    const std::string& locked);
    // 将数据序列化为 JSON 字符串
    static std::string toJSON(const std::string& topic,
                              double yaw, double pitch,
                              double distance, int id, bool locked);
};

// [ELEC-IF] 调试绘图辅助
class DebugDraw {
public:
    // 绘制装甲板轮廓及标注
    static void drawArmor(cv::Mat& img, const cv::RotatedRect& rect,
                          int id, double yaw, double pitch);
    // 绘制十字准星
    static void drawCrosshair(cv::Mat& img, int cx, int cy,
                              double yaw_deg, double pitch_deg);
    // 绘制 HUD 信息
    static void drawHUD(cv::Mat& img, double fps, int detectCnt,
                        double yaw_deg, double pitch_deg,
                        double distance_mm, bool locked);
};
