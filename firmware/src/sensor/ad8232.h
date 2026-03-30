/**
 * AD8232 ECG心电传感器驱动
 * 
 * 功能：采集ECG信号，用于心律失常检测和HRV分析
 */

#ifndef AD8232_H
#define AD8232_H

#include <Arduino.h>

// 函数声明
bool initAD8232();
void readECGData();
float getECGSignal();
bool detectRPeak();

#endif
