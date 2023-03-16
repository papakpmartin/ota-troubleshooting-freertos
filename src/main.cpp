#include <Arduino.h>
#include <Wire.h>
#include <string>

#include "Comms.hpp"


#define SDA1 GPIO_NUM_33
#define SCL1 GPIO_NUM_34

#define SDA2 GPIO_NUM_35
#define SCL2 GPIO_NUM_36

TwoWire I2C_MAIN = TwoWire(0);
TwoWire I2C_EXT = TwoWire(1);

#define MUTEX_PERMIT_TO_EXIT 1
#define MUTEX_USE 1 // enabling this will make OTA updates fail most of the time

#ifdef MUTEX_PERMIT_TO_EXIT
SemaphoreHandle_t mutex_i2c = NULL;
#endif

TaskHandle_t taskHandle__handleComms = NULL;
void task__handleComms(void * parameters) {
  for (;;) {
    vTaskDelay(30000 / portTICK_PERIOD_MS);

      Serial.println("WIFI: About to connect");
      Comms::instance().wifi_connect();

      Serial.println("MQTT: About to connect");
      Comms::instance().mqtt_connect();
      
      Serial.print(F("FIRMWARE: Desired version different than current? "));
      if (Comms::instance().ota_update_required())
      {
        Serial.print(Comms::instance().m_desiredFirmwareVersion);
        Serial.print(F(" != "));
        Serial.println(Comms::instance().m_firmwareVersion);
        Serial.println("FIRMWARE: Beginning OTA update process");
        
#ifdef MUTEX_USE
        xSemaphoreTake(mutex_i2c, portMAX_DELAY);
#endif

        Comms::instance().ota_perform_update();

#ifdef MUTEX_USE
        xSemaphoreGive(mutex_i2c);
#endif

        Serial.println("FIRMWARE: Should never get here");

      } else
      {
        Serial.print("FIRMWARE: Update not needed: ");
        Serial.print(Comms::instance().m_desiredFirmwareVersion);
        Serial.print(F(" == "));
        Serial.println(Comms::instance().m_firmwareVersion);
      }
      Serial.println("FIRMWARE: Firmware block completed");

      Comms::instance().mqtt_disconnect();
      delay(100);
      Comms::instance().wifi_disconnect();
      delay(100);
  }
}



void setup() {

  Serial.begin(115200);
  while (!Serial);
  Serial.println("SERIAL: Setup complete");

  I2C_MAIN.setPins(SDA2, SCL2);
  I2C_MAIN.begin();
  Serial.println("I2C_MAIN: Setup complete");
  I2C_EXT.setPins(SDA1, SCL1); 
  I2C_EXT.begin(); 
  Serial.println("I2C_EXT: Setup complete");

#ifdef MUTEX_PERMIT_TO_EXIT
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
  Serial.println("COMMS: handleOTA created");

}



void loop() {
  vTaskDelete(NULL);
}
