# OpenXRControllerDemo Agent Progress

## 项目目标

这是一个基于 PICO OpenXR Sample Framework 改出来的调试与控制应用，核心目标是：

- 在头显中稳定显示 OpenXR 单窗口控制台
- 同时展示手柄状态、手部追踪、灵巧手映射和局域网 UDP 联调信息
- 保持和官方 Sample 一样的 OpenXR / Swapchain / 图形链路，避免“能编译但运行路径不一致”的问题

## 当前结论

### 1. 崩溃主因已经定位并修复过一轮

之前“经常闪退”不是普通逻辑错误，而是 native 层未定义行为和内存损坏。

已修复的高风险点：

- `framework/src/model/objects/TruncatedCone.cpp`
  - 修复截锥体网格生成时的顶点/索引缓冲区错误
  - 这是此前握着手柄后容易崩的直接高风险点之一
- `framework/src/openxrWrapper/BasicOpenXrWrapper.cpp`
  - 回避运行时某些控制器类型探测路径的危险调用
- `framework/src/openxrWrapper/extensions/FBDisplayRefreshRates.cpp`
  - 修复 `float` 按错误格式输出的问题
  - 之前日志里异常的刷新率打印就是这个信号

### 2. GUI “整块黑板”问题已经定位并修复

根因不是配色，而是 GUI 渲染链在实际运行路径里没有真正接上。

已修复：

- `framework/src/graphicsPlugin/OpenGLESGraphicsPlugin.cpp`
  - 在实际运行的 OpenGLES 图形插件里初始化 `ImGuiRenderer`
  - 每帧触发 GUI 渲染
  - 关闭图形设备时正确销毁 GUI 渲染器

验证结果：

- 应用可以进入首帧
- 会话能进入 `XR_SESSION_STATE_FOCUSED`
- 日志中已出现 GUI draw list / 顶点数 / 首帧完成等信息

### 3. UI 已重构成“单窗口控制台”

现在的主界面目标是：

- 只保留正中一个主窗口
- 左侧做导航感区域
- 中间和右侧展示运行数据
- 底部做常用操作按钮

主要实现文件：

- `app/src/main/cpp/main.cpp`

## 本轮 UI 进展

### 已完成

- 主窗口改成白底高对比风格
- GUI 字体改为优先加载设备系统字体：
  - `PICOSansSC-Regular.ttf`
  - `PICOSansSC-Medium.ttf`
  - `NotoSansCJK-Regular.ttc`
- ImGui 字符集已切到：
  - `GetGlyphRangesChineseSimplifiedCommon()`
- 已将主面板可见文案大部分切回中文
- 已把 UI 缩放从过大的全局值调低，缓解按钮和文本被挤压的问题
- 已将按钮文本对齐设为居中
- 已扩大主窗口尺寸并放宽区块间距
- 已对底部按钮和左侧导航按钮重新调尺寸

### 当前状态

代码层面已经完成以下调整：

- `framework/src/gui/ImGuiRenderer.cpp`
  - 中文字库范围已启用
  - 预加载字号已包含 `16, 20, 24, 28, 32, 36, 40`
  - `ButtonTextAlign` 已设为居中
  - `ScaleAllSizes` 已从此前过大的值下调
- `app/src/main/cpp/main.cpp`
  - 主界面标题/导航/状态/网络/映射文案已大面积中文化
  - 主窗口尺寸从旧版进一步放大
  - 底部操作按钮已重排为更宽、更高的中文按钮
  - 运行总览、手柄状态、手部追踪、网络工具、灵巧手映射都改成更适合单窗口的紧凑文案

### 仍需用户在头显里最终确认

虽然构建、安装、运行都通过了，但下面两项仍需要佩戴头显后的真实视觉确认：

- 中文是否已经正常显示，不再变成问号或黑方块
- 当前按钮、标题和分栏是否还有裁切、重叠或太小的问题

也就是说：

- “代码和运行链路”已经通
- “最终视觉效果”还差用户确认这一关

## 最近已验证的事实

本地构建与运行状态：

- `assembleDebug` 构建通过
- APK 已成功安装到设备
- `com.example.openxrcontroller/android.app.NativeActivity` 进程可正常启动
- 设备日志确认过：
  - 中文字体文件成功加载
  - GUI 已生成 draw list
  - 应用进入 `First frame, completed`
  - 会话进入 `XR_SESSION_STATE_FOCUSED`

## 可直接运行的环境要求

如果新 session 需要直接构建、安装、拉起应用，请先确认下面这些环境。

### 1. Java

当前工程可以直接使用 Android Studio 自带的 JBR。

- `JAVA_HOME`
  - `C:\Program Files\Android\Android Studio\jbr`
- `PATH` 需要包含
  - `%JAVA_HOME%\bin`

PowerShell 可直接这样设置：

```powershell
$env:JAVA_HOME='C:\Program Files\Android\Android Studio\jbr'
$env:PATH="$env:JAVA_HOME\bin;$env:PATH"
```

如果没有这一步，常见现象就是：

