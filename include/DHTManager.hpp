#pragma once
#include <DHT.h>

class DHTManager {
 public:
  struct Data {
    float hum;
    float temp;
  };

  DHTManager(uint8_t dataPin, Data& data);
  void read();

 private:
  DHT _dht;
  Data& _data;
};