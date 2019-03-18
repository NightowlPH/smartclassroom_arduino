#include "smart_classroom.h"

void SmartClassroom::begin(uint8_t led_pin, uint8_t led2_pin){
    SPIFFS.begin();
    m_led_pin = led_pin;
    m_led2_pin = led2_pin;
    pinMode(m_led_pin, OUTPUT);
    pinMode(m_led2_pin, OUTPUT);
    mqtt = PubSubClient(espClient);
    set_settings(read_settings());
    setup_wifi();
    mqtt.setServer(settings.mqtt_server, 1883);
}


void SmartClassroom::reconnect(const char* topics[MAXTOPICS], uint8_t numtopics, const char* version) {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect(settings.mqtt_id, settings.mqtt_username, settings.mqtt_password)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      char msg[60];
      sprintf(msg, "{\"version\": \"%s\", \"mqtt_id\": \"%s\"}", version, settings.mqtt_id);
      mqtt.publish("smartclassroom/connected", msg);
      for(int i=0;i<numtopics;i++){
          mqtt.subscribe(topics[i]);
      }
      blink(3, 200);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      blink(50000, 100); //Blink fast in the mean time with 100ms intervals
      delay(5000);
    }
  }
  
}

void SmartClassroom::set_settings(JsonObject& root){
  strlcpy(settings.ssid, root["ssid"], sizeof(settings.ssid));
  strlcpy(settings.password, root["password"], sizeof(settings.password));
  strlcpy(settings.mqtt_server, root["mqtt_server"], sizeof(settings.mqtt_server));
  strlcpy(settings.mqtt_username, root["mqtt_username"], sizeof(settings.mqtt_username));
  strlcpy(settings.mqtt_password, root["mqtt_password"], sizeof(settings.mqtt_password));
  strlcpy(settings.mqtt_id, root["mqtt_id"], sizeof(settings.mqtt_id));
}

JsonObject& SmartClassroom::read_settings() {
  File f = SPIFFS.open("/settings.json", "r");
  Serial.println("");
  Serial.println("Opening /settings.json");
  if(!f){
    Serial.println("Failed to read file");
  }
  const size_t capacity = JSON_OBJECT_SIZE(6) + 160;
  DynamicJsonBuffer jsonBuffer(capacity);
  JsonObject& root = jsonBuffer.parseObject(f);
  f.close();
  if(!root.success()){
    Serial.println("Failed to read file");
  }
  else{
    Serial.println("Finished parsing settings file");
  }
  return root;
}

void SmartClassroom::checkUpdate(char* update_uri) {
  Serial.print("Checking for update on url ");
  Serial.println(update_uri);
  ESPhttpUpdate.setLedPin(m_led_pin, LOW);
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


void SmartClassroom::blink(uint32_t num_blink, uint16_t delay_time){
  //blink num_blink times with delay_time interval
  maxblink = num_blink;
  numblink = 0;
  blinker.attach_ms(delay_time, switch_led, this);
}

void switch_led(SmartClassroom *sc) {
  //turn the LED off if its on, or on if its off, as long as we should be blinking
  uint8_t led_state = digitalRead(sc->m_led_pin);
  digitalWrite(sc->m_led_pin, !led_state);
  if (led_state == LOW) {
    //LED was on, now turned off again, advance num_blink;
    sc->numblink++;
  }
  if (sc->numblink >= sc->maxblink) {
    //stop blinking because we blinked the maximum number
    sc->blinker.detach();
  }
}

void SmartClassroom::setup_wifi() {

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
