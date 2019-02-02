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
#include <SPI.h>
#include <Ticker.h>
#include <MFRC522.h>  // Library for Mifare RC522 Devices
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

Ticker unlocker;
Ticker blinker;


#define RST_PIN D1  // RST-PIN für RC522 - RFID - SPI - Modul GPIO15 
#define SS_PIN  D2 // SDA-PIN für RC522 - RFID - SPI - Modul GPIO2 

#define BUZZER_PIN D3 //buzzer
#define LED_PIN2 D4 //red 
#define TAGSIZE 12
#define RELAY_PIN 10
#define SWITCH_PIN D8 //Switch
#define LED_PIN D0


uint8_t successRead; //variable integer to keep if we hace successful read

uint8_t numblink;
uint8_t maxblink;

byte readCard[8]; //Stores scanned ID
char temp[3];
char cardID[9];

const int switch_interrupt = digitalPinToInterrupt(SWITCH_PIN);

// Update these with values suitable for your network.
const char* ssid = "";
const char* password = "";
const char* mqtt_server = "";
const char* mqtt_username = "";
const char* mqtt_password = "";
const char* mqtt_id = "door";
const char* cardread_topic = "smartclassroom/Door/cardread";
const char* lock_topic = "smartclassroom/Door/open";
const char* door_open_topic = "smartclassroom/Door/open";
const char* door_open_announce_topic = "smartclassroom/Door/announce/open";
uint8_t tags;

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
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(LED_PIN2, HIGH);
  lock();
  Serial.begin(115200);
  SPI.begin();

  mfrc522.PCD_Init(); //Initialize MFRC522 hardware
  delay(250);
  setup_ota();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  reconnect();
  ShowReaderDetails();
  digitalWrite(RELAY_PIN, HIGH);
  client.publish(door_open_announce_topic, "false");
  Serial.println(F("-------------------"));
  Serial.println(F("Everything Ready"));
  Serial.println(("Waiting PICCs to be scanned"));
}

void setup_ota() {
  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname("Door");
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
}

void blink(uint8_t num_blink, uint8_t delay_time){
  //blink num_blink times with delay_time interval
  Serial.println();
  Serial.println("Blink");
  maxblink = num_blink;
  numblink = 0;
  blinker.attach_ms(delay_time, switch_led);
}

void switch_led(){
  //turn the LED off if its on, or on if its off, as long as we should be blinking
  uint8_t led_state = digitalRead(LED_PIN);
  if(led_state == HIGH){
    //turn on
    digitalWrite(LED_PIN, LOW);
  }
  else{
    //turn off again, and advance num_blink;
    digitalWrite(LED_PIN, HIGH);
    numblink++;
  }
  if(numblink>=maxblink){
    //stop blinking because we blinked the maximum number
    blinker.detach();
  }
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
  //callback for handling incoming MQTT messages for topics we subscribed too
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  blink(4, 200);
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


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    int reconnect_time = 0;
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(mqtt_id, mqtt_username, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(lock_topic);
      client.subscribe(door_open_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
    blink(3,200);
  }
}

bool ota_flag = true;
uint16_t time_elapsed = 0;

void loop() {

  
  if (ota_flag)
  {
    while (time_elapsed < 15000)
    {
      ArduinoOTA.handle();
      time_elapsed = millis();
      delay(10);
    }
    ota_flag = false;
  }
  delay(500); 
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
  attachInterrupt(switch_interrupt, unlock_button, RISING); //attach the interrupt again to listen for the unlock button press
  if(digitalRead(RELAY_PIN) == LOW){
    digitalWrite(RELAY_PIN, HIGH);
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println("Lock");
  }
  else{
    Serial.println("Already locked");
  }
}
