#include <freertos/FreeRTOS.h>
#include <painlessMesh.h>

#define MESH_PREFIX "HomeSphereMesh"
#define MESH_PASSWORD "123456789"
#define MESH_PORT 5555

Scheduler scheduler;
painlessMesh mesh;

void sendMessage() {
  String msg = "Hi from node 2 ";
  msg += mesh.getNodeId();
  mesh.sendBroadcast(msg);
}

void taskMeshUpdate(void* parameter) {
  while (1) {
    mesh.update();

    // Delay - 10ms
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void taskSendMsg(void* parameter) {
  while (1) {
    sendMessage();

    vTaskDelay(pdMS_TO_TICKS(3000));
  }
}

void setup() {
  Serial.begin(115200);

  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA);

  xTaskCreatePinnedToCore(taskMeshUpdate, "Mesh Update", 4096, NULL, 1, NULL,
                          0);
  xTaskCreatePinnedToCore(taskSendMsg, "Send Message", 4096, NULL, 1, NULL, 0);
}

void loop() {}