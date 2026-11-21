// ============================================================
// RoboMaster 装甲板识别主程序
// [VISION] 整体流程: 采集帧 → 检测装甲板 → 解算角度 → 卡尔曼平滑 → 串口发送
// [ELEC-IF] 串口发送角度给电控
// ============================================================
#include "common.h"
#include "armor_detector.h"
#include "pnp_solver.h"
#include "kalman_predictor.h"
#include "serial_port.h"
#include "camera.h"
#include "debugger.h"

#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>
#include <iomanip>

// ============================================================
// 全局配置
// ============================================================
struct AppConfig {
    int       cameraIndex  = 0;        // [ELEC-IF] 相机索引
    std::string serialPort = "COM3";   // [ELEC-IF] 串口名称
    int       baudRate     = 115200;   // [ELEC-IF] 波特率
    bool      useCalibration = false;  // [VISION] 是否使用相机标定
    double    cam_fx       = 800.0;    // [VISION] 焦距 fx (px)
    double    cam_fy       = 800.0;    // [VISION] 焦距 fy (px)
    double    cam_cx       = 640.0;    // [VISION] 主点 cx (px)
    double    cam_cy       = 360.0;    // [VISION] 主点 cy (px)
};

// ============================================================
// 主处理循环（每帧执行）
// ============================================================
void processFrame(const cv::Mat& frame,
                   const ArmorDetector& detector,
                   KalmanPredictor& predictor,
                   SerialPort& serial,
                   DebugPublisher& pub,
                   const AppConfig& cfg) {
    // [VISION] Step 1: 检测装甲板
    auto detections = detector.detect(frame);

    // [VISION] Step 2: 取最佳检测结果（距离最近的）
    double bestYaw    = 0.0;
    double bestPitch  = 0.0;
    double bestDist   = 99999.0;
    int    bestId     = -1;
    bool   locked     = false;

    if (!detections.empty()) {
        // 选择距离最近的装甲板作为目标
        const auto& best = *std::min_element(
            detections.begin(), detections.end(),
            [](const ArmorDetection& a, const ArmorDetection& b) {
                return a.distance < b.distance;
            });

        bestId    = best.id;
        bestYaw   = best.yaw;
        bestPitch = best.pitch;
        bestDist  = best.distance;

        // [VISION] Step 3: 卡尔曼平滑预测
        std::tie(bestYaw, bestPitch) = predictor.update(best.yaw, best.pitch);
        locked = true;
    } else {
        // 目标丢失，使用预测值
        std::tie(bestYaw, bestPitch) = predictor.predict();
    }

    // [ELEC-IF] Step 4: 通过串口发送角度给电控
    // 角度转换为度数，限制在合理范围
    double yawDeg  = bestYaw  * 57.2958;
    double pitchDeg = bestPitch * 57.2958;
    yawDeg  = std::clamp(yawDeg,  -45.0, 45.0);
    pitchDeg = std::clamp(pitchDeg, -30.0, 30.0);
    uint8_t fire = locked ? 1 : 0;  // [ELEC-IF] 锁定目标时才允许开火

    if (serial.isOpen()) {
        serial.sendAngle(yawDeg, pitchDeg, fire);
    }

    // [ELEC-IF] Step 5: 发布调试信息
    pub.publish("armor", bestYaw, bestPitch, bestDist, bestId, locked);

    // [VISION] Step 6: 可视化绘制
    cv::Mat disp = frame.clone();
    if (!detections.empty()) {
        const auto& best = *std::min_element(
            detections.begin(), detections.end(),
            [](const ArmorDetection& a, const ArmorDetection& b) {
                return a.distance < b.distance;
            });
        DebugDraw::drawArmor(disp, best.rect, bestId, bestYaw, bestPitch);
    }
    int cx = disp.cols / 2, cy = disp.rows / 2;
    DebugDraw::drawCrosshair(disp, cx, cy, yawDeg, pitchDeg);

    // 计算 FPS
    static auto lastTime = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    double fps = 1000.0 / std::max(1.0,
        std::chrono::duration_cast<std::chrono::microseconds>(now - lastTime).count());
    lastTime = now;

    DebugDraw::drawHUD(disp, fps, static_cast<int>(detections.size()),
                        yawDeg, pitchDeg, bestDist, locked);

    cv::imshow("RoboMaster Vision", disp);
}

