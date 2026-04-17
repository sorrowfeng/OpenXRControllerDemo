# UDP JSON Format

## 概览

当前应用通过 UDP 发送的 JSON 顶层结构如下：

```json
{
  "hands": [
    { "... left hand object ..." },
    { "... right hand object ..." }
  ]
}
```

说明：

- `hands` 固定包含两个元素
- 第一个元素是左手
- 第二个元素是右手
- 每只手都会带：
  - `hand`
  - `relative_position`
  - `orientation`
  - `finger_joints`

## 顶层字段

### `hands`

类型：

- `array[2]`

内容：

- 左手对象
- 右手对象

## 单手对象字段

### `hand`

类型：

- `string`

取值：

- `"left"`
- `"right"`

### `relative_position`

类型：

- `object`

字段：

- `x`
- `y`
- `z`

含义：

- 当前手或手柄相对于“最近一次零点重置/校准”的相对位置
- 单位为米

### `orientation`

类型：

- `object`

字段：

- `pitch`
- `yaw`
- `roll`

含义：

- 当前手或手柄相对于“最近一次零点重置/校准”的相对欧拉角
- 单位为度

### `finger_joints`

类型：

- `array[6]`

长度固定为 6。

当前顺序是：

1. 拇指侧摆 / thumb_x
2. 拇指弯曲 / thumb_y
3. 食指弯曲
4. 中指弯曲
5. 无名指弯曲
6. 小拇指弯曲

注意：

- 当数据源切到“手势数据”时，这 6 个值来自真实手部追踪与校准结果
- 当数据源切到“手柄数据”时，这 6 个值会退化成控制器模拟值：
  - 第 1 项固定为 `1.4`
  - 第 2 到第 6 项为 `1.4 * trigger`

## 两种发送模式

当前发送内容受 UI 里的“UDP 数据源”控制。

### 1. 手势数据模式

对应代码路径：

- `BuildUdpPayloadJson() -> build_hand_json(...)`

特征：

- `relative_position` 来自掌心相对位置
- `orientation` 来自掌心姿态
- `finger_joints` 来自：
  - 拇指 `thumb_x`
  - 拇指 `thumb_y`
  - 四指弯曲角

### 2. 手柄数据模式

对应代码路径：

- `BuildUdpPayloadJson() -> build_controller_json(...)`

特征：

- `relative_position` 来自控制器相对位置
- `orientation` 来自控制器相对姿态
- `finger_joints` 是手柄回退模拟值

## 当前实现细节

基于当前代码，补充几个重要说明：

- 顶层目前只有 `hands`
- 没有额外的 `frame`、`sequence`、`timestamp` 字段
- `BuildUdpPayloadJson(uint64_t sequence)` 虽然收到了 `sequence` 参数，但当前实现里没有把它写进 JSON
- 数值输出用了 `std::fixed << std::setprecision(3)`，所以通常会保留 3 位小数
- 当某只手当前不可用时，位置会退回到 `0.0`
- 手柄模式下，如果控制器不在线，相对位置也会退回到 `0.0`

## 示例 1：手势数据模式

```json
{
  "hands": [
    {
      "hand": "left",
      "relative_position": {
        "x": 0.012,
        "y": -0.034,
        "z": 0.056
      },
      "orientation": {
        "pitch": -4.218,
        "yaw": 12.640,
        "roll": 1.905
      },
      "finger_joints": [
        18.200,
        34.500,
        42.100,
        38.900,
        35.700,
        30.600
      ]
    },
    {
      "hand": "right",
      "relative_position": {
        "x": -0.008,
        "y": 0.021,
        "z": 0.049
      },
      "orientation": {
        "pitch": 2.315,
        "yaw": -10.441,
        "roll": -3.220
      },
      "finger_joints": [
        14.800,
        28.700,
        40.000,
        36.200,
        31.400,
        27.900
      ]
    }
  ]
}
```

## 示例 2：手柄数据模式

```json
{
  "hands": [
    {
      "hand": "left",
      "relative_position": {
        "x": 0.003,
        "y": -0.015,
        "z": 0.022
      },
      "orientation": {
        "pitch": 1.240,
        "yaw": -5.120,
        "roll": 0.880
      },
      "finger_joints": [
        1.400,
        0.700,
        0.700,
        0.700,
        0.700,
        0.700
      ]
    },
    {
      "hand": "right",
      "relative_position": {
        "x": -0.011,
        "y": 0.018,
        "z": 0.019
      },
      "orientation": {
        "pitch": -2.030,
        "yaw": 6.410,
        "roll": -1.540
      },
      "finger_joints": [
        1.400,
        1.120,
        1.120,
        1.120,
        1.120,
        1.120
      ]
    }
  ]
}
```

## 对接建议

如果外部接收端要做协议解析，建议按下面这组稳定字段先对接：

- `hands[0].hand`
- `hands[0].relative_position`
- `hands[0].orientation`
- `hands[0].finger_joints`
- `hands[1].hand`
- `hands[1].relative_position`
- `hands[1].orientation`
- `hands[1].finger_joints`

如果后续要扩协议，优先考虑新增这些顶层字段：

- `sequence`
- `frame`
- `timestamp`
- `data_source`

这样接收端会更容易做去重、重放和状态判断。
