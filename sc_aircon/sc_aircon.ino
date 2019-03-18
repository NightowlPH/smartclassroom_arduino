/*
This sketch uses and ESP8266/nodemcu board to control a SANYO airconditioning through IR remote control codes.

 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"

IR send pin: D1

*/
#include <smart_classroom.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Daikin.h>



// Update these with values suitable for your network.
#define IR_SEND_PIN 3
#define LED_PIN 1
#define LED2_PIN 1

IRDaikinESP ac(IR_SEND_PIN);
IRsend irsend(IR_SEND_PIN);

#define SANYO_AC 0

const char* version = "1.2";

const char* subscribe_aircon_on = "smartclassroom/Aircon/on";
const char* subscribe_update_topic = "smartclassroom/Aircon/update";
const char* subscribe_aircon_temperature = "smartclassroom/Aircon temperature/on";
const char* sub_topics[] = {subscribe_aircon_on, subscribe_update_topic, subscribe_aircon_temperature};


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

char payload_value[50];
SmartClassroom sc;

void setup() {
  setup_ir();
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  sc.begin(LED_PIN, LED2_PIN);
  sc.mqtt.setCallback(callback);
  sc.reconnect(sub_topics, sizeof(sub_topics)/sizeof(*sub_topics), version);
  Serial.println(F("-------------------"));
  Serial.println(F("Everything Ready"));
}

void setup_ir() {
  #if SANYO_AC
    setup_ir_sanyo();
  #else
    setup_ir_daikin();
  #endif
}

void setup_ir_daikin() {
  ac.begin();
}

void setup_ir_sanyo() {
  pinMode(IR_SEND_PIN, FUNCTION_3);
  irsend.begin();
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  sc.blink(4, 200);

  memset(payload_value, 0, sizeof(payload_value));
  strncpy(payload_value, (char *)payload, length);
  
  if(strcmp(topic, subscribe_update_topic) == 0){
    sc.checkUpdate(payload_value);
    return;
  }

  if(strcmp(payload_value, "true")==0) {
    aircon_on();
    return;
  }
  else if(strcmp(payload_value, "false")==0) {
    aircon_off();
    return;
  }
  int temp = atoi(payload_value);
  if(temp >= 16 & temp <= 26){
    aircon_temp(temp);
    return;
  }
  
}

void loop() {
  if (!sc.mqtt.connected()) {
    sc.reconnect(sub_topics, sizeof(sub_topics)/sizeof(*sub_topics), version);
  }
  sc.mqtt.loop();
  delay(500); 
}

void aircon_on(){
  Serial.println("Turning aircon on");
  #if SANYO_AC
    aircon_on_sanyo();
  #else
    aircon_on_daikin();
  #endif
}

void aircon_on_sanyo() {
  irsend.sendRaw(sanyo_data_on, 73, 38);  // Send a raw data capture at 38kHz.
}

void aircon_on_daikin() {
  ac.on();
  ac.setMode(3);
  ac.setFan(10);
  ac.setSwingVertical(true);
  ac.setSwingHorizontal(true);
  ac.send();
}

void aircon_off_daikin() {
    ac.off();
    ac.send();
}

void aircon_off_sanyo() {
    irsend.sendRaw(sanyo_data_off, 73, 38);  // Send a raw data capture at 38kHz.
}

void aircon_off() {
  Serial.println("Turning aircon off");
  #if SANYO_AC
    aircon_off_sanyo();
  #else
    aircon_off_daikin();
  #endif
}

void aircon_temp(int temp) {
  Serial.printf("Setting temp to %d\n", temp);
  #if SANYO_AC
    aircon_temp_sanyo(temp);
  #else
    aircon_temp_daikin(temp);
  #endif
}

void aircon_temp_sanyo(int temp) {
  irsend.sendRaw(sanyo_temp_data[temp-16], 73, 38);  // Send a raw data capture at 38kHz.
}

void aircon_temp_daikin(int temp) {
  ac.setTemp(temp);
  ac.send();
}
