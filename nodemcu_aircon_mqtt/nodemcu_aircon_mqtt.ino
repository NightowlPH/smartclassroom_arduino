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


#define IR_SEND_PIN 3

uint16_t aircon_data_on[73] = {8996, 4454,  648, 1660,  650, 582,  630, 576,  624, 1658,  654, 580,  622, 1660,  650, 1660,  650, 582,  632, 576,  626, 1656,  654, 580,  622, 1662,  650, 584,  628, 578,  624, 582,  630, 576,  626, 582,  622, 584,  626, 580,  622, 586,  626, 580,  622, 584,  628, 580,  622, 584,  628, 578,  624, 582,  630, 576,  624, 582,  630, 1652,  648, 586,  626, 1656,  654, 580,  622, 584,  628, 1654,  656, 578,  626};  // UNKNOWN CFB5CCB7
uint16_t aircon_data_off[73] = {9006, 4444,  648, 1662,  650, 584,  630, 578,  624, 582,  630, 576,  626, 1656,  654, 1656,  654, 578,  624, 584,  630, 1654,  658, 576,  626, 1656,  654, 580,  622, 586,  628, 578,  624, 584,  628, 578,  626, 582,  628, 578,  624, 582,  620, 586,  626, 580,  622, 586,  626, 580,  622, 584,  628, 578,  624, 582,  630, 578,  624, 1658,  652, 580,  622, 1662,  648, 586,  628, 580,  622, 1660,  652, 582,  630};  // UNKNOWN 21255021
uint16_t rawData_16[73] = {9042, 4470,  654, 1666,  654, 584,  628, 584,  628, 1666,  656, 1664,  658, 582,  630, 1662,  658, 582,  630, 582,  632, 582,  630, 582,  630, 582,  630, 582,  630, 582,  630, 582,  630, 582,  630, 582,  630, 584,  628, 584,  630, 584,  628, 584,  628, 584,  628, 584,  628, 584,  628, 586,  626, 586,  626, 586,  628, 586,  626, 1666,  654, 584,  628, 1666,  656, 584,  628, 584,  628, 1666,  656, 584,  628};  // UNKNOWN 1A3AEF63
uint16_t rawData_17[73] = {9038, 4474,  650, 1670,  650, 588,  626, 588,  624, 1668,  652, 1666,  654, 586,  626, 1666,  656, 584,  628, 1664,  656, 582,  630, 582,  630, 584,  630, 582,  630, 582,  630, 584,  628, 584,  630, 582,  630, 584,  628, 584,  628, 584,  628, 584,  628, 584,  628, 584,  628, 584,  628, 584,  628, 584,  628, 584,  628, 584,  628, 1666,  654, 586,  628, 1664,  656, 584,  630, 584,  628, 1664,  658, 582,  632};  // UNKNOWN 9EC6B9C1
uint16_t rawData_18[73] = {9042, 4472,  652, 1668,  654, 586,  628, 586,  626, 1692,  628, 1666,  654, 584,  630, 1664,  656, 584,  630, 584,  628, 1690,  630, 582,  630, 582,  630, 582,  630, 582,  630, 582,  630, 584,  628, 584,  628, 584,  628, 584,  628, 584,  628, 584,  628, 584,  628, 584,  628, 586,  626, 586,  626, 586,  626, 586,  626, 586,  626, 1694,  628, 586,  626, 1668,  654, 586,  626, 586,  626, 1694,  626, 586,  628};  // UNKNOWN B565A1C5
uint16_t rawData_19[73] = {9070, 4470,  654, 1666,  654, 584,  628, 584,  626, 1666,  654, 1666,  654, 584,  628, 1666,  656, 584,  628, 1666,  654, 1666,  656, 584,  628, 584,  628, 586,  626, 586,  626, 586,  626, 586,  626, 588,  624, 588,  624, 588,  624, 588,  624, 588,  624, 590,  622, 590,  624, 590,  632, 580,  632, 580,  632, 582,  630, 582,  630, 1664,  656, 582,  630, 1664,  656, 582,  630, 584,  628, 1664,  656, 584,  628};  // UNKNOWN 67BD0ACD
uint16_t rawData_20[73] = {9048, 4466,  656, 1662,  658, 582,  630, 582,  630, 1664,  658, 1662,  658, 580,  632, 1662,  658, 580,  632, 582,  630, 582,  630, 1664,  658, 582,  630, 582,  630, 584,  628, 584,  628, 584,  628, 584,  628, 586,  626, 586,  626, 588,  626, 588,  624, 588,  624, 588,  624, 590,  624, 590,  622, 590,  632, 582,  632, 582,  630, 1664,  656, 582,  630, 1664,  658, 582,  630, 584,  628, 1664,  656, 584,  628};  // UNKNOWN AAB3DEA1
uint16_t rawData_21[73] = {9042, 4472,  652, 1668,  652, 586,  626, 588,  624, 1670,  652, 1668,  652, 588,  624, 1668,  652, 588,  624, 1668,  652, 588,  624, 1670,  650, 588,  624, 588,  624, 590,  622, 590,  622, 590,  622, 592,  630, 582,  632, 582,  630, 582,  630, 584,  630, 582,  630, 584,  628, 584,  628, 586,  626, 586,  626, 588,  626, 588,  624, 1670,  652, 588,  624, 1670,  652, 588,  624, 590,  622, 1672,  650, 590,  622};  // UNKNOWN A16E28DB
uint16_t rawData_22[73] = {9044, 4474,  660, 1662,  648, 590,  632, 580,  632, 1674,  646, 1664,  658, 582,  630, 1664,  656, 584,  628, 584,  630, 1666,  654, 1666,  656, 584,  628, 586,  626, 586,  626, 588,  624, 590,  624, 588,  624, 590,  622, 592,  632, 580,  632, 582,  630, 584,  628, 584,  628, 586,  626, 586,  626, 586,  626, 588,  624, 588,  624, 1672,  650, 590,  634, 1660,  660, 582,  630, 582,  630, 1664,  656, 584,  628};  // UNKNOWN FEF5D0F1
uint16_t rawData_23[73] = {9030, 4466,  648, 1668,  654, 584,  628, 582,  630, 1660,  650, 1666,  654, 582,  630, 1660,  650, 586,  626, 1664,  656, 1660,  650, 1666,  656, 582,  630, 582,  632, 580,  622, 588,  624, 588,  624, 586,  626, 584,  628, 582,  630, 582,  630, 580,  622, 588,  624, 586,  626, 586,  626, 586,  628, 584,  628, 582,  630, 580,  622, 1670,  652, 586,  626, 1664,  658, 580,  622, 590,  624, 1668,  654, 584,  630};  // UNKNOWN B14D39F9
uint16_t rawData_24[73] = {9042, 4476,  658, 1662,  658, 580,  632, 582,  630, 1664,  658, 1664,  658, 582,  630, 1664,  656, 584,  628, 584,  628, 586,  626, 586,  626, 1668,  654, 586,  626, 588,  624, 588,  624, 590,  622, 590,  622, 590,  622, 590,  632, 582,  630, 584,  630, 582,  630, 584,  628, 586,  626, 586,  628, 586,  626, 586,  626, 588,  626, 1670,  650, 588,  624, 1670,  650, 590,  624, 590,  632, 1662,  658, 582,  630};  // UNKNOWN 2A368E65
uint16_t rawData_25[73] = {9052, 4466,  658, 1662,  658, 580,  632, 582,  630, 1664,  658, 1662,  658, 582,  630, 1664,  658, 582,  632, 1662,  658, 582,  632, 582,  630, 1664,  658, 582,  630, 584,  630, 584,  628, 584,  628, 584,  628, 586,  628, 586,  626, 586,  626, 588,  624, 588,  624, 588,  624, 590,  622, 590,  632, 580,  632, 582,  630, 582,  632, 1664,  658, 556,  656, 1664,  658, 556,  656, 558,  654, 1666,  656, 558,  654};  // UNKNOWN 655F4B57
uint16_t rawData_26[73] = {9040, 4476,  658, 1662,  658, 580,  632, 580,  632, 1662,  658, 1662,  648, 592,  632, 1662,  648, 592,  632, 582,  632, 1662,  660, 580,  632, 1662,  658, 582,  630, 582,  630, 582,  630, 582,  630, 584,  628, 584,  630, 584,  628, 586,  626, 586,  626, 586,  626, 586,  626, 586,  624, 588,  624, 588,  624, 588,  624, 590,  622, 1672,  650, 590,  622, 1672,  650, 590,  632, 582,  630, 1664,  658, 580,  630};  // UNKNOWN 7BFE335B

