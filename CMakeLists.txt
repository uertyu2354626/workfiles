# ============================================================
# CMakeLists.txt — RoboMaster 装甲板识别视觉系统
# CMake ≥ 3.10, C++17
# ============================================================
cmake_minimum_required(VERSION 3.10)
project(romaster_vision VERSION 1.0.0 LANGUAGES CXX)

# ---------- 编译选项 ----------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Release 优化
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
if(MSVC)
    add_compile_options(/W4 /O2)
else()
    add_compile_options(-Wall -Wextra -O3)
endif()

# ---------- 查找依赖 ----------
find_package(OpenCV REQUIRED COMPONENTS core imgproc calib3d videoio highgui)
find_package(Threads REQUIRED)

message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBRARIES}")

# ============================================================
# 头文件目录
# ============================================================
include_directories(${CMAKE_SOURCE_DIR}/include)

# ============================================================
# 源码列表
# ============================================================
set(SOURCES
    src/main.cpp

    src/detector/armor_detector.cpp
    src/solver/pnp_solver.cpp
    src/predictor/kalman_predictor.cpp

    src/serial/serial_port.cpp
    src/camera/camera.cpp
    src/debug/debugger.cpp
)

# ============================================================
# 可执行文件
# ============================================================
add_executable(${PROJECT_NAME} ${SOURCES})

# 链接库
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${OpenCV_LIBRARIES}
        Threads::Threads
)

# Windows 下链接 ws2_32（串口用 WinSock）
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
endif()

# ============================================================
# 安装规则
# ============================================================
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)

# ============================================================
# 测试（可选）
# ============================================================
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    # TODO: 添加 gtest 单元测试
endif()
