#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <Ticker.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <MFRC522.h>  // Library for Mifare RC522 Devices
 
Ticker unlocker;
Ticker blinker;


#define RST_PIN D1  
#define SS_PIN  D2 

#define BUZZER_PIN D3 //buzzer
#define LED_PIN2 D4
#define TAGSIZE 12
#define RELAY_PIN 10
#define LED_PIN LED_BUILTIN

#define SWITCH_PIN_MODE INPUT
#define SWITCH_PIN_DETECT_EDGE RISING
#define SWITCH_PIN D8





uint8_t successRead; //variable integer to keep if we hace successful read

uint32_t numblink;
uint32_t maxblink;

byte readCard[8]; //Stores scanned ID
char temp[3];
char cardID[9];
char c;

const int switch_interrupt = digitalPinToInterrupt(SWITCH_PIN);

struct Settings{
  char ssid[64];
  char password[64];
  char mqtt_server[15];
  char mqtt_username[32];
  char mqtt_password[32];
  char mqtt_id[32];
};

const char* cardread_topic = "smartclassroom/Door/cardread";
const char* lock_topic = "smartclassroom/Door/open";
const char* door_open_topic = "smartclassroom/Door/open";
const char* door_open_announce_topic = "smartclassroom/Door/announce/open";
const char* subscribe_update_topic = "smartclassroom/Door/update";

uint8_t tags;

Settings settings;
WiFiClient espClient;
PubSubClient client(espClient);
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
MFRC522::MIFARE_Key key;

char button_value[50];

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN2, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(SWITCH_PIN, SWITCH_PIN_MODE);
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(LED_PIN2, HIGH);
  lock();
  Serial.begin(115200);
  SPIFFS.begin();
  read_settings();
  SPI.begin();

  mfrc522.PCD_Init(); //Initialize MFRC522 hardware
  delay(250);
  setup_wifi();
  Serial.print("MQTT Server ");
  Serial.println(settings.mqtt_server);
  client.setServer(settings.mqtt_server, 1883);
  client.setCallback(callback);
  reconnect();
  //ShowReaderDetails();
  client.publish(door_open_announce_topic, "false");
  Serial.println(F("-------------------"));
  Serial.println(F("Everything Ready"));
  Serial.println(("Waiting PICCs to be scanned"));
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

void callback(char* topic, byte* payload, unsigned int length) {
  //callback for handling incoming MQTT messages for topics we subscribed too
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

  if (strcmp(button_value, "true") == 0) {
    //digitalWrite(LED_PIN, HIGH);
    unlock(5000);
  }
  else if (strcmp(button_value, "false") == 0) {
    //digitalWrite(LED_PIN, LOW);
    lock();
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

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection with username ");
    Serial.print(settings.mqtt_username);
    Serial.print(" and password ");
    Serial.print(settings.mqtt_password);
    Serial.print("...");
    // Attempt to connect
    if (client.connect(settings.mqtt_id, settings.mqtt_username, settings.mqtt_password)) {
      Serial.println("connected");
      client.subscribe(lock_topic);
      client.subscribe(door_open_topic);
      client.subscribe(subscribe_update_topic);
      blink(3,200);
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

void loop() {
  
  do {
    if (!client.connected()) {
      reconnect();
    }
    client.loop();
    successRead = getID();
  }
  while (!successRead);
  Serial.println("");
  Serial.println("Publishing: ");
  client.publish(cardread_topic, cardID);
  Serial.println(cardID);
}

///////////////////////////////////////// Get PICC's UID ///////////////////////////////////
uint8_t getID() {
  // Getting ready for Reading PICCs
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Since a PICC placed get Serial and continue
    return 0;
  }
  // There are Mifare PICCs which have 4 byte or 7 byte UID care if you use 7 byte PICC
  // I think we should assume every PICC as they have 4 byte UID
  // Until we support 7 byte PICCs
  Serial.println(F("Scanned PICC's UID:"));
  int j = 0;
  for ( uint8_t i = 0; i < mfrc522.uid.size; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
    Serial.print(readCard[i], HEX);
    sprintf(temp, "%02X", readCard[i]);
    cardID[j] = temp[0];
    cardID[j + 1] = temp[1];
    j = j + 2;
  }

  mfrc522.PICC_HaltA(); // Stop reading
  return 1;
}

void ShowReaderDetails() {
  // Get the MFRC522 software version
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.print(F("MFRC522 Software Version: 0x"));
  Serial.print(v, HEX);
  if (v == 0x91)
    Serial.print(F(" = v1.0"));
  else if (v == 0x92)
    Serial.print(F(" = v2.0"));
  else
    Serial.print(F(" (unknown),probably a chinese clone?"));
  Serial.println("");
  // When 0x00 or 0xFF is returned, communication probably failed
  if ((v == 0x00) || (v == 0xFF)) {
    Serial.println(F("WARNING: Communication failure, is the MFRC522 properly connected?"));
    Serial.println(F("SYSTEM HALTED: Check connections."));
    // Visualize system is halted
    while (true); // do not go further
  }
}

void unlock_button() {
  //called when the button to unlock the door is pressed through an interrupt
  detachInterrupt(switch_interrupt); //Make sure the interrupt isn't triggered as long as the door is open.
  Serial.println("Button pressed");
  client.publish(door_open_announce_topic, "true");
  unlock(5000);
}

void unlock(int unlock_time) {
  //unlock the door for unlock_time milliseconds. If unlock_time <=0, stay open
  blink(2,200);
  Serial.println("Unlocking");
  if(digitalRead(RELAY_PIN) == HIGH){
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(BUZZER_PIN, HIGH);
    //  digitalWrite(LED_PIN2, LOW);
    Serial.println("Unlock");
    if(unlock_time>0){
      //set the timer to lock the door in unlock_time milliseconds
      unlocker.attach_ms(unlock_time, auto_lock);
    }
  }
  else{
    Serial.println("Already unlocked");
  }
}

void auto_lock(){
  //called when the door is locked automatically by the code
  unlocker.detach();
  client.publish(door_open_announce_topic, "false");
  lock();
}

void lock() { 
  //lock the door
  blink(2, 200);
  Serial.println("Locking");
  attachInterrupt(switch_interrupt, unlock_button, SWITCH_PIN_DETECT_EDGE); //attach the interrupt again to listen for the unlock button press
  if(digitalRead(RELAY_PIN) == LOW){
    digitalWrite(RELAY_PIN, HIGH);
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println("Lock");
  }
  else{
    Serial.println("Already locked");
  }
} 
