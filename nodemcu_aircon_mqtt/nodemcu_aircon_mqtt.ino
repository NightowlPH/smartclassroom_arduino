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
#include <ir_Daikin.h>

// Update these with values suitable for your network.


#define IR_SEND_PIN 3
#define LED_PIN 1

uint16_t aircon_data_on[73] = {9044, 4466,  648, 1670,  652, 586,  626, 588,  626, 1666,  654, 1666,  656, 556,  656, 1664,  658, 580,  632, 580,  632, 554,  660, 552,  650, 562,  650, 564,  648, 590,  624, 562,  650, 562,  650, 562,  650, 562,  652, 562,  650, 562,  652, 560,  652, 560,  652, 560,  652, 560,  652, 586,  626, 560,  652, 560,  652, 560,  652, 1666,  654, 558,  654, 1664,  658, 554,  658, 556,  656, 1662,  660, 552,  660};  // UNKNOWN 1A3AEF63
uint16_t aircon_data_off[73] = {9030, 4456,  658, 1658,  652, 584,  628, 580,  622, 588,  624, 1664,  656, 580,  622, 1666,  654, 584,  628, 582,  620, 590,  624, 586,  626, 584,  628, 582,  630, 580,  622, 588,  624, 586,  626, 584,  628, 582,  620, 590,  624, 586,  626, 584,  628, 582,  630, 580,  622, 588,  624, 586,  626, 584,  628, 582,  630, 580,  622, 1666,  656, 582,  630, 1658,  652, 584,  628, 582,  630, 1660,  652, 586,  628};  // UNKNOWN ABD88E13
uint16_t rawData_16[73] = {9042, 4474,  650, 1670,  650, 588,  624, 590,  624, 1670,  652, 1668,  652, 586,  626, 1668,  654, 586,  626, 586,  626, 560,  652, 586,  626, 586,  626, 588,  626, 588,  624, 588,  624, 588,  624, 562,  650, 590,  622, 562,  650, 590,  622, 590,  624, 588,  624, 590,  622, 590,  622, 564,  648, 590,  622, 590,  622, 592,  632, 1662,  650, 590,  622, 1672,  650, 588,  624, 590,  622, 1672,  650, 590,  622};  // UNKNOWN 1A3AEF63
uint16_t rawData_17[73] = {9176, 4472,  648, 1670,  650, 588,  624, 590,  624, 1670,  652, 1668,  652, 588,  624, 1668,  652, 586,  626, 1668,  652, 586,  626, 586,  626, 586,  626, 588,  626, 588,  624, 588,  624, 590,  624, 588,  624, 588,  624, 588,  624, 588,  624, 590,  622, 590,  624, 590,  624, 590,  622, 590,  622, 590,  632, 580,  632, 580,  630, 1662,  660, 580,  632, 1662,  660, 580,  632, 580,  632, 1662,  660, 580,  632};  // UNKNOWN 9EC6B9C1
uint16_t rawData_18[73] = {9044, 4468,  656, 1662,  658, 580,  632, 580,  632, 1662,  650, 1670,  652, 588,  624, 1668,  652, 586,  626, 586,  626, 1666,  654, 586,  626, 586,  626, 586,  626, 586,  628, 586,  626, 586,  626, 586,  626, 586,  626, 586,  626, 586,  626, 586,  626, 586,  626, 586,  626, 586,  626, 586,  626, 586,  626, 586,  626, 588,  624, 1668,  654, 586,  626, 1666,  654, 586,  626, 586,  628, 1666,  656, 584,  628};  // UNKNOWN B565A1C5
uint16_t rawData_19[73] = {9052, 4468,  656, 1664,  656, 556,  658, 556,  656, 1664,  656, 1664,  658, 554,  658, 1662,  658, 554,  658, 1662,  658, 1662,  658, 554,  658, 554,  658, 554,  658, 554,  658, 556,  658, 556,  656, 556,  656, 556,  658, 556,  656, 558,  654, 558,  654, 558,  654, 558,  654, 558,  654, 558,  654, 558,  654, 558,  654, 560,  652, 1668,  654, 558,  652, 1668,  654, 560,  652, 560,  652, 1668,  654, 560,  652};  // UNKNOWN 67BD0ACD
uint16_t rawData_20[73] = {9048, 4470,  654, 1668,  654, 584,  628, 584,  632, 1662,  656, 1664,  656, 582,  630, 1664,  656, 584,  628, 584,  630, 584,  628, 1666,  656, 584,  628, 584,  628, 584,  628, 586,  628, 586,  626, 586,  628, 586,  626, 586,  626, 586,  626, 586,  626, 588,  624, 588,  624, 588,  624, 588,  624, 588,  624, 590,  624, 590,  622, 1672,  650, 590,  624, 1670,  650, 590,  624, 590,  622, 1672,  650, 590,  622};  // UNKNOWN AAB3DEA1
uint16_t rawData_21[73] = {9044, 4476,  658, 1662,  658, 554,  658, 554,  658, 1664,  658, 1662,  658, 554,  658, 1664,  658, 554,  658, 1664,  658, 556,  658, 1664,  658, 554,  658, 556,  658, 556,  656, 556,  656, 558,  654, 558,  656, 558,  654, 558,  654, 558,  654, 558,  654, 558,  654, 558,  652, 560,  652, 560,  652, 560,  652, 560,  652, 562,  650, 1670,  650, 562,  650, 1670,  650, 562,  650, 564,  650, 1672,  650, 562,  650};  // UNKNOWN A16E28DB
uint16_t rawData_22[73] = {9046, 4472,  650, 1670,  652, 588,  624, 588,  626, 1670,  650, 1670,  652, 588,  624, 1670,  652, 588,  624, 588,  624, 1670,  650, 1670,  652, 588,  624, 588,  624, 588,  624, 590,  622, 590,  632, 580,  632, 582,  632, 580,  632, 582,  630, 582,  630, 584,  630, 584,  628, 584,  628, 586,  626, 586,  628, 586,  626, 586,  626, 1668,  652, 588,  626, 1670,  652, 560,  652, 588,  624, 1670,  650, 562,  650};  // UNKNOWN FEF5D0F1
uint16_t rawData_23[73] = {9046, 4470,  654, 1666,  654, 584,  630, 582,  628, 1664,  658, 1664,  658, 582,  630, 1664,  658, 582,  630, 1664,  658, 1664,  658, 1662,  658, 580,  632, 582,  632, 580,  632, 582,  630, 582,  630, 582,  630, 582,  630, 584,  628, 584,  628, 584,  628, 584,  628, 586,  628, 584,  628, 586,  626, 586,  626, 586,  626, 588,  626, 1668,  652, 586,  626, 1670,  652, 586,  626, 588,  624, 1670,  652, 588,  626};  // UNKNOWN B14D39F9
uint16_t rawData_24[73] = {9044, 4472,  654, 1666,  654, 584,  628, 586,  626, 1666,  654, 1666,  656, 556,  656, 1666,  656, 556,  656, 556,  656, 558,  654, 558,  656, 1666,  656, 556,  656, 558,  654, 558,  654, 586,  628, 586,  626, 560,  654, 558,  654, 560,  652, 560,  652, 562,  652, 560,  652, 562,  652, 562,  650, 562,  650, 562,  650, 562,  650, 1672,  650, 564,  648, 1672,  650, 564,  648, 564,  658, 1662,  650, 564,  660};  // UNKNOWN 2A368E65
uint16_t rawData_25[73] = {9052, 4468,  654, 1664,  656, 584,  630, 582,  630, 1664,  656, 1664,  656, 556,  656, 1664,  658, 582,  630, 1664,  658, 582,  632, 580,  630, 1664,  658, 582,  630, 582,  630, 582,  630, 584,  630, 582,  630, 584,  628, 584,  628, 584,  630, 584,  628, 584,  628, 586,  628, 584,  628, 586,  626, 586,  626, 586,  626, 586,  626, 1670,  652, 588,  624, 1670,  652, 588,  624, 588,  624, 1670,  652, 588,  624};  // UNKNOWN 655F4B57
uint16_t rawData_26[73] = {9046, 4472,  652, 1668,  652, 558,  654, 560,  652, 1668,  654, 1666,  654, 558,  654, 1666,  654, 558,  656, 556,  656, 1666,  656, 556,  656, 1666,  656, 556,  656, 558,  656, 556,  656, 558,  654, 558,  654, 558,  654, 558,  654, 558,  654, 558,  654, 558,  654, 558,  654, 558,  654, 558,  654, 560,  652, 560,  652, 560,  652, 1668,  652, 560,  652, 1668,  654, 560,  652, 560,  652, 1668,  652, 560,  652};  // UNKNOWN 7BFE335B

