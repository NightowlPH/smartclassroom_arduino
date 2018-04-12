/*
This sketch uses and ESP8266/nodemcu board to control a SANYO airconditioning through IR remote control codes.

 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"

IR send pin: D1

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

// Update these with values suitable for your network.


#define IR_SEND_PIN 4

uint16_t aircon_data_on[73] = {8996, 4454,  648, 1660,  650, 582,  630, 576,  624, 1658,  654, 580,  622, 1660,  650, 1660,  650, 582,  632, 576,  626, 1656,  654, 580,  622, 1662,  650, 584,  628, 578,  624, 582,  630, 576,  626, 582,  622, 584,  626, 580,  622, 586,  626, 580,  622, 584,  628, 580,  622, 584,  628, 578,  624, 582,  630, 576,  624, 582,  630, 1652,  648, 586,  626, 1656,  654, 580,  622, 584,  628, 1654,  656, 578,  626};  // UNKNOWN CFB5CCB7
uint16_t aircon_data_off[73] = {9006, 4444,  648, 1662,  650, 584,  630, 578,  624, 582,  630, 576,  626, 1656,  654, 1656,  654, 578,  624, 584,  630, 1654,  658, 576,  626, 1656,  654, 580,  622, 586,  628, 578,  624, 584,  628, 578,  626, 582,  628, 578,  624, 582,  620, 586,  626, 580,  622, 586,  626, 580,  622, 584,  628, 578,  624, 582,  630, 578,  624, 1658,  652, 580,  622, 1662,  648, 586,  628, 580,  622, 1660,  652, 582,  630};  // UNKNOWN 21255021

const char* ssid = "<wifi name>";
const char* password = "<wifi_password>";
const char* mqtt_server = "<mqtt server ip>";
const char* mqtt_username = "<mqtt server username>";
const char* mqtt_password = "<mqtt password>";
const char* mqtt_id = "<unique device mqtt id>";

const char* subscribe_aircon_on = "smartclassroom/event/aircon/on";

WiFiClient espClient;
PubSubClient client(espClient);
IRsend irsend(IR_SEND_PIN);

char button_value[50];

void setup() {
  // Initialize the BUILTIN_LED pin as an output
  
  irsend.begin();
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  
  delay(250);
  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname("Nightowl_aircon");
  ArduinoOTA.setPassword((const char *)"123");
  
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  reconnect();
  Serial.println(F("-------------------"));
  Serial.println(F("Everything Ready"));
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  memset(button_value, 0, sizeof(button_value));
  strncpy(button_value, (char *)payload, length);

  if(strcmp(button_value, "true")==0) {
    aircon_on();
  }
  else if(strcmp(button_value, "false")==0) {
    aircon_off();
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(mqtt_id, mqtt_username, mqtt_password)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.subscribe(subscribe_aircon_on);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

bool ota_flag = true;
uint16_t time_elapsed = 0;

void loop() {
  if(ota_flag)
  {
    while(time_elapsed < 15000)
    {
      ArduinoOTA.handle();
      time_elapsed = millis();
      delay(10);
    }
    ota_flag = false;
  }
  delay(500);

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  delay(500);
}


void aircon_on() {
  while (!client.connected()) {
    reconnect();
  }
  Serial.println("Turning aircon on");
  irsend.sendRaw(aircon_data_on, 73, 38);  // Send a raw data capture at 38kHz.

}

void aircon_off() {

  Serial.println("Turning aircon off");
  irsend.sendRaw(aircon_data_off, 73, 38);  // Send a raw data capture at 38kHz.
}