// ============================================================
// 相机帧回调
// ============================================================
void onFrameCallback(cv::Mat& frame,
                      ArmorDetector& detector,
                      KalmanPredictor& predictor,
                      SerialPort& serial,
                      DebugPublisher& pub,
                      const AppConfig& cfg) {
    processFrame(frame, detector, predictor, serial, pub, cfg);
}

// ============================================================
// 主函数
// ============================================================
int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "  RoboMaster 装甲板识别系统 v1.0" << std::endl;
    std::cout << "  C++17 + OpenCV + CMake" << std::endl;
    std::cout << "========================================" << std::endl;

    // 解析命令行参数
    AppConfig cfg;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--camera" && i+1 < argc) cfg.cameraIndex = std::stoi(argv[++i]);
        else if (arg == "--serial" && i+1 < argc) cfg.serialPort = argv[++i];
        else if (arg == "--baud" && i+1 < argc) cfg.baudRate = std::stoi(argv[++i]);
        else if (arg == "--help") {
            std::cout << "Usage: romaster_vision [OPTIONS]\n"
                      << "  --camera N    相机索引 (默认 0)\n"
                      << "  --serial PORT 串口名 (默认 COM3)\n"
                      << "  --baud RATE   波特率 (默认 115200)\n"
                      << "  --help        显示帮助\n";
            return 0;
        }
    }

    // [ELEC-IF] 初始化串口
    SerialPort serial;
    std::cout << "[SERIAL] 打开串口: " << cfg.serialPort
              << " @" << cfg.baudRate << "bps ..." << std::flush;
    if (!serial.open(cfg.serialPort, cfg.baudRate)) {
        std::cout << " 失败，使用模拟模式（不发送串口）" << std::endl;
    } else {
        std::cout << " 成功" << std::endl;
    }

    // [VISION] 初始化相机参数
    CameraInfo camInfo;
    camInfo.fx = cfg.cam_fx;
    camInfo.fy = cfg.cam_fy;
    camInfo.cx = cfg.cam_cx;
    camInfo.cy = cfg.cam_cy;
    camInfo.update();

    // [VISION] 初始化检测器
    ArmorDetector detector(camInfo);

    // [VISION] 初始化卡尔曼预测器
    KalmanPredictor predictor;

    // [ELEC-IF] 初始化调试发布器
    DebugPublisher pub;

    // [ELEC-IF] 初始化相机
    CameraCapture camera;
    std::cout << "[CAMERA] 打开相机 " << cfg.cameraIndex << " ..." << std::flush;
    if (!camera.open(cfg.cameraIndex)) {
        std::cout << " 失败" << std::endl;
        std::cerr << "[CAMERA] 尝试使用视频文件模式 (test.mp4)" << std::endl;
        // 如果没有相机，可以尝试读取视频文件
        if (!camera.open("test.mp4")) {
            std::cerr << "[CAMERA] 无可用视频源，生成模拟帧进行测试" << std::endl;
            // 直接运行主循环（用空帧测试串口和调试输出）
        }
    } else {
        std::cout << " 成功 (" << camera.getResolution().width << "x"
                  << camera.getResolution().height << ", "
                  << camera.getFPS() << "fps)" << std::endl;
        camera.setOnFrameCallback(
            [&](cv::Mat& frame) { onFrameCallback(frame, detector, predictor,
                                                   serial, pub, cfg); });
    }

    // ============================================================
    // 主循环
    // ============================================================
    std::cout << "\n按 'q' 键退出\n" << std::endl;

    cv::Mat frame;
    int frameCount = 0;

    while (true) {
        if (camera.isOpen() && camera.grabFrame(frame)) {
            if (!frame.empty()) {
                processFrame(frame, detector, predictor, serial, pub, cfg);
                ++frameCount;
            }
        }

        int key = cv::waitKey(1);
        if (key == 'q' || key == 27) break;  // q 或 ESC 退出
    }

    // 清理
    camera.close();
    serial.close();
    cv::destroyAllWindows();

    std::cout << "\n[SYSTEM] 共处理 " << frameCount << " 帧，程序退出。" << std::endl;
    return 0;
}
