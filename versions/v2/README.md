# VitaEdge 🩺

<p align="center">
  <img src="https://img.shields.io/badge/成本-¥167-red?style=for-the-badge" />
  <img src="https://img.shields.io/badge/Platform-ESP32--S3-blue?style=for-the-badge" />
  <img src="https://img.shields.io/badge/AI-TinyML%20on%20Device-orange?style=for-the-badge" />
  <img src="https://img.shields.io/badge/Agent-OpenClaw-blueviolet?style=for-the-badge" />
</p>

<p align="center">
  <b>¥167 的硬件 + 12KB 的模型 + OpenClaw Agent<br>构建一个 24 小时主动守护健康的智能体</b>
</p>

---

## 为什么做这个

父亲长期被高血糖、高血压困扰。但他不是不在乎——是每次有创检测太麻烦，一拖再拖，数据断断续续。

**监测的障碍从来不是意识，是摩擦。**

现有的健康设备解决的是「数据展示」，没有解决「主动发现问题」。VitaEdge 想做的是：让监测消失在日常里，让预警在问题变严重之前就发生。

```
传统设备：采集数据 → 展示数字 → 等待用户查看
VitaEdge：  数据 → TinyML 本地推理 → Agent 感知 → 主动预警 → 越用越懂你
```

---

## 核心差异点

| | 普通健康手环 | VitaEdge |
|---|---|---|
| **推理位置** | 云端（需联网） | **设备本地**（12KB TinyML） |
| **传感器** | PPG 单传感器 | **PPG + ECG 双路** |
| **Agent** | 无 | **OpenClaw 自主 Agent** |
| **行为** | 等你打开 App | **主动找你** |
| **成本** | ¥300–2000 | **¥167** |
| **数据** | 云端存储 | **Local-first，数据主权在你** |

---

## 技术架构

```
┌─────────────────────────────────────────────────────────┐
│                  硬件层 ESP32-S3  (firmware/)            │
│                                                         │
│   MAX30102 (PPG)          AD8232 (ECG)                  │
│        │                       │                        │
│        └──────────┬────────────┘                        │
│                   │                                     │
│            TinyML 推理引擎                               │
│       血压估算 / HRV 分析 / 异常检测                      │
│       12KB 模型，延迟 <100ms，完全离线                    │
│                   │                                     │
│                BLE 广播 (VitaEdge_001)                   │
└───────────────────┼─────────────────────────────────────┘
                    │ BLE
                    ▼
┌─────────────────────────────────────────────────────────┐
│              微信小程序 (miniapp/)                        │
│                                                         │
│   实时监测   历史趋势   Agent 对话   家人守护              │
│        └──────────────────┘                             │
│               ble.js 数据层                              │
└───────────────────┬─────────────────────────────────────┘
                    │ JSON
                    ▼
┌─────────────────────────────────────────────────────────┐
│          OpenClaw Agent 层 (agent/)                      │
│                                                         │
│  ┌─────────────────────────────────────────────────┐   │
│  │  Heartbeat  每 5 分钟自动巡检                     │   │
│  │  读取最新数据 → 对照个人基线 → 判断风险等级        │   │
│  │  正常 → 静默  |  异常 → 分级预警 → 通知家属        │   │
│  └─────────────────────────────────────────────────┘   │
│                                                         │
│  长期记忆（UserHealthProfile）                           │
│  ├── 食物-血糖影响图谱（持续学习）                        │
│  ├── 运动反应模型（个性化）                               │
│  └── 个人健康基线（跨会话持久）                           │
│                                                         │
│  情商感知（HRV → 情绪 → 个性化建议）                      │
└─────────────────────────────────────────────────────────┘
```

---

## OpenClaw 三大机制

### 1. Heartbeat — 主动巡检，不等你打开

```python
# agent/vitaedge_agent.py
def heartbeat(self) -> dict:
    """每 5 分钟自动触发，主动评估风险"""
    anomaly_level = self.detect_anomaly()
    
    if anomaly_level >= 2:
        return self.generate_alert(anomaly_level)  # 主动推送
    
    return {"status": "HEARTBEAT_OK"}  # 静默，不打扰
```

心率异常 / 血压飙升 / HRV 骤降 → Agent 主动找你，而不是等你刷 App。

---

### 2. 长期记忆 — 越用越懂你

```python
# 学习你吃什么会让血糖飙升
def learn_food_impact(self, food: str, glucose_delta: float, hours: int):
    # 自动建立「食物 → 血糖变化」的个人图谱
    self.profile.food_impact[food]["glucose_avg"] = ...

# 下次你问"我能吃白米饭吗"
# Agent 不给通用答案，给你自己的历史数据
```

