#pragma once

#include <Arduino.h>

class IndicatorManager {
 public:
  IndicatorManager(const uint8_t ledPin);
  void begin();
  void set(const bool status);

 private:
  const uint8_t _LED_PIN;
};