/**
 * VitaEdge 微信小程序 - 服务层
 * 
 * 功能：BLE通信、数据处理、Agent接口
 */

// 设备UUID
const DEVICE_NAME = 'VitaEdge_001'
const SERVICE_UUID = '4fafc201-1fb5-459e-8fcc-c5c9c331914b'
const CHARACTERISTIC_UUID = 'beb5483e-36e1-4688-b7f5-ea07361b26a8'

// 全局状态
let device = null
let characteristic = null
let connected = false

// 健康数据缓存
let healthData = {
  heartRate: 0,
  spO2: 0,
  hrv: 0,
  bpSystolic: 0,
  bpDiastolic: 0,
  glucose: 0,
  stressIndex: 0,
  anomalyLevel: 0,
  timestamp: 0
}

// 历史数据
let historyData = []

// ======================== BLE连接 ========================

/**
 * 连接VitaEdge设备
 */
function connectDevice() {
  return new Promise((resolve, reject) => {
    wx.showLoading({ title: '连接设备...' })
    
    wx.openBluetoothAdapter({
      success: (res) => {
        // 搜索设备
        wx.startBluetoothDevicesDiscovery({
          services: [SERVICE_UUID],
          success: (res) => {
            // 等待找到设备
            setTimeout(() => {
              wx.getBluetoothDevices({
                success: (res) => {
                  const device = res.devices.find(d => d.name === DEVICE_NAME)
                  if (device) {
                    connectToDevice(device.deviceId)
                      .then(resolve)
                      .catch(reject)
                  } else {
                    wx.hideLoading()
                    wx.showToast({ title: '未找到设备', icon: 'error' })
                    reject('设备未找到')
                  }
                }
              })
            }, 2000)
          },
          fail: reject
        })
      },
      fail: (err) => {
        wx.showToast({ title: '蓝牙未开启', icon: 'error' })
        reject(err)
      }
    })
  })
}

/**
 * 连接指定设备
 */
function connectToDevice(deviceId) {
  return new Promise((resolve, reject) => {
    wx.createBLEConnection({
      deviceId,
      success: () => {
        // 获取服务
        wx.getBLEDeviceServices({
          deviceId,
          success: (res) => {
            const service = res.services.find(s => s.uuid === SERVICE_UUID)
            if (service) {
              // 获取特征值
              wx.getBLEDeviceCharacteristics({
                deviceId,
                serviceId: service.uuid,
                success: (res) => {
                  const char = res.characteristics.find(c => c.uuid === CHARACTERISTIC_UUID)
                  if (char) {
                    device = deviceId
                    characteristic = char
                    connected = true
                    
                    // 开启监听
                    wx.onBLECharacteristicChange((res) => {
                      parseHealthData(res.value)
                    })
                    
                    // 启用通知
                    wx.notifyBLECharacteristicValueChange({
                      deviceId,
                      serviceId: service.uuid,
                      characteristicId: char.uuid,
                      state: true
                    })
                    
                    wx.hideLoading()
                    wx.showToast({ title: '连接成功' })
                    resolve()
                  } else {
                    reject('特征值未找到')
                  }
                },
                fail: reject
              })
            } else {
              reject('服务未找到')
            }
          },
          fail: reject
        })
      },
      fail: reject
    })
  })
}

// ======================== 数据解析 ========================

/**
 * 解析BLE传来的健康数据
 */
function parseHealthData(buffer) {
  const view = new DataView(buffer)
  
  healthData = {
    heartRate: view.getFloat32(0, true),
    spO2: view.getFloat32(4, true),
    hrv: view.getFloat32(8, true),
    bpSystolic: view.getFloat32(12, true),
    bpDiastolic: view.getFloat32(16, true),
    glucose: view.getFloat32(20, true),
    stressIndex: view.getFloat32(24, true),
    anomalyLevel: view.getUint8(28),
    timestamp: Date.now()
  }
  
  // 添加到历史
  historyData.push({ ...healthData })
  if (historyData.length > 1000) {
    historyData.shift()
  }
  
  // 触发更新
  updateHealthDisplay()
  
  // 检查异常
  checkAnomaly()
}

// ======================== 异常检测 ========================

function checkAnomaly() {
  const { anomalyLevel, heartRate, bpSystolic, glucose } = healthData
  
  if (anomalyLevel === 0) return
  
  // 紧急预警
  if (anomalyLevel >= 3) {
    wx.showModal({
      title: '⚠️ 紧急预警',
      content: `检测到生命体征异常！
心率: ${heartRate.toFixed(0)} bpm
血压: ${bpSystolic.toFixed(0)}/${healthData.bpDiastolic.toFixed(0)} mmHg
血糖: ${glucose.toFixed(1)} mmol/L`,
      showCancel: false,
      confirmText: '我知道了'
    })
    
    // 通知家人
    notifyFamily(healthData)
  }
}

// ======================== 家人通知 ========================

/**
 * 通知家人
 */
function notifyFamily(data) {
  wx.request({
    url: 'https://api.vitaedge.com/notify',
    method: 'POST',
    data: {
      type: 'emergency',
      userId: getApp().globalData.userId,
      healthData: data,
      timestamp: Date.now()
    },
    success: (res) => {
      console.log('家人通知已发送')
    }
  })
}

// ======================== 页面更新 ========================

function updateHealthDisplay() {
  const pages = getCurrentPages()
  const currentPage = pages[pages.length - 1]
  
  if (currentPage && currentPage.updateHealthData) {
    currentPage.updateHealthData(healthData)
  }
}

// ======================== 导出 ========================

module.exports = {
  connectDevice,
  healthData,
  historyData,
  notifyFamily
}
