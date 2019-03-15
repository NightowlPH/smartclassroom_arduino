#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include <FS.h>
#include <ArduinoJson.h>

// Update these with values suitable for your network.


#define RELAY_PIN 3
#define LED_PIN 1

const char* subscribe_lights_topic = "smartclassroom/Lights/on";
const char* subscribe_update_topic = "smartclassroom/Ligths/update";

struct Settings{
  char ssid[64];
  char password[64];
  char mqtt_server[15];
  char mqtt_username[32];
  char mqtt_password[32];
  char mqtt_id[32];
};

struct State{
  bool lights;
};

Settings settings;
State state;
WiFiClient espClient;
PubSubClient client(espClient);
Ticker blinker;

char button_value[50];
uint32_t numblink;
uint32_t maxblink;
char c;


void setup() {
  // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);

  SPIFFS.begin();
  read_settings();
  read_current_state();
  pinMode(RELAY_PIN, OUTPUT);
  if(state.lights){
    digitalWrite(RELAY_PIN, HIGH);  
  }
  else{
    digitalWrite(RELAY_PIN, LOW);
  }
  
  setup_wifi();
  client.setServer(settings.mqtt_server, 1883);
  client.setCallback(callback);
  reconnect();
  Serial.println(F("Everything Ready"));
  pinMode(LED_PIN, OUTPUT);
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network


  WiFi.mode(WIFI_STA);
  WiFi.hostname("Lights");
  WiFi.begin(settings.ssid, settings.password);
  Serial.print("Connecting to wifi ");
  Serial.println(settings.ssid);

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

void read_current_state() {
  File f = SPIFFS.open("/state.json", "r");
  if(!f){
    Serial.println("Failed to read state file");
  }
  Serial.println("Opened /state.json");
  const size_t capacity = JSON_OBJECT_SIZE(2) + 160;
  DynamicJsonBuffer jsonBuffer(capacity);
  JsonObject &root = jsonBuffer.parseObject(f);
  if(!root.success())
    Serial.println(F("Failed to parse state file"));
  f.seek(0, SeekSet);
  Serial.print("State file contents: ");
  Serial.println(f.read());
  f.close();
  state.lights = root["lights"] == "1" | 0;
  Serial.printf("Light state set to %d\n", state.lights);
}

void write_current_state() {
  Serial.println(F("Writing state file"));
  const size_t capacity = JSON_OBJECT_SIZE(2) + 160;
  DynamicJsonBuffer jsonBuffer(capacity);
  JsonObject &root = jsonBuffer.createObject();
   
  root["lights"] = (int) state.lights;
  String buf;
  File f = SPIFFS.open("/state.json", "w");
  if(!f){
    Serial.println(F("Failed to open state file for writing."));
  }
  root.printTo(buf);
  //Serial.println(buf);
  if(f.println("Blabla")){
    Serial.println(F("State was written"));
  } else {
    Serial.println(F("An error occured while writing"));
  }
  
  f.close();
}

void blink(uint32_t num_blink, uint16_t delay_time) {
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

void callback(char* topic, byte* payload, unsigned int length) {
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println("");
  blink(4, 200);

  if (strcmp(topic, subscribe_update_topic) == 0) {
    checkUpdate((char *)payload);
    return;
  }

  memset(button_value, 0, sizeof(button_value));
  strncpy(button_value, (char *)payload, length);

  if (strcmp(button_value, "true") == 0) {
    digitalWrite(RELAY_PIN, HIGH);
    state.lights = 1;
  }
  else if (strcmp(button_value, "false") == 0) {
    digitalWrite(RELAY_PIN, LOW);
    state.lights = 0;
  }
  write_current_state();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection with username ");
    Serial.print(settings.mqtt_username);
    Serial.print(" and password ");
    Serial.print(settings.mqtt_password);
    if (client.connect(settings.mqtt_id, settings.mqtt_username, settings.mqtt_password)) {
      Serial.println(" connected");
      // Once connected, publish an announcement...
      client.subscribe(subscribe_lights_topic);
      client.subscribe(subscribe_update_topic);
      blink(3, 200);
    } else {
      Serial.print(" failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      blink(50000, 100); //Blink fast in the mean time with 100ms intervals
      delay(5000);
    }
  }
  
}

void loop() {
  client.loop();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
