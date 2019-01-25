/*
This sketch reads the MFRC522 RFID reader and sends out MQTT messages when cards are scanned. It also subscribes to MQTT messages telling it to open the door lock, which is actuated by a relay switching an electronic door strike.

 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"


MFRC522 pin connections
SDA=>D2
SCK=>D5
MOSI=>D7
MISO=>D6
IRC=>NC
RST=>D1

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// Update these with values suitable for your network.


#define RELAY_PIN 3




const char* ssid = "SC_DORM_401";
const char* password = "zeupheT2Ch";
const char* mqtt_server = "10.7.1.1";
const char* mqtt_username = "smartclassroom";
const char* mqtt_password = "Reemei8doh";
const char* mqtt_id = "lights";
const char* subscribe_lights = "smartclassroom/Lights/on";

WiFiClient espClient;
PubSubClient client(espClient);

char button_value[50];

void setup() {
  // Initialize the BUILTIN_LED pin as an output
  pinMode(RELAY_PIN, OUTPUT);
  

  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname("Lights");
  ArduinoOTA.setPassword((const char *)"xai0aeQu2g");
  
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";
   
  });
  ArduinoOTA.onEnd([]() {
    
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    
  });
  ArduinoOTA.onError([](ota_error_t error) {
   
  });
  ArduinoOTA.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  reconnect();
  digitalWrite(RELAY_PIN, HIGH);
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
 

  WiFi.mode(WIFI_STA);
  WiFi.hostname("Lights");
  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(5000);
    ESP.restart();
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
 
  memset(button_value, 0, sizeof(button_value));
  strncpy(button_value, (char *)payload, length);

  if(strcmp(button_value, "true")==0) {
    digitalWrite(RELAY_PIN, HIGH);
  }
  else if(strcmp(button_value, "false")==0) {
    digitalWrite(RELAY_PIN, LOW);
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    // Attempt to connect
    if (client.connect(mqtt_id, mqtt_username, mqtt_password)) {
      // Once connected, publish an announcement...
      client.subscribe(subscribe_lights);
    } else {
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

bool ota_flag = true;
uint16_t time_elapsed = 0;

void loop() {
  //if(ota_flag)
  //{
    //while(time_elapsed < 30000)
    //{
  ArduinoOTA.handle();
      //time_elapsed = millis();
      //delay(10);
    //}
    //ota_flag = false;
  //}
  delay(500);
  client.loop();
  reconnect();
}



