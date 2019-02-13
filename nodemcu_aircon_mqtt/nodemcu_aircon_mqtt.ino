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

#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <PubSubClient.h>

#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Daikin.h>
#include <Ticker.h>
#include <FS.h>
#include <ArduinoJson.h>


// Update these with values suitable for your network.


#define IR_SEND_PIN 3
#define LED_PIN 1

#define SANYO_AC 0


const char* subscribe_aircon_on = "smartclassroom/Aircon/on";
const char* subscribe_update_topic = "smartclassroom/Aircon/update";
const char* subscribe_aircon_temperature = "smartclassroom/Aircon temperature/on";


uint16_t sanyo_data_on[73] = {9044, 4466,  648, 1670,  652, 586,  626, 588,  626, 1666,  654, 1666,  656, 556,  656, 1664,  658, 580,  632, 580,  632, 554,  660, 552,  650, 562,  650, 564,  648, 590,  624, 562,  650, 562,  650, 562,  650, 562,  652, 562,  650, 562,  652, 560,  652, 560,  652, 560,  652, 560,  652, 586,  626, 560,  652, 560,  652, 560,  652, 1666,  654, 558,  654, 1664,  658, 554,  658, 556,  656, 1662,  660, 552,  660};  // UNKNOWN 1A3AEF63
uint16_t sanyo_data_off[73] = {9030, 4456,  658, 1658,  652, 584,  628, 580,  622, 588,  624, 1664,  656, 580,  622, 1666,  654, 584,  628, 582,  620, 590,  624, 586,  626, 584,  628, 582,  630, 580,  622, 588,  624, 586,  626, 584,  628, 582,  620, 590,  624, 586,  626, 584,  628, 582,  630, 580,  622, 588,  624, 586,  626, 584,  628, 582,  630, 580,  622, 1666,  656, 582,  630, 1658,  652, 584,  628, 582,  630, 1660,  652, 586,  628};  // UNKNOWN ABD88E13
uint16_t sanyo_temp_data[11][73] = {
  {9042, 4474,  650, 1670,  650, 588,  624, 590,  624, 1670,  652, 1668,  652, 586,  626, 1668,  654, 586,  626, 586,  626, 560,  652, 586,  626, 586,  626, 588,  626, 588,  624, 588,  624, 588,  624, 562,  650, 590,  622, 562,  650, 590,  622, 590,  624, 588,  624, 590,  622, 590,  622, 564,  648, 590,  622, 590,  622, 592,  632, 1662,  650, 590,  622, 1672,  650, 588,  624, 590,  622, 1672,  650, 590,  622},  // UNKNOWN 1A3AEF63
  {9176, 4472,  648, 1670,  650, 588,  624, 590,  624, 1670,  652, 1668,  652, 588,  624, 1668,  652, 586,  626, 1668,  652, 586,  626, 586,  626, 586,  626, 588,  626, 588,  624, 588,  624, 590,  624, 588,  624, 588,  624, 588,  624, 588,  624, 590,  622, 590,  624, 590,  624, 590,  622, 590,  622, 590,  632, 580,  632, 580,  630, 1662,  660, 580,  632, 1662,  660, 580,  632, 580,  632, 1662,  660, 580,  632},  // UNKNOWN 9EC6B9C1
  {9044, 4468,  656, 1662,  658, 580,  632, 580,  632, 1662,  650, 1670,  652, 588,  624, 1668,  652, 586,  626, 586,  626, 1666,  654, 586,  626, 586,  626, 586,  626, 586,  628, 586,  626, 586,  626, 586,  626, 586,  626, 586,  626, 586,  626, 586,  626, 586,  626, 586,  626, 586,  626, 586,  626, 586,  626, 586,  626, 588,  624, 1668,  654, 586,  626, 1666,  654, 586,  626, 586,  628, 1666,  656, 584,  628},  // UNKNOWN B565A1C5
  {9052, 4468,  656, 1664,  656, 556,  658, 556,  656, 1664,  656, 1664,  658, 554,  658, 1662,  658, 554,  658, 1662,  658, 1662,  658, 554,  658, 554,  658, 554,  658, 554,  658, 556,  658, 556,  656, 556,  656, 556,  658, 556,  656, 558,  654, 558,  654, 558,  654, 558,  654, 558,  654, 558,  654, 558,  654, 558,  654, 560,  652, 1668,  654, 558,  652, 1668,  654, 560,  652, 560,  652, 1668,  654, 560,  652},  // UNKNOWN 67BD0ACD
  {9048, 4470,  654, 1668,  654, 584,  628, 584,  632, 1662,  656, 1664,  656, 582,  630, 1664,  656, 584,  628, 584,  630, 584,  628, 1666,  656, 584,  628, 584,  628, 584,  628, 586,  628, 586,  626, 586,  628, 586,  626, 586,  626, 586,  626, 586,  626, 588,  624, 588,  624, 588,  624, 588,  624, 588,  624, 590,  624, 590,  622, 1672,  650, 590,  624, 1670,  650, 590,  624, 590,  622, 1672,  650, 590,  622},  // UNKNOWN AAB3DEA1
  {9044, 4476,  658, 1662,  658, 554,  658, 554,  658, 1664,  658, 1662,  658, 554,  658, 1664,  658, 554,  658, 1664,  658, 556,  658, 1664,  658, 554,  658, 556,  658, 556,  656, 556,  656, 558,  654, 558,  656, 558,  654, 558,  654, 558,  654, 558,  654, 558,  654, 558,  652, 560,  652, 560,  652, 560,  652, 560,  652, 562,  650, 1670,  650, 562,  650, 1670,  650, 562,  650, 564,  650, 1672,  650, 562,  650},  // UNKNOWN A16E28DB
  {9046, 4472,  650, 1670,  652, 588,  624, 588,  626, 1670,  650, 1670,  652, 588,  624, 1670,  652, 588,  624, 588,  624, 1670,  650, 1670,  652, 588,  624, 588,  624, 588,  624, 590,  622, 590,  632, 580,  632, 582,  632, 580,  632, 582,  630, 582,  630, 584,  630, 584,  628, 584,  628, 586,  626, 586,  628, 586,  626, 586,  626, 1668,  652, 588,  626, 1670,  652, 560,  652, 588,  624, 1670,  650, 562,  650},  // UNKNOWN FEF5D0F1
  {9046, 4470,  654, 1666,  654, 584,  630, 582,  628, 1664,  658, 1664,  658, 582,  630, 1664,  658, 582,  630, 1664,  658, 1664,  658, 1662,  658, 580,  632, 582,  632, 580,  632, 582,  630, 582,  630, 582,  630, 582,  630, 584,  628, 584,  628, 584,  628, 584,  628, 586,  628, 584,  628, 586,  626, 586,  626, 586,  626, 588,  626, 1668,  652, 586,  626, 1670,  652, 586,  626, 588,  624, 1670,  652, 588,  626},  // UNKNOWN B14D39F9
  {9044, 4472,  654, 1666,  654, 584,  628, 586,  626, 1666,  654, 1666,  656, 556,  656, 1666,  656, 556,  656, 556,  656, 558,  654, 558,  656, 1666,  656, 556,  656, 558,  654, 558,  654, 586,  628, 586,  626, 560,  654, 558,  654, 560,  652, 560,  652, 562,  652, 560,  652, 562,  652, 562,  650, 562,  650, 562,  650, 562,  650, 1672,  650, 564,  648, 1672,  650, 564,  648, 564,  658, 1662,  650, 564,  660},  // UNKNOWN 2A368E65
  {9052, 4468,  654, 1664,  656, 584,  630, 582,  630, 1664,  656, 1664,  656, 556,  656, 1664,  658, 582,  630, 1664,  658, 582,  632, 580,  630, 1664,  658, 582,  630, 582,  630, 582,  630, 584,  630, 582,  630, 584,  628, 584,  628, 584,  630, 584,  628, 584,  628, 586,  628, 584,  628, 586,  626, 586,  626, 586,  626, 586,  626, 1670,  652, 588,  624, 1670,  652, 588,  624, 588,  624, 1670,  652, 588,  624},  // UNKNOWN 655F4B57
  {9046, 4472,  652, 1668,  652, 558,  654, 560,  652, 1668,  654, 1666,  654, 558,  654, 1666,  654, 558,  656, 556,  656, 1666,  656, 556,  656, 1666,  656, 556,  656, 558,  656, 556,  656, 558,  654, 558,  654, 558,  654, 558,  654, 558,  654, 558,  654, 558,  654, 558,  654, 558,  654, 558,  654, 560,  652, 560,  652, 560,  652, 1668,  652, 560,  652, 1668,  654, 560,  652, 560,  652, 1668,  652, 560,  652}  // UNKNOWN 7BFE335B
};