- `JAVA_HOME` 没设
- `java` 不在 `PATH`
- 无法直接执行 `.\gradlew.bat assembleDebug`

### 2. Android SDK / adb

当前机器上可用的 `adb` 路径是：

- `C:\Users\plf\AppData\Local\Android\Sdk\platform-tools\adb.exe`

如果不想改全局 `PATH`，可以直接用绝对路径执行。

### 3. 工作目录

所有命令默认在下面这个目录执行：

- `D:\Project\PICOProject\OpenXRControllerDemo`

### 4. NDK 现状

当前工程可以正常构建，但 Gradle 会提示：

- `ndk.dir` 的用法已过时

这不是当前阻塞项，暂时不影响 `assembleDebug` 成功。

## 一键常用命令

### 构建 Debug APK

```powershell
$env:JAVA_HOME='C:\Program Files\Android\Android Studio\jbr'
$env:PATH="$env:JAVA_HOME\bin;$env:PATH"
.\gradlew.bat assembleDebug
```

### 安装 APK 到设备

```powershell
& 'C:\Users\plf\AppData\Local\Android\Sdk\platform-tools\adb.exe' install -r 'D:\Project\PICOProject\OpenXRControllerDemo\app\build\outputs\apk\debug\app-debug.apk'
```

### 强制停止并重新启动应用

```powershell
& 'C:\Users\plf\AppData\Local\Android\Sdk\platform-tools\adb.exe' shell am force-stop com.example.openxrcontroller
& 'C:\Users\plf\AppData\Local\Android\Sdk\platform-tools\adb.exe' shell am start -W -a android.intent.action.MAIN -c org.khronos.openxr.intent.category.IMMERSIVE_HMD -n com.example.openxrcontroller/android.app.NativeActivity
```

### 抓取关键日志

```powershell
& 'C:\Users\plf\AppData\Local\Android\Sdk\platform-tools\adb.exe' shell logcat -d -t 400 | Select-String 'ImGuiRenderer loaded font|fallback to default font|GUI debug|First frame, completed|XR_SESSION_STATE_FOCUSED|Fatal signal|Runtime aborting' | ForEach-Object { $_.Line }
```

## 关键文件

### 高优先级文件

- `D:/Project/PICOProject/OpenXRControllerDemo/app/src/main/cpp/main.cpp`
- `D:/Project/PICOProject/OpenXRControllerDemo/framework/src/gui/ImGuiRenderer.cpp`
- `D:/Project/PICOProject/OpenXRControllerDemo/framework/src/graphicsPlugin/OpenGLESGraphicsPlugin.cpp`

### 之前的稳定性修复文件

- `D:/Project/PICOProject/OpenXRControllerDemo/framework/src/model/objects/TruncatedCone.cpp`
- `D:/Project/PICOProject/OpenXRControllerDemo/framework/src/openxrWrapper/BasicOpenXrWrapper.cpp`
- `D:/Project/PICOProject/OpenXRControllerDemo/framework/src/openxrWrapper/extensions/FBDisplayRefreshRates.cpp`

## 当前工作区改动

截至写入本文件时，工作区里已有这些改动：

- `app/src/main/cpp/main.cpp`
- `framework/src/graphicsPlugin/OpenGLESGraphicsPlugin.cpp`
- `framework/src/gui/ImGuiRenderer.cpp`
- `framework/src/model/objects/TruncatedCone.cpp`
- `framework/src/openxrWrapper/BasicOpenXrWrapper.cpp`
- `framework/src/openxrWrapper/extensions/FBDisplayRefreshRates.cpp`

注意：

- 不要误回滚这些改动
- 其中前 3 个主要是 GUI/显示链路
- 后 3 个主要是稳定性修复

## 下一步建议

新 session 进来后，优先按这个顺序继续：

1. 先读本文件，再读 `main.cpp` 和 `ImGuiRenderer.cpp`
2. 确认用户提供的最新头显截图
3. 只围绕“中文是否正常显示”和“按钮/文字是否仍被裁切”继续微调
4. 如果中文仍异常，优先继续检查字体 atlas 和设备字体兼容性
5. 如果中文正常但布局仍挤，继续收缩信息密度，不要再往主窗口里加更多调试字段

## 新 Session 推荐提示词

新开一个 Codex session 后，最省事的说法是：

`请先阅读 D:/Project/PICOProject/OpenXRControllerDemo/AGENTS.md，然后接管这个项目，按文档里的当前进度继续处理 UI 显示问题。`

如果你想更明确一点，可以直接这样说：

`请先阅读 D:/Project/PICOProject/OpenXRControllerDemo/AGENTS.md 和 D:/Project/PICOProject/OpenXRControllerDemo/app/src/main/cpp/main.cpp，再继续处理头显里的中文显示和按钮裁切问题。不要回滚已有稳定性修复。`

## 重要说明

Codex 不会在“新 session 一打开”时自动执行某个 md 文件。

你需要在新对话里明确告诉它：

- 先读哪个 md
- 读完后按什么目标继续做

也就是说，`AGENTS.md` 是接管入口，不是自动启动脚本。