const char* ssid = "SC_NIGHTOWL_LAB";
const char* password = "a78ae1be68";
const char* mqtt_server = "10.7.1.1";
const char* mqtt_username = "smart_classroom";
const char* mqtt_password = "5dc2deb5fe";
const char* mqtt_id = "NightOwl-Aircon-RV2";

const char* subscribe_aircon_on = "smartclassroom/NightOwl/aircon2/on";

WiFiClient espClient;
PubSubClient client(espClient);
IRsend irsend(IR_SEND_PIN);

char button_value[50];

void setup() {
  // Initialize the BUILTIN_LED pin as an output
  pinMode(IR_SEND_PIN, FUNCTION_3); 
  irsend.begin();
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  
  delay(250);
  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname("Nightowl_aircon-RV2");
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
  else if(strcmp(button_value, "16")==0) {
    aircon_temp_16();
  }
  else if(strcmp(button_value, "17")==0) {
    aircon_temp_17();
  }
  else if(strcmp(button_value, "18")==0) {
    aircon_temp_18();
  }
  else if(strcmp(button_value, "19")==0) {
    aircon_temp_19();
  }
  else if(strcmp(button_value, "20")==0) {
    aircon_temp_20();
  }
  else if(strcmp(button_value, "21")==0) {
    aircon_temp_21();
  }
  else if(strcmp(button_value, "22")==0) {
    aircon_temp_22();
  }
  else if(strcmp(button_value, "23")==0) {
    aircon_temp_23();
  }
  else if(strcmp(button_value, "24")==0) {
    aircon_temp_24();
  }
  else if(strcmp(button_value, "25")==0) {
    aircon_temp_25();
  }
  else if(strcmp(button_value, "26")==0) {
    aircon_temp_26();
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

void aircon_temp_16() {
  irsend.sendRaw(rawData_16, 73, 38);  // Send a raw data capture at 38kHz.
}

void aircon_temp_17() {
  irsend.sendRaw(rawData_17, 73, 38);  // Send a raw data capture at 38kHz.
}

void aircon_temp_18() {
  irsend.sendRaw(rawData_18, 73, 38);  // Send a raw data capture at 38kHz.
}

void aircon_temp_19() {
  irsend.sendRaw(rawData_19, 73, 38);  // Send a raw data capture at 38kHz.
}

void aircon_temp_20() {
  irsend.sendRaw(rawData_20, 73, 38);  // Send a raw data capture at 38kHz.
}

void aircon_temp_21() {
  irsend.sendRaw(rawData_21, 73, 38);  // Send a raw data capture at 38kHz.
}

void aircon_temp_22() {
  irsend.sendRaw(rawData_22, 73, 38);  // Send a raw data capture at 38kHz.
}

void aircon_temp_23() {
  irsend.sendRaw(rawData_23, 73, 38);  // Send a raw data capture at 38kHz.
}

void aircon_temp_24() {
  irsend.sendRaw(rawData_24, 73, 38);  // Send a raw data capture at 38kHz.
}

void aircon_temp_25() {
  irsend.sendRaw(rawData_25, 73, 38);  // Send a raw data capture at 38kHz.
}

void aircon_temp_26() {
  irsend.sendRaw(rawData_26, 73, 38);  // Send a raw data capture at 38kHz.
}
