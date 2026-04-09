# OpenXR Controller Demo

基于PICO OpenXR Native SDK的手柄控制器演示项目。

## 功能特性

- **手柄姿态追踪**: 实时获取左右手柄的位置和旋转
- **扳机键检测**: 检测扳机键按压值(0.0-1.0)
- **握持键检测**: 检测握持键(grip)按压值
- **摇杆输入**: 获取摇杆2D位置
- **按钮检测**: A/B/X/Y/Menu/摇杆按下检测
- **触觉反馈**: 支持手柄震动反馈
- **可视化渲染**: 3D渲染手柄立方体，颜色随输入变化

## 项目结构
```
OpenXRControllerDemo/
├── app/src/main/
│   ├── cpp/
│   │   ├── main.cpp                 # 程序入口
│   │   ├── OpenXRControllerApp.h/cpp # 主应用逻辑
│   │   ├── InputManager.h/cpp       # 手柄输入管理
│   │   └── GraphicsRenderer.h/cpp   # OpenGL ES渲染
│   ├── AndroidManifest.xml
│   └── build.gradle.kts
├── openxr/include/openxr/           # OpenXR头文件
│   ├── openxr.h
│   └── openxr_platform.h
├── CMakeLists.txt                   # CMake构建配置
└── build.gradle.kts                 # 根构建配置
```

## 技术栈

- **OpenXR 1.0**: 核心XR API
- **OpenGL ES 3.2**: 图形渲染
- **Android NativeActivity**: 无Java的纯原生应用
- **CMake**: 构建系统

## 构建步骤

### 1. 环境要求
- Android Studio Hedgehog (2023.1.1) 或更新版本
- Android SDK 34
- Android NDK 25 或更新版本
- CMake 3.22.1+

### 2. 打开项目
1. 启动 Android Studio
2. 选择 "Open an Existing Project"
3. 选择 `D:\Project\PICOProject\OpenXRControllerDemo` 目录

### 3. 构建APK
```bash
./gradlew assembleDebug
```

或使用 Android Studio 的 "Build > Build Bundle(s) / APK(s) > Build APK(s)"

### 4. 部署到PICO设备
1. 启用PICO设备的开发者模式和USB调试
2. 连接USB线
3. 点击 Android Studio 的 "Run" 按钮

## 操作说明

启动应用后，你会看到：
- 左右手柄以彩色立方体形式显示
- 扳机键按下时立方体变红
- 握持键按下时立方体变绿
- 手柄位置/旋转实时追踪

查看Logcat可获取详细的输入数据日志：
```
adb logcat -s InputManager:D OpenXRControllerDemo:D
```

## 关键代码说明

### 手柄输入获取
```cpp
// 获取扳机值
XrActionStateFloat triggerState = {XR_TYPE_ACTION_STATE_FLOAT};
xrGetActionStateFloat(session, &triggerGetInfo, &triggerState);
float triggerValue = triggerState.currentState;

// 获取姿态
XrSpaceLocation location = {XR_TYPE_SPACE_LOCATION};
xrLocateSpace(aimSpace, referenceSpace, time, &location);
XrPosef pose = location.pose;
```

### 姿态矩阵构建
```cpp
// 将OpenXR四元数转换为旋转矩阵
float rotMatrix[16] = {
    1-2*(yy+zz), 2*(xy-wz),   2*(xz+wy),  0,
    2*(xy+wz),   1-2*(xx+zz), 2*(yz-wx),  0,
    2*(xz-wy),   2*(yz+wx),   1-2*(xx+yy),0,
    0,           0,           0,          1
};
```

## 参考文档

- [PICO OpenXR SDK 文档](https://developer.picoxr.com/)
- [OpenXR Specification](https://www.khronos.org/openxr/)
- [PICO 开发者论坛](https://developer-cn.picoxr.com/)

## 许可证

基于PICO OpenXR SDK示例代码修改，遵循相应许可证。