---

### 3. 情商感知 — 读懂你的状态

```python
# HRV 直接映射到情绪状态
def perceive_emotion(self) -> dict:
    hrv = self.current_health["hrv"]
    
    if hrv > 60:   return {"emotion": "状态良好",  "emoji": "🙂"}
    elif hrv > 40: return {"emotion": "略有压力",  "emoji": "😐"}
    elif hrv > 20: return {"emotion": "压力较大",  "emoji": "😰"}
    else:          return {"emotion": "极度紧张",  "emoji": "😱"}
```

---

## TinyML on ESP32 — 最极客的部分

```cpp
// firmware/src/main.cpp
// 12KB 模型，跑在 ¥35 的芯片上
// 完全离线，延迟 <100ms

void runInference() {
    float ppg_buffer[256];
    getPPGBuffer(ppg_buffer, 256);

    // 血压估算（TinyML模型）
    float bp_result[2];
    estimateBloodPressure(ppg_buffer, 256, bp_result);
    
    // HRV 分析
    current_hrv = analyzeHRV(ppg_buffer, 256);
    
    // 异常检测
    anomaly_level = detectAnomaly(
        current_heart_rate,
        bp_result[0],       // 收缩压
        current_glucose,
        current_hrv
    );
}
```

不依赖云端，不需要联网，数据不离开设备。

---

## 硬件 BOM（¥167 总成本）

| 器件 | 型号 | 用途 | 成本 |
|---|---|---|---|
| 主控 | ESP32-S3 | WiFi + BLE + TinyML 推理 | ¥35 |
| PPG 传感器 | MAX30102 | 心率 + 血氧 + HRV | ¥15 |
| ECG 模块 | AD8232 | 心电信号采集 | ¥12 |
| OLED 屏幕 | 0.96" | 实时数据显示 | ¥10 |
| 锂电池 | 18650 3000mAh | 24h+ 续航 | ¥20 |
| 指夹 | PPG 专用 | 传感器固定 | ¥25 |
| ECG 电极 | 胸贴 | 心电采集 | ¥15 |
| 其他 | 杜邦线/外壳 | 组装 | ¥35 |
| **合计** | | | **¥167** |

---

## 快速开始

```bash
# 克隆仓库
git clone git@github.com:wangdekun0818-web/vitaledge.git
cd vitaledge

# 烧录固件（需安装 PlatformIO）
cd firmware
pio run --target upload

# 启动 Agent
cd agent
pip install -r requirements.txt
python vitaedge_agent.py

# 微信小程序
# 用微信开发者工具打开 miniapp/ 目录
```

---

## 目录结构

```
VitaEdge/
├── firmware/                   # ESP32-S3 固件
│   ├── src/
│   │   ├── main.cpp            # 主程序：采样 → 推理 → BLE 广播
│   │   ├── sensor/             # MAX30102 + AD8232 驱动
│   │   ├── model/              # TinyML 推理引擎
│   │   └── ble/                # BLE 服务广播
│   └── platformio.ini
│
├── miniapp/                    # 微信小程序
│   ├── pages/
│   │   ├── index/              # 实时监测看板
│   │   ├── history/            # 历史趋势
│   │   ├── agent/              # Agent 对话
│   │   └── family/             # 家人守护
│   └── services/
│       └── ble.js              # BLE 连接 + 数据解析
│
├── agent/
│   └── vitaedge_agent.py       # OpenClaw Agent（心跳/记忆/情商）
│
├── models/                     # TinyML 模型
│   ├── training/               # 训练代码
│   └── tflite/                 # 编译好的 .tflite 文件
│
└── docs/
    └── architecture.md         # 系统架构文档
```

---

## 当前状态

- [x] ESP32-S3 固件框架（PPG + ECG 采集）
- [x] TinyML 推理引擎（血压估算 + HRV + 异常检测）
- [x] BLE 广播协议
- [x] OpenClaw Agent（Heartbeat + 长期记忆 + 情商感知）
- [x] 微信小程序 BLE 数据层
- [x] 分级预警（0–3 级）+ 家人通知
- [ ] TinyML 模型训练与优化（进行中）
- [ ] 小程序 UI 完善
- [ ] 实机联调测试
- [ ] 黑客松现场 Demo

---

## 关于

**David** — 前美团 / 携程研发工程师，硬件极客爱好者。

VitaEdge 源于一个真实的家庭问题：父亲长期高血糖，但有创检测的摩擦让他一拖再拖。我意识到监测的障碍不是意识，是摩擦——于是用工程师的方式来解决它。

**用 Agent 构建 Agent。用极客的方式照顾家人。**

---

## License

MIT