const char* ssid = "";
const char* password = "";
const char* mqtt_server = "";
const char* mqtt_username = "";
const char* mqtt_password = "";
const char* mqtt_id = "";

const char* subscribe_aircon_on = "smartclassroom/Aircon/on";
const char* subscribe_aircon_temperature = "smartclassroom/Aircon temperature/on";

WiFiClient espClient;
PubSubClient client(espClient);
//IRsend irsend(IR_SEND_PIN);
IRDaikinESP ac(IR_SEND_PIN);

char button_value[50];

void setup() {
  setup_ir();
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
  pinMode(LED_PIN, OUTPUT);
  blink();
}

void setup_ir() {
  // Initialize the BUILTIN_LED pin as an output
  //pinMode(IR_SEND_PIN, FUNCTION_3); 
  //irsend.begin();
  ac.begin();
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

void blink(){
  Serial.println();
  Serial.println("Blink");
  digitalWrite(LED_PIN, LOW);
  delay(100);
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
  delay(100);
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
  delay(100);
  digitalWrite(LED_PIN, HIGH);
  delay(100);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  blink();
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
      client.subscribe(subscribe_aircon_temperature);
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
  //irsend.sendRaw(aircon_data_on, 73, 38);  // Send a raw data capture at 38kHz.
  ac.on();
  ac.setFan(10);
  ac.setTemp(16);
  ac.setSwingVertical(true);
  ac.setSwingHorizontal(true);
  ac.send();
}

void aircon_off() {

  Serial.println("Turning aircon off");
  //irsend.sendRaw(aircon_data_off, 73, 38);  // Send a raw data capture at 38kHz.
  ac.off();
  ac.send();
}

void aircon_temp_16() {
  //irsend.sendRaw(rawData_16, 73, 38);  // Send a raw data capture at 38kHz.
  ac.setTemp(16);
  ac.send();
}

void aircon_temp_17() {
  //irsend.sendRaw(rawData_17, 73, 38);  // Send a raw data capture at 38kHz.
  ac.setTemp(17);
  ac.send();
}

void aircon_temp_18() {
  //irsend.sendRaw(rawData_18, 73, 38);  // Send a raw data capture at 38kHz.
  ac.setTemp(18);
  ac.send();
}

void aircon_temp_19() {
  //irsend.sendRaw(rawData_19, 73, 38);  // Send a raw data capture at 38kHz.
  ac.setTemp(19);
  ac.send();
}

void aircon_temp_20() {
  //irsend.sendRaw(rawData_20, 73, 38);  // Send a raw data capture at 38kHz.
  ac.setTemp(20);
  ac.send();
}

void aircon_temp_21() {
  ///irsend.sendRaw(rawData_21, 73, 38);  // Send a raw data capture at 38kHz.
  ac.setTemp(21);
  ac.send();
}

void aircon_temp_22() {
  //irsend.sendRaw(rawData_22, 73, 38);  // Send a raw data capture at 38kHz.
  ac.setTemp(22);
  ac.send();
}

void aircon_temp_23() {
  //irsend.sendRaw(rawData_23, 73, 38);  // Send a raw data capture at 38kHz.
  ac.setTemp(23);
  ac.send();
}

void aircon_temp_24() {
  //irsend.sendRaw(rawData_24, 73, 38);  // Send a raw data capture at 38kHz.
  ac.setTemp(24);
  ac.send();
}

void aircon_temp_25() {
  //irsend.sendRaw(rawData_25, 73, 38);  // Send a raw data capture at 38kHz.
  ac.setTemp(25);
  ac.send();
}

void aircon_temp_26() {
  //irsend.sendRaw(rawData_26, 73, 38);  // Send a raw data capture at 38kHz.
  ac.setTemp(26);
  ac.send();
}
