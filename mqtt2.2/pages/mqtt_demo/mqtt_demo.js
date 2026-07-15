// ========== 常量与工具函数 ==========
const CONFIG = {
  AUTH_INFO: "version=2022-05-01&res=userid%2F538724&et=1815469619&method=md5&sign=rHPrBTm1W1gOe4yW0xS%2BLg%3D%3D",
  PRODUCT_ID: "Id96NmW7s1",
  DEVICE_NAME: "fire",
  API_BASE_URL: "https://iot-api.heclouds.com",
  DATA_REFRESH_INTERVAL: 5000,
};

Page({
  data: {
    status: '未连接',
    statusColor: '#999999',
    isConnected: false,
    logs: [],
    currentMode: 'USR',
    sensorType: 'AD',
    _modeJustSet: false,
    _triggerJustSet: false,
    buzzerFreq: 0,
    lastFlameData: null,
  },

  dataRefreshTimer: null,

  onLoad(options) {
    this.fetchAllDeviceInfo();
    this.startDataRefreshTimer();
    this.setData({
      status: '已连接',
      statusColor: '#07C160',
      isConnected: true
    });
    this.appendLog('系统', '已连接到OneNET平台');
  },

  onUnload() {
    this.clearDataRefreshTimer();
  },

  // ========== OneNET HTTP API ==========

  startDataRefreshTimer() {
    this.clearDataRefreshTimer();
    this.dataRefreshTimer = setInterval(() => {
      this.fetchDeviceProperties();
    }, CONFIG.DATA_REFRESH_INTERVAL);
  },

  clearDataRefreshTimer() {
    if (this.dataRefreshTimer) {
      clearInterval(this.dataRefreshTimer);
      this.dataRefreshTimer = null;
    }
  },

  fetchAllDeviceInfo() {
    this.fetchDeviceProperties();
  },

  async fetchDeviceProperties() {
    const url = `${CONFIG.API_BASE_URL}/thingmodel/query-device-property?product_id=${CONFIG.PRODUCT_ID}&device_name=${CONFIG.DEVICE_NAME}`;
    try {
      const res = await this.wxRequest({ url: url, method: 'GET' });

      if (res.data.code === 0 && res.data.data) {
        this.updateLocalDeviceData(res.data.data);
      } else {
        this.appendLog('API错误', 'code=' + res.data.code + ' msg=' + (res.data.msg || '无'));
      }
    } catch (error) {
      this.appendLog('网络错误', JSON.stringify(error));
    }
  },

  updateLocalDeviceData(remoteDataArray) {
    const updates = {};

    remoteDataArray.forEach(remoteItem => {
      if (remoteItem.identifier === 'fire' || remoteItem.identifier === 'Flame') {
        var fireVal = parseFloat(remoteItem.value);
        if (isNaN(fireVal)) fireVal = 0;
        updates.lastFlameData = '火焰传感器: ' + fireVal;
      }
      if (remoteItem.identifier === 'beep') {
        updates.buzzerFreq = parseInt(remoteItem.value, 10) || 0;
      }
      if (remoteItem.identifier === 'aiuser') {
        var newMode = (remoteItem.value === 'true' || remoteItem.value === '1') ? 'USR' : 'SMART';
        if (this.data._modeJustSet) {
          if (newMode === this.data.currentMode) {
            updates._modeJustSet = false;
          }
        } else {
          updates.currentMode = newMode;
        }
      }
      if (remoteItem.identifier === 'ADIO') {
        var newSensor = (remoteItem.value === 'true' || remoteItem.value === '1') ? 'IO' : 'AD';
        if (this.data._triggerJustSet) {
          if (newSensor === this.data.sensorType) {
            updates._triggerJustSet = false;
          }
        } else {
          updates.sensorType = newSensor;
        }
      }
    });

    if (Object.keys(updates).length > 0) this.setData(updates);
  },

  async setDeviceProperty(param_name, new_value) {
    wx.showLoading({ title: '设置中...', mask: true });
    const requestData = {
      product_id: CONFIG.PRODUCT_ID,
      device_name: CONFIG.DEVICE_NAME,
      params: { [param_name]: new_value }
    };
    try {
      const res = await this.wxRequest({
        url: `${CONFIG.API_BASE_URL}/thingmodel/set-device-property`,
        method: 'POST',
        data: requestData
      });
      wx.hideLoading();
      if (res.data && res.data.code === 0) {
        this.appendLog('下发', param_name + ': ' + new_value);
        wx.showToast({ title: '设置成功', icon: 'success' });
      } else {
        wx.showToast({ title: res.data.msg || '设置失败', icon: 'none', duration: 2000 });
      }
    } catch (error) {
      wx.hideLoading();
      wx.showToast({ title: '网络错误，设置失败', icon: 'none', duration: 2000 });
    }
  },

  wxRequest(options) {
    return new Promise((resolve, reject) => {
      wx.request({
        ...options,
        header: {
          'Authorization': CONFIG.AUTH_INFO,
          'content-type': 'application/json',
          ...options.header,
        },
        success: (res) => resolve(res),
        fail: (err) => reject(err)
      });
    });
  },

  // ========== 原有UI事件处理函数 (底层改为HTTP API调用) ==========

  appendLog: function (topic, message) {
    var logs = this.data.logs || [];
    logs.push({
      time: new Date().toLocaleTimeString(),
      topic: topic,
      message: message
    });
    if (logs.length > 100) logs = logs.slice(-100);
    this.setData({ logs: logs });
  },

  setMode: function (e) {
    var mode = e.currentTarget.dataset.mode;
    var val = mode == 'SMART' ? false : true;
    this.setDeviceProperty('aiuser', val);
    this.setData({ 
      currentMode: mode,
      _modeJustSet: true
    });
    this.appendLog('下发', '切换模式: ' + (mode == 'SMART' ? '智能控制' : '用户控制'));
  },

  setSensorType: function (e) {
    var sensor = e.currentTarget.dataset.sensor;
    var val = sensor == 'IO' ? true : false;
    this.setDeviceProperty('ADIO', val);
    this.setData({ 
      sensorType: sensor,
      _triggerJustSet: true
    });
    this.appendLog('下发', '切换触发类型: ' + (sensor == 'AD' ? 'AD模拟' : 'IO数字'));
  },

  onBuzzerSliderChange: function (e) {
    this.setData({ buzzerFreq: e.detail.value });
  },

  sendBuzzerCmd: function (e) {
    var freq = parseInt(e.currentTarget.dataset.freq);
    this.setDeviceProperty('beep', freq);
    this.setData({ buzzerFreq: freq });
    this.appendLog('下发', '蜂鸣器: ' + (freq > 20 ? freq + 'Hz' : '关闭'));
  },

  clearLogs: function () {
    this.setData({ logs: [] });
  },

  reconnectMQTT: function () {
    this.setData({ logs: [] });
    this.fetchAllDeviceInfo();
    this.startDataRefreshTimer();
    this.setData({
      status: '已连接',
      statusColor: '#07C160',
      isConnected: true
    });
    this.appendLog('系统', '已重新连接OneNET平台');
    wx.showToast({ title: '已重新连接', icon: 'none' });
  },
});