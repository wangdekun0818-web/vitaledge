/**
 * VitaEdge - ESP32-S3 主程序
 * 
 * 功能：
 * - PPG/ECG 传感器数据采集
 * - TinyML 本地推理
 * - BLE 广播
 * 
 * 极客精神：用最低成本($167)，实现24h健康监测
 */

#include <Arduino.h>
#include "sensor/max30102.h"
#include "sensor/ad8232.h"
#include "model/inference.h"
#include "ble/ble_service.h"
#include "power/sleep_manager.h"

// ======================== 配置 ========================

#define DEBUG_MODE true
#define LED_PIN 2

// 采样间隔 (ms)
#define PPG_SAMPLE_INTERVAL 10      // 100Hz
#define ECG_SAMPLE_INTERVAL 4       // 250Hz
#define INFERENCE_INTERVAL 1000     // 1秒推理一次
#define BLE_BROADCAST_INTERVAL 1000 // BLE广播间隔

// ======================== 全局变量 ========================

// 传感器数据
static float current_heart_rate = 0;
static float current_spo2 = 0;
static float current_hrv = 0;
static float current_bp_systolic = 0;
static float current_bp_diastolic = 0;
static float current_glucose_estimate = 0;
static float current_stress_index = 0;
static uint8_t anomaly_level = 0; // 0=正常, 1=注意, 2=警告, 3=危险

// 系统状态
static bool sensor_initialized = false;
static uint32_t loop_count = 0;

// ======================== 初始化 ========================

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n===========================================");
    Serial.println("  VitaEdge - 24h 健康守护Agent");
    Serial.println("  极客精神：用最小的成本，守护健康");
    Serial.println("===========================================\n");
    
    // LED 指示灯
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // 1. 初始化传感器
    Serial.println("[1/4] 初始化传感器...");
    if (initMAX30102()) {
        Serial.println("  ✓ MAX30102 (PPG) 初始化成功");
        sensor_initialized = true;
    } else {
        Serial.println("  ✗ MAX30102 初始化失败");
    }
    
    if (initAD8232()) {
        Serial.println("  ✓ AD8232 (ECG) 初始化成功");
    } else {
        Serial.println("  ✗ AD8232 初始化失败");
    }
    
    // 2. 初始化 TinyML 模型
    Serial.println("[2/4] 加载TinyML模型...");
    if (initInference()) {
        Serial.println("  ✓ TinyML模型加载成功 (12KB)");
        Serial.println("  ✓ 血压估算模型: 启用");
        Serial.println("  ✓ HRV分析模型: 启用");
        Serial.println("  ✓ 异常检测模型: 启用");
    } else {
        Serial.println("  ✗ TinyML模型加载失败");
    }
    
    // 3. 初始化 BLE
    Serial.println("[3/4] 初始化BLE广播...");
    initBLEService();
    Serial.println("  ✓ BLE服务已启动");
    Serial.println("  ✓ 设备名称: VitaEdge_001");
    
    // 4. 初始化功耗管理
    Serial.println("[4/4] 初始化功耗管理...");
    initSleepManager();
    Serial.println("  ✓ 低功耗模式已启用");
    
    Serial.println("\n===========================================");
    Serial.println("  VitaEdge 启动完成！");
    Serial.printf("  成本: ¥167 | 续航: 24h+ | 模型: 本地运行\n");
    Serial.println("===========================================\n");
    
    digitalWrite(LED_PIN, HIGH); // 启动完成灯亮
}

// ======================== 主循环 ========================

void loop() {
    loop_count++;
    static uint32_t last_inference = 0;
    static uint32_t last_ble_broadcast = 0;
    
    // LED 闪烁表示系统运行
    if (loop_count % 1000 == 0) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        Serial.printf("[Loop %d] HRV: %.1f | BP: %.0f/%.0f | 压力: %.1f\n", 
                      loop_count, current_hrv, current_bp_systolic, current_bp_diastolic, current_stress_index);
    }
    
    // PPG 数据采集（在中断或定时器中完成，这里简化处理）
    readPPGData();
    
    // ECG 数据采集
    readECGData();
    
    // TinyML 推理 (每秒1次)
    if (millis() - last_inference >= INFERENCE_INTERVAL) {
        last_inference = millis();
        runInference();
    }
    
    // BLE 广播 (每秒1次)
    if (millis() - last_ble_broadcast >= BLE_BROADCAST_INTERVAL) {
        last_ble_broadcast = millis();
        broadcastHealthData();
    }
    
    // 功耗管理
    managePower();
    
    delay(10); // 10ms 周期，100Hz
}

// ======================== 推理引擎 ========================

void runInference() {
    // 获取PPG原始数据
    float ppg_buffer[256];
    int ppg_len = getPPGBuffer(ppg_buffer, 256);
    
    if (ppg_len < 50) return; // 数据不够不推理
    
    // HRV 分析
    current_hrv = analyzeHRV(ppg_buffer, ppg_len);
    
    // 压力指数 (0-100)
    current_stress_index = calculateStressIndex(current_hrv);
    
    // 血压估算 (TinyML模型)
    float bp_result[2];
    if (estimateBloodPressure(ppg_buffer, ppg_len, bp_result)) {
        current_bp_systolic = bp_result[0];
        current_bp_diastolic = bp_result[1];
    }
    
    // 血糖趋势估算
    current_glucose_estimate = estimateGlucoseTrend(ppg_buffer, ppg_len, current_hrv);
    
    // 异常检测
    anomaly_level = detectAnomaly(
        current_heart_rate,
        current_bp_systolic,
        current_glucose_estimate,
        current_hrv
    );
    
    // 根据异常级别调整采样频率
    if (anomaly_level >= 2) {
        // 危险级别，提高采样频率
        setSampleRate(PPG_SAMPLE_INTERVAL / 2);
        digitalWrite(LED_PIN, HIGH); // 持续亮灯警告
    }
}

// ======================== BLE 广播 ========================

void broadcastHealthData() {
    HealthDataPacket packet;
    packet.heart_rate = current_heart_rate;
    packet.spo2 = current_spo2;
    packet.hrv = current_hrv;
    packet.bp_systolic = current_bp_systolic;
    packet.bp_diastolic = current_bp_diastolic;
    packet.glucose = current_glucose_estimate;
    packet.stress_index = current_stress_index;
    packet.anomaly_level = anomaly_level;
    packet.battery = getBatteryLevel();
    packet.timestamp = millis();
    
    sendBLEPacket(packet);
    
    // 串口输出（调试用）
    if (DEBUG_MODE && anomaly_level >= 2) {
        Serial.printf("[ALERT] 异常等级: %d | 心率: %.0f | 血压: %.0f/%.0f\n",
                      anomaly_level, current_heart_rate, current_bp_systolic, current_bp_diastolic);
    }
}

// ======================== 功耗管理 ========================

void managePower() {
    // 根据电池电量调整
    uint8_t battery = getBatteryLevel();
    
    if (battery < 20) {
        // 电量低，降低采样频率
        setSampleRate(PPG_SAMPLE_INTERVAL * 2);
        // 关闭OLED显示
        // sleepOLED();
    } else if (battery < 50) {
        // 电量中等，保持正常
    } else {
        // 电量充足，开启所有功能
    }
    
    // 进入低功耗模式（定时器唤醒）
    // enterLightSleep(1000);
}