uint32_t numblink;
uint32_t maxblink;
char c;

struct Settings{
  char ssid[64];
  char password[64];
  char mqtt_server[15];
  char mqtt_username[32];
  char mqtt_password[32];
  char mqtt_id[32];
};

struct Settings settings;

Ticker blinker;

WiFiClient espClient;
PubSubClient client(espClient);
#if SANYO_AC
IRsend irsend(IR_SEND_PIN);
#else
IRDaikinESP ac(IR_SEND_PIN);
#endif

char button_value[50];

void setup() {
  setup_ir();
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  SPIFFS.begin();
  read_settings();
  
  delay(250);
  setup_wifi();
  
  client.setServer(settings.mqtt_server, 1883);
  client.setCallback(callback);
  reconnect();
  Serial.println(F("-------------------"));
  Serial.println(F("Everything Ready"));
  pinMode(LED_PIN, OUTPUT);
}

void setup_ir() {
  #if SANYO_AC
    pinMode(IR_SEND_PIN, FUNCTION_3);
    irsend.begin();
  #else
    ac.begin();
  #endif
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(settings.ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(settings.ssid, settings.password);

  while (WiFi.status() != WL_CONNECTED) {
    blink(5, 100);
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void read_settings() {
  File f = SPIFFS.open("/settings.json", "r");
  Serial.println("");
  Serial.println("Opening /settings.json");
  if(!f){
    Serial.println("Failed to read file");
    return;
  }
  const size_t capacity = JSON_OBJECT_SIZE(6) + 160;
  DynamicJsonBuffer jsonBuffer(capacity);
  JsonObject &root = jsonBuffer.parseObject(f);
  if(!root.success()){
    Serial.println("Failed to read file");
    return;
  }
  f.close();
  Serial.println("Finished parsing settings file");
  strlcpy(settings.ssid, root["ssid"], sizeof(settings.ssid));
  strlcpy(settings.password, root["password"], sizeof(settings.password));
  strlcpy(settings.mqtt_server, root["mqtt_server"], sizeof(settings.mqtt_server));
  strlcpy(settings.mqtt_username, root["mqtt_username"], sizeof(settings.mqtt_username));
  strlcpy(settings.mqtt_password, root["mqtt_password"], sizeof(settings.mqtt_password));
  strlcpy(settings.mqtt_id, root["mqtt_id"], sizeof(settings.mqtt_id));
}

void blink(uint32_t num_blink, uint16_t delay_time){
  //blink num_blink times with delay_time interval
  maxblink = num_blink;
  numblink = 0;
  blinker.attach_ms(delay_time, switch_led);
}

void switch_led() {
  //turn the LED off if its on, or on if its off, as long as we should be blinking
  uint8_t led_state = digitalRead(LED_PIN);
  digitalWrite(LED_PIN, !led_state);
  if (led_state == LOW) {
    //LED was on, now turned off again, advance num_blink;
    numblink++;
  }
  if (numblink >= maxblink) {
    //stop blinking because we blinked the maximum number
    blinker.detach();
  }
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  blink(4, 200);
  
  if(strcmp(topic, subscribe_update_topic) == 0){
    checkUpdate((char *)payload);
    return;
  }

  memset(button_value, 0, sizeof(button_value));
  strncpy(button_value, (char *)payload, length);

  if(strcmp(button_value, "true")==0) {
    aircon_on();
  }
  else if(strcmp(button_value, "false")==0) {
    aircon_off();
    return;
  }
  if((int) (&button_value - '0') >= 16 & (int) (&button_value - '0') <= 26){
    aircon_temp((int) (&button_value - '0'));
    return;
  }
  
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(settings.mqtt_id, settings.mqtt_username, settings.mqtt_password)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.subscribe(subscribe_aircon_on);
      client.subscribe(subscribe_aircon_temperature);
      client.subscribe(subscribe_update_topic);
      blink(3, 200);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      blink(50000, 100); //Blink fast in the mean time with 100ms intervals
      delay(5000);
    }
  }
  
}

void checkUpdate(char* update_uri) {
  Serial.print("Checking for update on url ");
  Serial.println(update_uri);
  ESPhttpUpdate.setLedPin(LED_PIN, LOW);
  t_httpUpdate_return ret = ESPhttpUpdate.update(update_uri);
  switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        break;

      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        break;

      case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        break;
    }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  delay(500); 
}


void aircon_on() {
  Serial.println("Turning aircon on");
  #if SANYO_AC
  irsend.sendRaw(sanyo_data_on, 73, 38);  // Send a raw data capture at 38kHz.
  #else
  ac.on();
  ac.setMode(3);
  ac.setFan(10);
  ac.setSwingVertical(true);
  ac.setSwingHorizontal(true);
  ac.send();
  #endif
}

void aircon_off() {

  Serial.println("Turning aircon off");
  #if SANYO_AC
    irsend.sendRaw(sanyo_data_off, 73, 38);  // Send a raw data capture at 38kHz.
  #else
    ac.off();
    ac.send();
  #endif
}

void aircon_temp(int temp) {
  #if SANYO_AC
    irsend.sendRaw(sanyo_temp_data[temp-16], 73, 38);  // Send a raw data capture at 38kHz.
  #else
    ac.setTemp(temp);
    ac.send();
  #endif
}
