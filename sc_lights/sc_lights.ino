#include <smart_classroom.h>

// Update these with values suitable for your network.


#define RELAY_PIN 3
#define LED_PIN 1
#define LED2_PIN 1

const char* subscribe_lights_topic = "smartclassroom/Lights/on";
const char* subscribe_update_topic = "smartclassroom/Lights/update";

struct State{
  bool lights;
};

State state;

char payload_value[50];
SmartClassroom sc;

void sub_func(){
  sc.mqtt.subscribe(subscribe_lights_topic);
  sc.mqtt.subscribe(subscribe_update_topic);
}

void setup() {
  // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  
  sc.begin(LED_PIN, LED2_PIN);
  read_current_state();
  if(state.lights){
    digitalWrite(RELAY_PIN, HIGH);  
  }
  else{
    digitalWrite(RELAY_PIN, LOW);
  }
  sc.mqtt.setCallback(callback);
  sc.reconnect(sub_func);
  
  Serial.println(F("Everything Ready"));
  pinMode(LED_PIN, OUTPUT);
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

void callback(char* topic, byte* payload, unsigned int length) {
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println("");
  sc.blink(4, 200);

  if (strcmp(topic, subscribe_update_topic) == 0) {
    sc.checkUpdate((char *)payload);
    return;
  }

  memset(payload_value, 0, sizeof(payload_value));
  strncpy(payload_value, (char *)payload, length);

  if (strcmp(payload_value, "true") == 0) {
    digitalWrite(RELAY_PIN, HIGH);
    state.lights = 1;
  }
  else if (strcmp(payload_value, "false") == 0) {
    digitalWrite(RELAY_PIN, LOW);
    state.lights = 0;
  }
  write_current_state();
}

void loop() {
  if (!sc.mqtt.connected()) {
    sc.reconnect(sub_func);
  }
  sc.mqtt.loop();
  delay(500);
}
