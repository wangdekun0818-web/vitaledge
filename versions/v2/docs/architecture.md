# VitaEdge 技术架构说明

## 1. 总体架构

VitaEdge 采用三层结构：

1. 设备层（`firmware`）：运行在 ESP32-S3，负责采集传感器数据与本地推理。
2. 应用层（`miniapp`）：微信小程序负责实时展示、历史趋势与家人守护交互。
3. 智能体层（`agent`）：OpenClaw Agent 负责健康对话、记忆管理与主动关怀策略。

## 2. 设备层（ESP32-S3）

- 传感器模块：
  - `MAX30102`（PPG）
  - `AD8232`（ECG）
- 推理模块：`firmware/src/model/inference.*`
  - 本地 TinyML 推理，降低延迟与云依赖。
- 通信模块：`firmware/src/ble/ble_service.*`
  - 通过 BLE 将体征与状态广播到小程序。
- 主控流程：`firmware/src/main.cpp`
  - 采样 -> 预处理 -> 推理 -> 广播。

## 3. 小程序层（WeChat Mini Program）

- 页面结构（`miniapp/pages`）：
  - `index`：实时监测看板
  - `history`：历史趋势分析
  - `agent`：Agent 对话界面
  - `family`：家人守护与预警
- 服务层（`miniapp/services`）：
  - `ble.js` 负责 BLE 连接、读取与解析数据。

## 4. 智能体层（OpenClaw Agent）

- 核心文件：`agent/vitaedge_agent.py`
- 能力侧重点：
  - 上下文记忆（长期健康画像）
  - 心跳机制（定时主动关怀）
  - 情绪/压力感知（结合 HRV 等指标）
  - 分级告警与家人通知联动

## 5. 数据流说明

1. 设备层采集 PPG/ECG 与衍生指标。
2. TinyML 在设备侧输出健康状态/风险信号。
3. 通过 BLE 将数据同步到小程序。
4. Agent 结合历史上下文生成建议、提醒与告警策略。

## 6. 目录索引

- `firmware/`：ESP32 固件与边缘推理
- `miniapp/`：微信小程序前端与 BLE 服务
- `agent/`：智能体逻辑与记忆策略
- `models/`：模型训练与导出产物目录
- `docs/`：项目文档（本文件）
