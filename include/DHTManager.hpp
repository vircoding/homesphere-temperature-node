#pragma once
#include <DHT.h>

class DHTManager {
 public:
  struct Data {
    float hum = NAN;
    float temp = NAN;
  };

  DHTManager(const gpio_num_t dataPin, Data& data);
  void read();

 private:
  DHT _dht;
  Data& _data;
};