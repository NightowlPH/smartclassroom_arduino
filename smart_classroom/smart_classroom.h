#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include <FS.h>
#include <ArduinoJson.h>

#ifndef smart_classroom_h
#define smart_classroom_h

#define MAXTOPICS 20


struct Settings{
  public:
    char ssid[64];
    char password[64];
    char mqtt_server[15];
    char mqtt_username[32];
    char mqtt_password[32];
    char mqtt_id[32];
};

class SmartClassroom {
    public:
        void blink(uint32_t num_blink, uint16_t delay_time);
        WiFiClient espClient;
        PubSubClient mqtt;
        void begin(uint8_t led_pin, uint8_t led2_pin);
        void reconnect(const char* topics[MAXTOPICS], uint8_t numtopics, const char* version);
        void checkUpdate(char* update_uri);
        uint32_t numblink;
        uint32_t maxblink;
        Ticker blinker;
        uint8_t m_led_pin;
        uint8_t m_led2_pin;
    private:
        struct Settings settings;
        char c;
        void set_settings(JsonObject& root);
        JsonObject& read_settings();
        void setup_wifi();
};

void switch_led(SmartClassroom *sc);
#endif
