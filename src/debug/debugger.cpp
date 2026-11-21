// ============================================================
// [ELEC-IF] 调试可视化实现
// ============================================================
#include "debugger.h"
#include <sstream>
#include <iomanip>
#include <iostream>

// ---------- DebugPublisher 实现 ----------

void DebugPublisher::publish(const std::string& topic,
                              double yaw, double pitch,
                              double distance, int id, bool locked) {
    std::string json = toJSON(topic, yaw, pitch, distance, id, locked);
    std::cout << "[" << topic << "] " << json << std::endl;
    // 实际项目中可在此调用 publish() 发送给 UI 或 ROS
    (void)json; // 避免 unused 警告
}

void DebugPublisher::publishAll(const std::string& topic,
                                 const std::string& yaw,
                                 const std::string& pitch,
                                 const std::string& distance,
                                 const std::string& id,
                                 const std::string& locked) {
    std::ostringstream oss;
    oss << "{\"topic\":\"" << topic << "\","
        << "\"yaw\":" << yaw << ","
        << "\"pitch\":" << pitch << ","
        << "\"distance\":" << distance << ","
        << "\"id\":" << id << ","
        << "\"locked\":" << (locked ? "true" : "false") << "}";
    std::cout << oss.str() << std::endl;
}

std::string DebugPublisher::toJSON(const std::string& topic,
                                    double yaw, double pitch,
                                    double distance, int id, bool locked) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4);
    oss << "{"
        << "\"topic\":\"" << topic << "\","
        << "\"yaw\":"    << yaw    << ","
        << "\"pitch\":"  << pitch  << ","
        << "\"distance\":" << distance << ","
        << "\"id\":"     << id     << ","
        << "\"locked\":" << (locked ? "true" : "false")
        << "}";
    return oss.str();
}

// ---------- DebugDraw 实现 ----------

void DebugDraw::drawArmor(cv::Mat& img, const cv::RotatedRect& rect,
                           int id, double yaw, double pitch) {
    cv::Point2f vertices[4];
    rect.points(vertices);
    for (int i = 0; i < 4; ++i)
        cv::line(img, vertices[i], vertices[(i+1)%4],
                 cv::Scalar(0, 255, 0), 2);

    // 标注 ID 和角度
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1)
       << "ID:" << id
       << " Y:" << yaw   * 57.2958
       << " P:" << pitch * 57.2958;
    cv::putText(img, ss.str(),
                cv::Point2f(rect.center.x, rect.center.y - 10),
                cv::FONT_HERSHEY_SIMPLEX, 0.6,
                cv::Scalar(0, 255, 0), 2);
}

void DebugDraw::drawCrosshair(cv::Mat& img, int cx, int cy,
                               double yaw_deg, double pitch_deg) {
    int len = 20;
    cv::line(img, cv::Point(cx - len, cy), cv::Point(cx + len, cy),
             cv::Scalar(0, 255, 255), 1);
    cv::line(img, cv::Point(cx, cy - len), cv::Point(cx, cy + len),
             cv::Scalar(0, 255, 255), 1);
    cv::circle(img, cv::Point(cx, cy), 3, cv::Scalar(0, 255, 255), -1);

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1)
       << "Y:" << yaw_deg << " P:" << pitch_deg;
    cv::putText(img, ss.str(),
                cv::Point(cx + 10, cy - 10),
                cv::FONT_HERSHEY_SIMPLEX, 0.5,
                cv::Scalar(0, 255, 255), 1);
}

void DebugDraw::drawHUD(cv::Mat& img, double fps, int detectCnt,
                         double yaw_deg, double pitch_deg,
                         double distance_mm, bool locked) {
    std::ostringstream hud;
    hud << std::fixed << std::setprecision(1)
        << "FPS: "      << fps << "\n"
        << "Targets: "  << detectCnt << "\n"
        << "Yaw:   "    << yaw_deg << " deg\n"
        << "Pitch: "    << pitch_deg << " deg\n"
        << "Dist:  "    << distance_mm << " mm\n"
        << "Lock:  "    << (locked ? "YES" : "NO");

    cv::putText(img, hud.str(),
                cv::Point(10, 20),
                cv::FONT_HERSHEY_SIMPLEX, 0.55,
                cv::Scalar(0, 255, 0), 1);
}
