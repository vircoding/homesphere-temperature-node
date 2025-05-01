#pragma once

#include <Arduino.h>
#include <esp_now.h>

class NowManager {
 public:
  struct TemperatureData {
    float hum;
    float temp;
    int node_id;
  };

  NowManager(const uint8_t masterMac[6]);
  bool init();
  void onSend(esp_now_send_cb_t callback);
  bool registerMasterPeer();
  bool sendTemperatureData(const float temp, const float hum);
  static String getMacStr(const uint8_t mac[6]);

 private:
  uint8_t _masterMac[6];
};