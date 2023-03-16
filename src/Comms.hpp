
//===[ Include only once ]===============================================
#if !defined(Comms_HPP)
#define Comms_HPP

#include "credentials.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <HTTPUpdate.h>
#include <EEPROM.h>


#define FIRMWARE_VERSION_EEPROM_LOC     0
#define FIRMWARE_MQTT_TOPIC             "ota_troubleshooting/version"


//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//--------------------|  Comms class definition   |---------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
/*
 * 
 *  This class provides connectivity to the Comms Gateway, MQTT processing,
 *  and the ability to handle over-the-air (OTA) firmware updating.
 * 
 */
class Comms {
    //===[ Public Methods ]==============================================
    public:
        // Singleton accessor
        static Comms  &instance();
    
        // Functions
        void            wifi_connect();
        void            wifi_disconnect();

        boolean         mqtt_connect();
        void            mqtt_disconnect();
        void            mqtt_callback();

        void            ota_perform_update();  // Presumes running in a context with Wi-Fi already connected
        boolean         ota_update_required();


        PubSubClient    m_mqttClient;

        int             m_firmwareVersion;
        int             m_desiredFirmwareVersion;


    //===[ Protected Methods ]===========================================
    protected:
        // Constructor
        Comms();

        // Internal data
        WiFiClient      m_wiFiClient;


};

#endif
