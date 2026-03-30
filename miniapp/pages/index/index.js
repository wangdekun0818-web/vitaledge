/**
 * VitaEdge - 首页（实时监测）
 */

const bleService = require('../../services/ble.js')

Page({
  data: {
    // 连接状态
    connected: false,
    deviceName: 'VitaEdge_001',
    
    // 健康数据
    heartRate: '--',
    spO2: '--',
    hrv: '--',
    bpSystolic: '--',
    bpDiastolic: '--',
    glucose: '--',
    stressIndex: 0,
    stressText: '正常',
    
    // 状态颜色
    statusColor: '#4CAF50',
    
    // 最后更新时间
    lastUpdate: '',
    
    // 设备电量
    battery: 85
  },
  
  onLoad() {
    this.updateHealthData = this.updateHealthData.bind(this)
  },
  
  onShow() {
    // 自动连接
    if (!this.data.connected) {
      this.connectDevice()
    }
  },
  
  /**
   * 连接设备
   */
  connectDevice() {
    wx.showLoading({ title: '连接中...' })
    
    bleService.connectDevice()
      .then(() => {
        this.setData({ connected: true })
      })
      .catch((err) => {
        console.error('连接失败:', err)
        wx.showToast({ title: '连接失败', icon: 'error' })
      })
      .finally(() => {
        wx.hideLoading()
      })
  },
  
  /**
   * 更新健康数据（由BLE服务调用）
   */
  updateHealthData(data) {
    const stressText = this.getStressText(data.stressIndex)
    const statusColor = this.getStatusColor(data.anomalyLevel)
    
    this.setData({
      heartRate: data.heartRate.toFixed(0),
      spO2: data.spO2.toFixed(0),
      hrv: data.hrv.toFixed(0),
      bpSystolic: data.bpSystolic.toFixed(0),
      bpDiastolic: data.bpDiastolic.toFixed(0),
      glucose: data.glucose.toFixed(1),
      stressIndex: data.stressIndex,
      stressText,
      statusColor,
      battery: data.battery || 85,
      lastUpdate: new Date().toLocaleTimeString()
    })
  },
  
  /**
   * 获取压力状态文字
   */
  getStressText(score) {
    if (score < 20) return '非常放松 😌'
    if (score < 40) return '状态良好 🙂'
    if (score < 60) return '略有压力 😐'
    if (score < 80) return '压力较大 😰'
    return '极度紧张 😱'
  },
  
  /**
   * 获取状态颜色
   */
  getStatusColor(level) {
    switch(level) {
      case 0: return '#4CAF50' // 绿色
      case 1: return '#FFC107'  // 黄色
      case 2: return '#FF9800'  // 橙色
      case 3: return '#F44336' // 红色
      default: return '#4CAF50'
    }
  },
  
  /**
   * 断开连接
   */
  disconnect() {
    wx.closeBLEConnection({
      deviceId: bleService.deviceId,
      success: () => {
        this.setData({ connected: false })
      }
    })
  },
  
  /**
   * 刷新数据
   */
  refresh() {
    this.connectDevice()
  }
})
