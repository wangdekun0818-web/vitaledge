/**
 * BLE 广播服务
 * 
 * 功能：将健康数据通过BLE广播到手机
 * 特点：低功耗，数据加密
 */

#ifndef BLE_SERVICE_H
#define BLE_SERVICE_H

#include <Arduino.h>

// 健康数据包
struct HealthDataPacket {
    float heart_rate;       // 心率 bpm
    float spo2;             // 血氧 %
    float hrv;             // 心率变异性 ms
    float bp_systolic;      // 收缩压 mmHg
    float bp_diastolic;     // 舒张压 mmHg
    float glucose;          // 血糖估算 mmol/L
    float stress_index;     // 压力指数 0-100
    uint8_t anomaly_level;  // 异常等级 0-3
    uint8_t battery;        // 电量 %
    uint32_t timestamp;     // 时间戳 ms
} __attribute__((packed));

// 函数声明
void initBLEService();
void sendBLEPacket(const HealthDataPacket& data);
uint8_t getBatteryLevel();

#endif
