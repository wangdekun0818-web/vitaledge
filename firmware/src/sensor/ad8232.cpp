/**
 * AD8232 ECG心电传感器实现
 */

#include "ad8232.h"

// 引脚定义
#define ECG_PIN 34  // ESP32 ADC引脚
#define LO_P 35     // 左正极
#define LO_N 36     // 左负极

static bool ecg_initialized = false;

bool initAD8232() {
    pinMode(LO_P, INPUT);
    pinMode(LO_N, INPUT);
    pinMode(ECG_PIN, INPUT);
    
    ecg_initialized = true;
    return true;
}

void readECGData() {
    if (!ecg_initialized) return;
    
    // 检查电极连接
    if (digitalRead(LO_P) || digitalRead(LO_N)) {
        // 电极脱落
        return;
    }
    
    // 读取ADC值
    int ecg_value = analogRead(ECG_PIN);
    
    // 这里可以做简单的R峰检测
    // 实际应该用算法检测QRS波群
}

float getECGSignal() {
    if (!ecg_initialized) return 0;
    
    if (digitalRead(LO_P) || digitalRead(LO_N)) {
        return 0; // 电极脱落
    }
    
    return analogRead(ECG_PIN);
}

bool detectRPeak() {
    // 简化版R峰检测
    // 实际应该用Pan-Tompkins算法
    static float last_value = 0;
    static uint32_t last_peak_time = 0;
    
    float current = getECGSignal();
    
    // 简单峰值检测
    if (current > 2000 && last_value < 2000) {
        uint32_t now = millis();
        if (now - last_peak_time > 200) { // 最小RR间期
            last_peak_time = now;
            last_value = current;
            return true;
        }
    }
    
    last_value = current;
    return false;
}
