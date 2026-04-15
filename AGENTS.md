# OpenXRControllerDemo Agent Handoff

## 项目定位

这是一个基于 PICO OpenXR Sample Framework 改造出来的头显内调试与控制应用。

当前目标不是做通用 UI Demo，而是做一个能在头显内稳定运行的单窗口控制台，用来同时完成这些事：

- 查看左右手柄状态
- 查看手部追踪状态与关键手势指标
- 查看灵巧手/机械手映射数据
- 通过局域网 UDP 把当前控制与手势数据发给外部设备
- 在保持官方 Sample 渲染链路的前提下做 GUI，不要偏离 OpenXR / swapchain / graphics plugin 的真实运行路径

## 接管前先看什么

建议新 session 先读这几个文件：

1. `D:/Project/PICOProject/OpenXRControllerDemo/AGENTS.md`
2. `D:/Project/PICOProject/OpenXRControllerDemo/app/src/main/cpp/main.cpp`
3. `D:/Project/PICOProject/OpenXRControllerDemo/framework/src/gui/ImGuiRenderer.cpp`
4. `D:/Project/PICOProject/OpenXRControllerDemo/framework/src/gui/GuiWindow.h`
5. `D:/Project/PICOProject/OpenXRControllerDemo/framework/src/gui/GuiWindow.cpp`

如果接手人准备动稳定性问题，再补读：

- `D:/Project/PICOProject/OpenXRControllerDemo/framework/src/model/objects/TruncatedCone.cpp`
- `D:/Project/PICOProject/OpenXRControllerDemo/framework/src/openxrWrapper/BasicOpenXrWrapper.cpp`
- `D:/Project/PICOProject/OpenXRControllerDemo/framework/src/openxrWrapper/extensions/FBDisplayRefreshRates.cpp`
- `D:/Project/PICOProject/OpenXRControllerDemo/framework/src/graphicsPlugin/OpenGLESGraphicsPlugin.cpp`

## 当前架构

### 1. 运行主线

主业务基本都在：

- `D:/Project/PICOProject/OpenXRControllerDemo/app/src/main/cpp/main.cpp`

核心类：

- `ControllerDiagnosticDemo : public AndroidOpenXrProgram`

当前主循环相关方法：

- `CustomizedSessionInit()`
  - 初始化手部追踪
  - 初始化网络发送
  - 创建 GUI / 场景对象
- `CustomizedPreRenderFrame()`
  - 更新手柄可视化
  - 更新手部追踪
  - 更新手部骨架/关节可视化
  - 更新校准状态
  - 处理窗口拖动
  - 处理输入事件
  - 更新 UDP 发送
- `CustomizedRender()`
  - 走 OpenXR + graphics plugin 正常渲染路径

### 2. GUI 架构

项目底层确实是 ImGui，但业务层不是直接在外面到处裸写 `ImGui::Begin()`。

目前是“两层结构”：

- 低层渲染：
  - `framework/src/gui/ImGuiRenderer.cpp`
- 业务窗口抽象：
  - `framework/src/gui/GuiWindow.h`
  - `framework/src/gui/GuiWindow.cpp`

这套结构当前已经扩展成支持两种模式：

1. 旧模式：`AddText / AddButton / AddCheckBox`
2. 新模式：给 `GuiWindow` 挂一个自定义 ImGui 绘制回调

已新增能力：

- `GuiWindow::SetCustomRenderCallback(...)`
- `GuiWindow::GetCustomRenderCallback()`
- `GuiWindow::SetButtonCallback(...)`

`ImGuiRenderer.cpp` 里已经接好：

- 如果某个 `GuiWindow` 挂了 `customRenderCallback`
- 则在渲染该窗口时直接执行回调

这意味着当前主控制台已经是“业务层原生 ImGui 布局 + 仍然走项目原本 GuiWindow/ImGuiRenderer/纹理平面渲染链”的模式。

### 3. 主控制台 UI 架构

当前主窗口入口：

- `AddMainDashboard()`

当前主窗口绘制入口：

- `DrawDashboardImGui()`

