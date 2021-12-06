#ifndef _OTA_H_
#define _OTA_H_

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "config.h"
#include "utils.h"

class OTA {
private:
    String      _hostname;

public:
    OTA(String hostname) {
        _hostname = hostname;
    }

    void setup(char *ssid, char *password) {
        LOG("OTA !!!!!!\n");
        WiFi.mode(WIFI_STA);
        WiFi.setHostname(_hostname.c_str());
        WiFi.begin(ssid, password);
        while (WiFi.waitForConnectResult() != WL_CONNECTED) {
            LOG("Connection Failed! Rebooting...\n");
            delay(5000);
            ESP.restart();
        }

        // Port defaults to 3232
        // ArduinoOTA.setPort(3232);

        // Hostname defaults to esp3232-[MAC]
        ArduinoOTA.setHostname(_hostname.c_str());

        // No authentication by default
        // ArduinoOTA.setPassword("admin");

        // Password can be set with it's md5 value as well
        // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
        // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

        ArduinoOTA
            .onStart([]() {
                String type;
                if (ArduinoOTA.getCommand() == U_FLASH) {
                    type = "sketch";
                } else { // U_SPIFFS
                    type = "filesystem";
                }
                // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                LOG("Start updating : %s\n", type.c_str());
            })
            .onEnd([]() {
                LOG("\nEnd\n");
            })
            .onProgress([](unsigned int progress, unsigned int total) {
                LOG("Progress: %u%%\r", (progress / (total / 100)));
            })
            .onError([](ota_error_t error) {
                LOG("Error[%u]: ", error);
                if (error == OTA_AUTH_ERROR) LOG("Auth Failed\n");
                else if (error == OTA_BEGIN_ERROR) LOG("Begin Failed\n");
                else if (error == OTA_CONNECT_ERROR) LOG("Connect Failed\n");
                else if (error == OTA_RECEIVE_ERROR) LOG("Receive Failed\n");
                else if (error == OTA_END_ERROR) LOG("End Failed\n");
            });

            ArduinoOTA.begin();
            LOG("Ready\n");
            LOG("IP address: %s\n", WiFi.localIP().toString().c_str());
            LOG("hostname  : %s\n", ArduinoOTA.getHostname().c_str());
    }

    void loop() {
        ArduinoOTA.handle();
    }

};
#endif