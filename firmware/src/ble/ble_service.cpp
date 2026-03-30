/**
 * BLE 广播服务实现
 * 
 * 使用 NimBLE 库实现低功耗广播
 */

#include "ble_service.h"
#include <NimBLEDevice.h>

// BLE广播设备名称
#define DEVICE_NAME "VitaEdge_001"
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// BLE Advertising
static NimBLEAdvertising* pAdvertising;
static uint8_t bleConnected = 0;

// 健康数据特征
static NimBLECharacteristic* pHealthCharacteristic;

class HealthServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        bleConnected = 1;
        Serial.println("[BLE] 客户端已连接");
    }
    void onDisconnect(BLEServer* pServer) {
        bleConnected = 0;
        Serial.println("[BLE] 客户端已断开");
    }
};

void initBLEService() {
    // 初始化BLE
    NimBLEDevice::init(DEVICE_NAME);
    
    // 创建BLE服务器
    BLEServer* pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new HealthServerCallbacks());
    
    // 创建服务
    BLEService* pService = pServer->createService(SERVICE_UUID);
    
    // 创建健康数据特征
    pHealthCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ | 
        NIMBLE_PROPERTY::NOTIFY
    );
    pHealthCharacteristic->setValue("VitaEdge");
    pHealthCharacteristic->notify();
    
    // 启动服务
    pService->start();
    
    // 配置广播
    pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    
    // 开始广播
    NimBLEDevice::startAdvertising();
    
    Serial.printf("[BLE] 广播启动: %s\n", DEVICE_NAME);
    Serial.println("[BLE] Service UUID: " SERVICE_UUID);
}

void sendBLEPacket(const HealthDataPacket& data) {
    if (!bleConnected) {
        return; // 没连接不发送，节省电量
    }
    
    // 将数据打包为字节数组
    uint8_t packet[sizeof(HealthDataPacket)];
    memcpy(packet, &data, sizeof(HealthDataPacket));
    
    // 发送通知
    pHealthCharacteristic->setValue(packet, sizeof(packet));
    pHealthCharacteristic->notify();
}

uint8_t getBatteryLevel() {
    // 简化实现，实际应该读取ADC
    return 85; // 模拟电量85%
}
