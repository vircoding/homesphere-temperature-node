#pragma once

#include <ArduinoJson.h>

class ConfigManager {
 public:
  struct NetworkConfig {
    String ssid;
    String password;
  };

  bool init();

  NetworkConfig getMeshConfig() const { return _meshConfig; }

 private:
  NetworkConfig _meshConfig;

  bool _loadConfig();
};