/**
 * MAX30102 PPG传感器驱动
 * 
 * 功能：采集PPG信号，用于心率、血氧、血压估算
 */

#ifndef MAX30102_H
#define MAX30102_H

#include <Arduino.h>
#include <Wire.h>

// MAX30102 I2C地址
#define MAX30102_ADDR 0x57

// 寄存器地址
#define REG_INTR_STATUS_1 0x00
#define REG_INTR_STATUS_2 0x01
#define REG_INTR_ENABLE_1 0x02
#define REG_INTR_ENABLE_2 0x03
#define REG_FIFO_WR_PTR 0x04
#define REG_FIFO_OVF_PTR 0x05
#define REG_FIFO_RD_PTR 0x06
#define REG_FIFO_DATA 0x07
#define REG_FIFO_CONFIG 0x08
#define REG_MODE_CONFIG 0x09
#define REG_LIGHT_CONFIG 0x0C
#define REG_PILOT_PA 0x0D
#define REG_MULTILED_CTRL_1 0x11
#define REG_MULTILED_CTRL_2 0x12
#define REG_TEMP 0x1F
#define REG_TEMP_CONFIG 0x21
#define REG_PROX_INT_THRESH 0x30

// 全局变量
extern float g_heart_rate;
extern float g_spo2;

// 函数声明
bool initMAX30102();
void readPPGData();
int getPPGBuffer(float* buffer, int max_len);
float calculateHeartRate(const float* ppg_data, int len);
float calculateSpO2(const float* ir_data, const float* red_data, int len);

#endif