当前 dashboard 已经不再使用旧的静态 text/button id 拼接主界面。
旧 dashboard 兼容逻辑已经清理过一轮，现在主面板主要依赖 `DrawDashboardImGui()`。

主窗口布局结构：

- 左侧：导航区
- 顶部：状态 badge
- 中部：按页面切换的卡片区
- 底部：按页面切换的操作按钮区

页面枚举：

- `Overview`
- `Controller`
- `Hand`
- `Network`
- `Mapping`

切页方法：

- `SetDashboardSection(...)`

### 4. 其他仍存在的旧面板

这些旧窗口对象和对应 `UpdateText()` 更新逻辑还在：

- `controller_window_`
- `hand_window_`
- `runtime_window_`
- `robot_mapping_window_`
- `network_window_`

它们现在不是主交互入口，但相关文本构造函数仍然被复用：

- `BuildControllerText(...)`
- `BuildHandText(...)`
- `BuildRobotMappingText(...)`
- `BuildNetworkPanelText()`

如果后续确认这些旧窗口已经完全不再展示，可以继续清理。
但清理前要先确认没有别的平面或场景还在引用它们。

## 已实现功能

### 1. 中文 GUI 与字体

相关文件：

- `framework/src/gui/ImGuiRenderer.cpp`

已做的事：

- 优先尝试加载设备系统中文字体
  - `PICOSansSC-Regular.ttf`
  - `PICOSansSC-Medium.ttf`
  - `NotoSansCJK-Regular.ttc`
- ImGui 字符范围已切到：
  - `GetGlyphRangesChineseSimplifiedCommon()`
- `ButtonTextAlign` 已设为居中
- 全局 `ScaleAllSizes(...)` 已调整过
- 预加载字体尺寸包括：
  - `16, 20, 24, 28, 32, 36, 40`

注意：

- 当前“能显示中文”这件事代码层已经打通
- 但最终字号、行高、局部拥挤与否，仍然需要头显实机看效果

### 2. 单窗口控制台

相关文件：

- `app/src/main/cpp/main.cpp`

当前页面含义：

- `总览`
  - 运行摘要
  - 输入摘要
  - 网络摘要
- `手柄`
  - 左右手柄独立卡片
  - 输入与联调摘要
- `手部`
  - 左右手追踪卡
  - 手势与校准区
- `网络`
  - 连接状态
  - 全量设备列表
  - 发送内容与步骤
- `映射`
  - 左右手映射值
  - 映射说明

当前使用的原生 ImGui 控件/布局方式包括：

- `Button`
- `Columns`
- `BeginChild`
- `Separator`
- `TextWrapped`
- 滚动子区域

### 3. 手柄状态与输入事件

相关方法：

- `BuildControllerText(int hand) const`
- `BuildButtonSummary(int hand) const`
- `DetectInputEvents()`
- `PulseController(int hand)`

当前可显示/处理的信息：

- 手柄是否连接
- 握持位姿 / 指向位姿
- 扳机值
- 握把值
- 电量
- 摇杆值
- 按键/触摸摘要
- 最近输入事件
- 左右手控制器震动按钮

### 4. 手部追踪与手势

相关方法：

- `InitializeHandTracking()`
- `UpdateHandTracking()`
- `UpdateHandVisuals()`
- `BuildHandText(int hand) const`
- `BuildHandGestureSummary(int hand) const`
- `ComputeThumbPinchStrength(...)`

当前手部追踪数据包括：

- 手掌位置
- 各关节位置
- index tip pose
- pinch:
  - 食指
  - 中指
  - 无名指
  - 小指
- 追踪数据源状态

当前手势摘要已经不是只看食指捏合了，而是会显示：

- 四指捏合百分比
- 拇指 `thumb_x`
- 拇指 `thumb_y`
- 食指弯曲
- 中指弯曲
- 无名指弯曲
- 小指弯曲

### 5. 灵巧手 / 机械手映射

相关方法：

- `ComputeRobotHandMetrics(int hand) const`
- `ApplyCalibration(int hand, const RobotHandMetrics& raw_metrics) const`
- `BuildRobotMappingText(int hand) const`

