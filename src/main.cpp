#include <Arduino.h>
#include <Wire.h>
#include <string>

#include "Comms.hpp"



#define MUTEX_PERMIT_TO_EXIST 1
#define MUTEX_USE_OUTSIDE_OF_COMMS 1
// #define MUTEX_USE_INSIDE_OF_COMMS 1 // enabling this will make OTA updates fail most of the time

#ifdef MUTEX_PERMIT_TO_EXIST
SemaphoreHandle_t mutex_i2c = NULL;
#endif

TaskHandle_t taskHandle__handleComms = NULL;
void task__handleComms(void * parameters) {
  for (;;) {
    vTaskDelay(30000 / portTICK_PERIOD_MS);

    Serial.println("WIFI: About to connect");
    Comms::instance().wifi_connect();

#ifdef MUTEX_USE_INSIDE_OF_COMMS
    xSemaphoreTake(mutex_i2c, portMAX_DELAY);
#endif

    Comms::instance().ota_perform_update();

#ifdef MUTEX_USE_INSIDE_OF_COMMS
    xSemaphoreGive(mutex_i2c);
#endif

    Serial.println("FIRMWARE: Should never get here");
    Comms::instance().wifi_disconnect();
    delay(100);
  }
}



TaskHandle_t taskHandle__heartbeatLogging = NULL;
void task__heartbeatLogging(void * parameters) {
  for (;;) {
        
#ifdef MUTEX_USE_OUTSIDE_OF_COMMS
    xSemaphoreTake(mutex_i2c, portMAX_DELAY);
#endif

    Serial.print("HEARTBEAT: ");
    Serial.println( millis() );

#ifdef MUTEX_USE_OUTSIDE_OF_COMMS
    xSemaphoreGive(mutex_i2c);
#endif

    vTaskDelay(1000 / portTICK_PERIOD_MS);

  }
}


void setup() {

  Serial.begin(115200);
  while (!Serial);
  Serial.println("SERIAL: Setup complete");

#ifdef MUTEX_PERMIT_TO_EXIST
  mutex_i2c = xSemaphoreCreateMutex();
  if (mutex_i2c == NULL) { 
    Serial.println("Mutex can not be created"); 
  }
#endif

  xTaskCreatePinnedToCore(
    task__handleComms,
    "task__handleComms",
    100000,                          // stack size… how to calculate this?
    NULL,                            // parameters
    10,                              // priority, higher number is higher priority
    &taskHandle__handleComms,        // task handle, which would need to be global scope
    CONFIG_ARDUINO_RUNNING_CORE      // Wi-Fi tasks must run on same core and Arduino
  );
  Serial.println("COMMS: task__handleComms created");

  xTaskCreate(
    task__heartbeatLogging,
    "task__heartbeatLogging",
    1000,                            // stack size… how to calculate this?
    NULL,                            // parameters
    1,                               // priority, higher number is higher priority
    &taskHandle__heartbeatLogging    // task handle, which would need to be global scope
  );
  Serial.println("HEARTBEAT: task__handleComms created");

}



void loop() {
  vTaskDelete(NULL);
}
