/**
 * TinyML 推理引擎
 * 
 * 极客精神：
 * - 模型跑在ESP32上，不是云端
 * - 总模型大小 < 25KB
 * - 推理延迟 < 50ms
 */

#ifndef INFERENCE_H
#define INFERENCE_H

#include <Arduino.h>

// 函数声明

// 初始化推理引擎
bool initInference();

// HRV分析
float analyzeHRV(const float* ppg_data, int len);

// 计算压力指数 (0-100)
float calculateStressIndex(float hrv);

// 血压估算
bool estimateBloodPressure(const float* ppg_data, int len, float* result);

// 血糖趋势估算
float estimateGlucoseTrend(const float* ppg_data, int len, float hrv);

// 异常检测
// 返回: 0=正常, 1=注意, 2=警告, 3=危险
uint8_t detectAnomaly(float hr, float bp_sys, float glucose, float hrv);

// 设置采样率
void setSampleRate(uint32_t ms);

#endif