当前映射关注值：

- `thumb_x`
  - 语义：大拇指侧摆
- `thumb_y`
  - 语义：大拇指弯曲
- `index_bend`
- `middle_bend`
- `ring_bend`
- `little_bend`
- `palm_position`
  - 已支持作为相对空间位置输出

### 6. 校准逻辑

相关方法：

- `BeginHandCalibration()`
- `ClearHandCalibration()`
- `UpdateHandCalibration()`
- `ApplyCalibration(...)`
- `BuildCalibrationStatusText()`
- `CalibrateControllerFallbackZero(int hand)`

当前校准能力：

- 点击“开始校准”后
  - 等待手势上线
  - 延迟 2 秒
  - 把当前姿态记为零点
- 校准内容包括：
  - 掌心相对位置零点
  - `thumb_x`
  - `thumb_y`
  - 四指弯曲角
- 支持左右手分别校准

当前还有一条“手柄回退校准”路径：

- 当手柄已连接但手势未识别
- 按下扳机时
- 调用 `CalibrateControllerFallbackZero(hand)`
- 将当前控制器位置作为零点

### 7. UDP 网络联调

相关方法：

- `InitializeNetworkStreaming()`
- `StartLanScan()`
- `AdvanceSelectedLanTarget(int delta)`
- `BuildSelectedTargetLabel()`
- `ResolveSelectedTarget(...)`
- `BuildUdpPayloadJson(uint64_t sequence) const`
- `UpdateNetworkStreaming()`
- `BuildNetworkPanelText()`

当前网络侧能力：

- 扫描局域网设备
- 记录所有发现的 IP / MAC
- 在 UI 中显示当前目标
- 支持切换上一个/下一个目标
- 支持广播目标
- 支持持续 UDP 发送
- 支持发送一帧 snapshot
- UI 中显示：
  - 是否已连接局域网
  - 当前接口名
  - 本机 IP
  - 广播地址
  - 端口
  - 当前目标
  - 扫描状态
  - 已发包数量
  - 设备总数

网络页当前已经改成支持“全部设备滚动显示”。

### 8. UDP JSON 输出内容

相关方法：

- `BuildUdpPayloadJson(...)`

当前 JSON 已包含的关键内容：

- frame / sequence 之类的基础字段
- 左右手状态
- 相对掌心位置
- `thumb_x`
- `thumb_y`
- 兼容旧字段：
  - `thumb_side`
  - `thumb_bend`
- 四指弯曲角
- `calibrated`
- `sending_by_trigger`

### 9. 扳机发送逻辑

相关方法：

- `IsTriggerStreamingActive(int hand) const`
- `DetectInputEvents()`
- `UpdateNetworkStreaming()`

当前行为：

- 如果手势不在线，但手柄在线
- 按下扳机会触发当前手的回退零点校准
- 持续按住扳机时
  - 会继续通过网络发送同款 JSON
- UI 文案会显示：
  - `扳机发送中`

### 10. VR 中移动主窗口

相关方法：

- `UpdateDashboardDragging()`
- `ResetPanelPose()`

当前行为：

- 通过握把键按住 + 移动控制器来拖动主窗口
- 可以用“面板居中”动作把窗口重置

### 11. 3D 可视化模型

相关方法：

- `CreateControllerModel(int hand)`
- `UpdateControllerVisuals()`
- `UpdateHandVisuals()`

当前状态：

- 手柄模型已经被简化过，不再是复杂模型
- 手部骨架/关节还在显示
- 指尖可视化已缩短/缩小过一轮，避免过长难看

## 已知关键修复

### 1. 稳定性修复

不要轻易回滚这些文件的改动：

- `framework/src/model/objects/TruncatedCone.cpp`
  - 修过截锥体 mesh 生成问题
- `framework/src/openxrWrapper/BasicOpenXrWrapper.cpp`
  - 回避过运行时的危险控制器探测路径
- `framework/src/openxrWrapper/extensions/FBDisplayRefreshRates.cpp`
  - 修过错误格式化输出

