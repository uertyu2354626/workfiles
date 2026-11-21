// ============================================================
// [ELEC-IF] 串口通信实现 — CRC8 + 协议封装
// 协议: 0xAA 0x55 + yaw(float LE) + pitch(float LE) + fire(uint8) + CRC8
// ============================================================
#include "serial_port.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

// 仅包含平台无关部分；Windows 下用 WinSock，Linux 下用 termios
#ifdef _WIN32
    #include <windows.h>
#else
    #include <fcntl.h>
    #include <unistd.h>
    #include <termios.h>
#endif

SerialPort::SerialPort() : fd_(-1), opened_(false) {}

SerialPort::~SerialPort() { close(); }

bool SerialPort::open(const std::string& portName, int baudRate) {
#ifdef _WIN32
    // Windows: 使用 CreateFile 打开 COM 端口
    std::wstring wport = L"\\\\.\\\" + portName;
    HANDLE h = CreateFileW(wport.c_str(),
                           GENERIC_READ | GENERIC_WRITE,
                           0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[SERIAL] 无法打开串口 %s\n", portName.c_str());
        return false;
    }
    fd_ = reinterpret_cast<int>(h);

    DCB dcb = {0};
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(h, &dcb)) { CloseHandle(h); return false; }
    dcb.BaudRate = baudRate;
    dcb.ByteSize = 8;
    dcb.Parity   = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    if (!SetCommState(h, &dcb)) { CloseHandle(h); return false; }

    COMMTIMEOUTS to = {0};
    to.ReadIntervalTimeout         = 10;
    to.ReadTotalTimeoutConstant    = 50;
    to.ReadTotalTimeoutMultiplier  = 10;
    to.WriteTotalTimeoutConstant   = 50;
    to.WriteTotalTimeoutMultiplier = 10;
    SetCommTimeouts(h, &to);
    opened_ = true;
    return true;
#else
    // Linux: 使用 termios 配置串口
    fd_ = open(portName.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd_ < 0) {
        perror("[SERIAL] open");
        return false;
    }

    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd_, &tty) != 0) { close(fd_); return false; }

    cfsetispeed(&tty, baudRate);
    cfsetospeed(&tty, baudRate);
    tty.c_cflag = (BARE|PARENB|CLOCAL|CREAD);
    tty.c_cflag &= ~PARENB;   // 无校验
    tty.c_cflag &= ~CSTOPB;   // 1位停止位
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;       // 8位数据位
    tty.c_iflag = 0;
    tty.c_oflag = 0;
    tty.c_lflag = 0;          // 非canonical
    tty.c_cc[VMIN]  = 1;
    tty.c_cc[VTIME] = 5;

    if (tcsetattr(fd_, TCSANOW, &tty) != 0) { close(fd_); return false; }
    tcflush(fd_, TCIOFLUSH);
    opened_ = true;
    return true;
#endif
}

void SerialPort::close() {
    if (!opened_) return;
#ifdef _WIN32
    CloseHandle(reinterpret_cast<void*>(fd_));
#else
    if (fd_ >= 0) close(fd_);
#endif
    fd_ = -1;
    opened_ = false;
}

bool SerialPort::isOpen() const { return opened_; }

// CRC-8/MAXIM 多项式: x^8 + x^5 + x^4 + 1  (0x31)
uint8_t SerialPort::crc8(const uint8_t* data, size_t len) {
    uint8_t crc = 0;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x80)
                crc = static_cast<uint8_t>((crc << 1) ^ 0x31);
            else
                crc = static_cast<uint8_t>(crc << 1);
        }
    }
    return crc;
}

bool SerialPort::sendAngle(double yaw, double pitch, uint8_t fire) {
    if (!opened_) return false;

    // 构建 payload: yaw(4B) + pitch(4B) + fire(1B) = 9B
    uint8_t payload[9];
    union { float f; uint8_t b[4]; } u;

    u.f = static_cast<float>(yaw);
    std::copy(std::begin(u.b), std::end(u.b), payload + 0);

    u.f = static_cast<float>(pitch);
    std::copy(std::begin(u.b), std::end(u.b), payload + 4);

    payload[8] = fire;

    // 完整帧: 0xAA 0x55 + payload + CRC8
    uint8_t frame[12];
    frame[0] = 0xAA;
    frame[1] = 0x55;
    std::copy(std::begin(payload), std::end(payload), frame + 2);
    frame[11] = crc8(frame + 2, 9);

#ifdef _WIN32
    DWORD written = 0;
    BOOL ok = WriteFile(reinterpret_cast<void*>(fd_), frame, 12, &written, nullptr);
    return ok && written == 12;
#else
    ssize_t n = write(fd_, frame, 12);
    return n == 12;
#endif
}

void SerialPort::setReceiveCallback(
    std::function<void(const std::vector<uint8_t>&)> cb) {
    rx_cb_ = std::move(cb);
}

bool SerialPort::parseFrame(const std::vector<uint8_t>& buf,
                             double& yaw, double& pitch, uint8_t& fire) {
    if (buf.size() < 12) return false;
    if (buf[0] != 0xAA || buf[1] != 0x55) return false;

    // 验证 CRC
    uint8_t crc = crc8(buf.data() + 2, 9);
    if (crc != buf[11]) return false;

    union { float f; uint8_t b[4]; } u;
    std::copy(buf.begin()+2, buf.begin()+6, u.b);
    yaw   = static_cast<double>(u.f);
    std::copy(buf.begin()+6, buf.begin()+10, u.b);
    pitch = static_cast<double>(u.f);
    fire  = buf[10];
    return true;
}
