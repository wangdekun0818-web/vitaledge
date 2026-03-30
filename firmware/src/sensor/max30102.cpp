/**
 * MAX30102 PPG传感器实现
 */

#include "max30102.h"

// 全局变量
float g_heart_rate = 0;
float g_spo2 = 0;

// 内部变量
static bool initialized = false;
static uint8_t ppg_buffer[256];
static int buffer_index = 0;

// I2C写入
void writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(MAX30102_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

// I2C读取
uint8_t readRegister(uint8_t reg) {
    Wire.beginTransmission(MAX30102_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(MAX30102_ADDR, 1);
    return Wire.read();
}

bool initMAX30102() {
    Wire.begin(21, 22); // ESP32-S3 默认I2C引脚
    
    // 检测设备
    uint8_t chip_id = readRegister(0xFF);
    if (chip_id != 0x15) { // MAX30102 chip ID
        Serial.printf("MAX30102未检测到! Chip ID: 0x%02X\n", chip_id);
        return false;
    }
    
    // 软复位
    writeRegister(REG_MODE_CONFIG, 0x40);
    delay(100);
    
    // 配置FIFO
    writeRegister(REG_FIFO_CONFIG, 0x0F); // 几乎不清空
    
    // 配置LED脉冲
    writeRegister(REG_LIGHT_CONFIG, 0x24); // IR脉冲
    
    // 配置模式：心率模式
    writeRegister(REG_MODE_CONFIG, 0x02);
    
    // 启用中断
    writeRegister(REG_INTR_ENABLE_1, 0xC0); // FIFO almost full & new data ready
    
    initialized = true;
    return true;
}

void readPPGData() {
    if (!initialized) return;
    
    // 读取FIFO数据
    uint8_t read_ptr = readRegister(REG_FIFO_RD_PTR);
    uint8_t write_ptr = readRegister(REG_FIFO_WR_PTR);
    
    int samples_to_read = (write_ptr - read_ptr + 32) % 32;
    if (samples_to_read == 0) return;
    
    // 读取FIFO
    Wire.beginTransmission(MAX30102_ADDR);
    Wire.write(REG_FIFO_DATA);
    Wire.endTransmission(false);
    
    int bytes_to_read = samples_to_read * 3; // 每样本3字节(IR)
    for (int i = 0; i < bytes_to_read && i < 256; i++) {
        ppg_buffer[buffer_index++] = Wire.read();
        if (buffer_index >= 256) buffer_index = 0;
    }
    
    // 简单的心率计算（演示用）
    // 实际应该用峰值检测算法
    static uint32_t lastbeat = 0;
    static float beats = 0;
    
    uint32_t now = millis();
    if (now - lastbeat > 300 && now - lastbeat < 2000) {
        beats++;
        g_heart_rate = beats * 60000.0f / (now - lastbeat);
        if (g_heart_rate < 40 || g_heart_rate > 200) {
            g_heart_rate = 70; // 默认值
        }
    }
    if (now - lastbeat > 2000) {
        beats = 0;
    }
    lastbeat = now;
}

int getPPGBuffer(float* buffer, int max_len) {
    int copy_len = min(buffer_index, max_len);
    for (int i = 0; i < copy_len; i++) {
        buffer[i] = ppg_buffer[i];
    }
    return copy_len;
}

float calculateHeartRate(const float* ppg_data, int len) {
    // 简化版心率计算
    // 实际应该用峰值检测算法(如elapschitz算法)
    return g_heart_rate;
}

float calculateSpO2(const float* ir_data, const float* red_data, int len) {
    // 简化版血氧计算
    // 实际应该用R/IR比值查表
    return 98.0f; // 默认值
}
