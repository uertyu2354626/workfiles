# RoboMaster 装甲板识别系统 — 快速开始

## 编译

```bash
cd F:\test
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
mingw32-make -j
```

## 运行

```bash
./romaster_vision.exe --camera 0 --serial COM3
```

## 协议格式

```
AA 55 [yaw:4B LE] [pitch:4B LE] [fire:1B] [CRC8:1B]
```

## 目录结构

```
include/    — 所有头文件
src/
  detector/ — [VISION] 装甲板检测
  solver/   — [VISION] PnP 角度解算
  predictor/— [VISION] 卡尔曼预测
  serial/   — [ELEC-IF] 串口通信
  camera/   — [ELEC-IF] 相机驱动
  debug/    — [ELEC-IF] 调试可视化
  main.cpp  — 主程序入口
```