### 2. GUI 渲染链修复

不要轻易回滚：

- `framework/src/graphicsPlugin/OpenGLESGraphicsPlugin.cpp`

这里已经接过：

- ImGuiRenderer 初始化
- 每帧 GUI 渲染触发
- 关闭时销毁 renderer

这部分是“黑板 UI 问题”能被彻底解决的关键。

## 当前工作区状态

截至本次交接，工作区有未提交改动：

- `app/src/main/cpp/main.cpp`
- `framework/src/gui/GuiWindow.cpp`
- `framework/src/gui/GuiWindow.h`
- `framework/src/gui/ImGuiRenderer.cpp`

这些改动主要对应：

- 原生 ImGui dashboard
- GuiWindow 自定义渲染回调
- 中文字体 / 缩放 / 对齐
- dashboard 旧兼容逻辑清理

注意：

- 不要为了“回到干净状态”去误回滚这些文件
- 当前这几处正是最新 UI 架构的核心

## 当前已验证事实

最近已经确认过：

- `assembleDebug` 可以通过
- APK 可以安装到设备
- `NativeActivity` 可以被成功拉起
- 之前的若干运行日志里出现过：
  - 中文字体成功加载
  - GUI draw list
  - `First frame, completed`
  - `XR_SESSION_STATE_FOCUSED`

但要注意：

- 最近一次 `adb shell am start -W ...` 虽然返回 `Status: ok`
- 但后续没有稳定抓到进程和关键日志
- 这更像是“启动请求成功，但是否真正留在沉浸式前台还需要头显现场确认”

所以当前结论是：

- 构建链路是通的
- 安装链路是通的
- 启动请求是通的
- 但最新这版是否稳定留在前台，需要戴头显再看

## 当前最可能继续要做的事

### 优先级 1

继续优化头显内实际 UI 效果：

- 字号是否仍偏小
- 某些页面是否仍有重叠
- 卡片尺寸是否合理
- 网络页设备列表是否足够清晰
- 状态 badge 是否足够醒目

### 优先级 2

继续把页面完全做成“工具面板化”：

- 更明显的连接状态
- 更明显的发送状态
- 更明显的校准状态
- 更明显的手势在线/离线状态

### 优先级 3

如果确认旧窗口完全没用了，可以考虑继续清理：

- `controller_window_`
- `hand_window_`
- `runtime_window_`
- `robot_mapping_window_`
- `network_window_`

但在没有确认场景里完全不再挂这些平面前，不建议贸然删除。

## 暂时不要做的事

- 不要回滚前述稳定性修复
- 不要改坏 OpenXR / graphics plugin 的真实渲染链路
- 不要重新把 dashboard 改回大段静态 `AddText()` 布局
- 不要在没有头显反馈的情况下盲目继续堆更多文本字段

## 常用命令

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

### 查看设备

```powershell
& 'C:\Users\plf\AppData\Local\Android\Sdk\platform-tools\adb.exe' devices
```

### 抓关键日志

```powershell
& 'C:\Users\plf\AppData\Local\Android\Sdk\platform-tools\adb.exe' shell logcat -d -t 500 | Select-String 'ImGuiRenderer loaded font|fallback to default font|GUI debug|First frame, completed|XR_SESSION_STATE_FOCUSED|Fatal signal|Runtime aborting|OpenXRControllerDemo|openxrcontroller' | ForEach-Object { $_.Line }
```

## 给下一个 Agent 的最短提示词

如果下一个 session 想最快接手，直接这样说：

`请先阅读 D:/Project/PICOProject/OpenXRControllerDemo/AGENTS.md 和 D:/Project/PICOProject/OpenXRControllerDemo/app/src/main/cpp/main.cpp，然后继续接管这个项目。重点继续做头显里的单窗口控制台 UI、状态显示和实机运行验证，不要回滚已有稳定性修复。`

## 重要提醒

`AGENTS.md` 只是交接入口，不会自动执行。

新 session 必须明确告诉 Codex：

- 先读哪个文件
- 然后按什么目标继续

否则它不会自动接着当前上下文往下做。
