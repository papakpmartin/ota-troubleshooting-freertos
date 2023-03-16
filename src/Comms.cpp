#include <Arduino.h>

// Our stuff
#include "Comms.hpp"

//-----------------------------------------------------------------------
//  Singleton accessor
//
Comms &Comms::instance()
{
    // Hidden static instance.
    // ...Allocated at initialization, but constructor is not
    // ...called until this method is called.
    static Comms theInstance;
    return theInstance;
}


/* 
 * Methods to handle Wi-Fi connections
 */

void Comms::wifi_connect() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print("WIFI: Status: ");
    Serial.println(WiFi.status());
    Serial.println("WIFI: Connecting...");
  }
  Serial.println(F("WIFI: CONNECTED"));
}

void Comms::wifi_disconnect() {
  WiFi.disconnect();
}


/* 
 * Methods to handle MQTT broker connections and subscriptions
 */

boolean Comms::mqtt_connect() {

  if (!m_mqttClient.connected()) {
    Serial.println("MQTT: Setting server");
    m_mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    Serial.println("MQTT: Setting callback");
    m_mqttClient.setCallback([](char* topic, byte* payload, unsigned int length)->void{

      String message;
      for (unsigned int i = 0; i < length; i++) {
        message = message + (char)payload[i]; // convert *byte to string
      }

      Serial.print("MQTT: Recieved: ");
      Serial.print(topic);
      Serial.print(": ");
      Serial.println(message);

      if (strcmp(topic, FIRMWARE_MQTT_TOPIC) == 0) {
        Serial.print(F("MQTT: Processing: "));
        Serial.println(FIRMWARE_MQTT_TOPIC);
        Comms::instance().m_desiredFirmwareVersion = atoi(message.c_str());
        Serial.println(F("MQTT: Desired firmware version set"));
      }
      
    });

    Serial.printf("MQTT: Connecting to broker: %s:%i\n", MQTT_BROKER, MQTT_PORT);
    delay(500);
    static String mac_address = WiFi.macAddress();
    while (!m_mqttClient.connect(mac_address.c_str(), MQTT_USER, MQTT_PASS)) {
      Serial.println("MQTT: Connecting...");
      Serial.print("MQTT: State: ");
      Serial.println(m_mqttClient.state());
    }
    Serial.printf("MQTT: Connected to broker: %s:%i\n", MQTT_BROKER, MQTT_PORT);

    if ( m_mqttClient.subscribe(FIRMWARE_MQTT_TOPIC) ) {
      Serial.printf("MQTT: Subscribe to `%s` successful\n", FIRMWARE_MQTT_TOPIC);
    } else {
      Serial.printf("MQTT: Subscribe to `%s` UNSUCCESSFUL\n", FIRMWARE_MQTT_TOPIC);
    }

    // Unfortunately we need to service the loop so that we will get
    // get any topics we're subscribed to (critical for OTA updates)
    for (int i=0; i < 10; i++) {
      Serial.println("MQTT: loop()");
      m_mqttClient.loop();
      delay(100);
    }
  }
  return m_mqttClient.connected();
}

void Comms::mqtt_disconnect() {
  m_mqttClient.disconnect();
}



void Comms::ota_perform_update() {

    httpUpdate.setLedPin(GPIO_NUM_21, LOW);

    Serial.println("OTA: Attaching callbacks");
    httpUpdate.onStart([]() -> void {
    });
    httpUpdate.onProgress([](int cur, int total) -> void {
      Serial.printf("OTA: HTTP update process at %d of %d bytes...\n", cur, total);
    });
    httpUpdate.onEnd([]() -> void {
      Serial.println("OTA: HTTP update process finishing");
      int new_firmware_version = Comms::instance().m_desiredFirmwareVersion;
      Serial.printf("\nFIRMWARE: New version installed: %d\n", new_firmware_version);
      EEPROM.write(FIRMWARE_VERSION_EEPROM_LOC, new_firmware_version);
      if (EEPROM.commit()) {
        Serial.println("EEPROM: successfully committed");
      } else {
        Serial.println("EEPROM: ERR! commit failed");
      }
      Serial.println("OTA: Disconnecting Wi-Fi");
      WiFi.disconnect();
      Serial.println("OTA: Delaying 5s so Wi-Fi reconnect is more reliable");
      delay(5000);
    });
    httpUpdate.onError([](int err) -> void {
      Serial.printf("OTA: HTTP update fatal error code %d\n", err);
    });

    Serial.println("OTA: Getting current firmware version");
    std::string s = std::to_string(m_firmwareVersion);



    Serial.println("OTA: Starting httpUpdate.update()");
    t_httpUpdate_return ret = httpUpdate.update(
      m_wiFiClient,
      OTA_FIRMWARE_URI
      ,s.c_str()
    );
    httpUpdate.setLedPin(GPIO_NUM_21, HIGH);

    switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
        break;

      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        break;

      case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        break;
    }
}

boolean Comms::ota_update_required() {
  return m_desiredFirmwareVersion != m_firmwareVersion;
}


Comms::Comms(): 
  m_mqttClient(m_wiFiClient)
{
  // FIXME: Handling EEPROM in this class is probably a bad idea... should be a new class to centralize this
  EEPROM.begin(256);
  byte firmware_byte = EEPROM.read(FIRMWARE_VERSION_EEPROM_LOC);
  m_firmwareVersion = (int)firmware_byte;
  m_desiredFirmwareVersion = m_firmwareVersion;
  Serial.printf("FIRMWARE: Version %d\n", m_firmwareVersion);
}