// ============================================================
// [ELEC-IF] 串口通信接口
// 协议: 0xAA 0x55 + yaw(float LE) + pitch(float LE) + fire(uint8) + CRC8
// ============================================================
#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <functional>

class SerialPort {
public:
    SerialPort();
    ~SerialPort();

    // 打开串口, portName 如 "COM3", baudRate 如 115200
    bool open(const std::string& portName, int baudRate = 115200);
    void close();
    bool isOpen() const;

    // [ELEC-IF] 发送角度数据给电控
    // 格式: 0xAA 0x55 + yaw(f) + pitch(f) + fire(u8) + CRC8
    bool sendAngle(double yaw, double pitch, uint8_t fire = 0);

    // 设置接收回调, 收到完整帧时触发
    void setReceiveCallback(std::function<void(const std::vector<uint8_t>&)> cb);

private:
    uint8_t crc8(const uint8_t* data, size_t len);
    bool parseFrame(const std::vector<uint8_t>& buf,
                    double& yaw, double& pitch, uint8_t& fire);

    int  fd_;                 // 文件描述符 (Linux) / HANDLE (Windows)
    bool opened_;
    std::function<void(const std::vector<uint8_t>&)> rx_cb_;
};
