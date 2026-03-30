/**
 * VitaEdge - 家人守护页面
 * 
 * 功能：
 * - 查看被监护人健康数据
 * - 接收分级预警通知
 * - SOS紧急求助
 */

Page({
  data: {
    // 家人列表
    familyMembers: [],
    
    // 被监护人数据
   监护人: {
      name: '父亲',
      avatar: '/assets/avatar-father.png',
      status: 'online',
      lastUpdate: '刚刚',
      healthData: {
        heartRate: 72,
        bpSystolic: 125,
        bpDiastolic: 82,
        glucose: 5.9,
        anomalyLevel: 0
      }
    },
    
    // 紧急联系人
    emergencyContacts: [
      { name: '母亲', phone: '138****8888', relation: '配偶' },
      { name: '姐姐', phone: '139****9999', relation: '姐妹' }
    ],
    
    // SOS状态
    sosActive: false,
    sosCountdown: 0
  },
  
  onLoad() {
    // 加载家人列表
    this.loadFamilyMembers()
    
    // 监听预警通知
    this.listenForAlerts()
  },
  
  /**
   * 加载家人列表
   */
  loadFamilyMembers() {
    // 从服务器获取
    wx.request({
      url: 'https://api.vitaedge.com/family/list',
      method: 'GET',
      success: (res) => {
        if (res.data.code === 0) {
          this.setData({ familyMembers: res.data.data })
        }
      }
    })
  },
  
  /**
   * 监听预警通知
   */
  listenForAlerts() {
    // 订阅消息推送
    wx.onPushMessage((res) => {
      const data = JSON.parse(res.data)
      if (data.type === 'alert') {
        this.handleAlert(data)
      } else if (data.type === 'sos') {
        this.handleSOS(data)
      }
    })
  },
  
  /**
   * 处理预警
   */
  handleAlert(alert) {
    const level = alert.anomalyLevel
    const icon = level >= 3 ? '🔴' : level >= 2 ? '🟠' : '🟡'
    const title = level >= 3 ? '紧急预警' : level >= 2 ? '重要提醒' : '健康通知'
    
    wx.showModal({
      title: `${icon} ${title} - ${alert.memberName}`,
      content: `心率: ${alert.heartRate} bpm
血压: ${alert.bpSystolic}/${alert.bpDiastolic} mmHg
血糖: ${alert.glucose} mmol/L

${alert.message || '点击查看详情'}`,
      confirmText: '查看详情',
      cancelText: '稍后',
      success: (res) => {
        if (res.confirm) {
          this.viewMemberDetail(alert.memberId)
        }
      }
    })
  },
  
  /**
   * 处理SOS
   */
  handleSOS(sos) {
    wx.showModal({
      title: '🆘 SOS紧急求助',
      content: `${sos.memberName} 发出了紧急求助！
      
位置: ${sos.location || '未知'}
时间: ${new Date(sos.timestamp).toLocaleString()}
      
立即联系或拨打120？`,
      confirmText: '立即联系',
      cancelText: '查看详情',
      success: (res) => {
        if (res.confirm) {
          this.callEmergency(sos.memberId)
        }
      }
    })
  },
  
  /**
   * 发起SOS
   */
  triggerSOS() {
    wx.showModal({
      title: '🆘 确认发送SOS',
      content: '将向所有家人发送您的位置和健康数据，并拨打120',
      confirmText: '确认SOS',
      cancelText: '取消',
      success: (res) => {
        if (res.confirm) {
          this.doSOS()
        }
      }
    })
  },
  
  /**
   * 执行SOS
   */
  doSOS() {
    // 获取位置
    wx.getLocation({
      success: (res) => {
        this.sendSOS(res.latitude, res.longitude)
      },
      fail: () => {
        // 没有位置也发送
        this.sendSOS(null, null)
      }
    })
  },
  
  /**
   * 发送SOS
   */
  sendSOS(lat, lng) {
    wx.request({
      url: 'https://api.vitaedge.com/sos',
      method: 'POST',
      data: {
        userId: getApp().globalData.userId,
        location: lat && lng ? { lat, lng } : null,
        healthData: getApp().globalData.currentHealthData,
        timestamp: Date.now()
      },
      success: (res) => {
        wx.showToast({
          title: 'SOS已发送',
          icon: 'success'
        })
        
        // 自动拨打120
        wx.makePhoneCall({
          phoneNumber: '120',
          fail: () => {
            wx.showToast({
              title: '请手动拨打120',
              icon: 'error'
            })
          }
        })
      }
    })
  },
  
  /**
   * 紧急呼叫家人
   */
  callFamily(memberId) {
    const member = this.data.familyMembers.find(m => m.id === memberId)
    if (member && member.phone) {
      wx.makePhoneCall({
        phoneNumber: member.phone
      })
    }
  },
  
  /**
   * 查看家人详情
   */
  viewMemberDetail(memberId) {
    wx.navigateTo({
      url: `/pages/family/detail?id=${memberId}`
    })
  }
})
