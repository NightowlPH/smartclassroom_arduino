#include <smart_classroom.h>
#include <SPI.h>
#include <MFRC522.h>  // Library for Mifare RC522 Devices
 
Ticker unlocker;

#define RST_PIN D1  
#define SS_PIN  D2 

#define BUZZER_PIN D3 //buzzer
#define TAGSIZE 12
#define RELAY_PIN 10
#define LED_PIN LED_BUILTIN
#define LED2_PIN D4

#define SWITCH_PIN_MODE INPUT
#define SWITCH_PIN_DETECT_EDGE RISING
#define SWITCH_PIN D8

uint8_t successRead; //variable integer to keep if we hace successful read

byte readCard[8]; //Stores scanned ID
char temp[3];
char cardID[9];

const int switch_interrupt = digitalPinToInterrupt(SWITCH_PIN);

const char* cardread_topic = "smartclassroom/Door/cardread";
const char* lock_topic = "smartclassroom/Door/open";
const char* door_open_topic = "smartclassroom/Door/open";
const char* door_open_announce_topic = "smartclassroom/Door/announce/open";
const char* update_topic = "smartclassroom/Door/update";

SmartClassroom sc;
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
MFRC522::MIFARE_Key key;

void sub_func(){
    sc.mqtt.subscribe(lock_topic);
    sc.mqtt.subscribe(door_open_topic);
    sc.mqtt.subscribe(update_topic);
}

char payload_value[50];

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(SWITCH_PIN, SWITCH_PIN_MODE);
  lock();
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init(); //Initialize MFRC522 hardware
  sc.begin(LED_PIN, LED2_PIN);
  //ShowReaderDetails();
  sc.mqtt.publish(door_open_announce_topic, "false");
  Serial.println(F("-------------------"));
  Serial.println(F("Everything Ready"));
  Serial.println(("Waiting PICCs to be scanned"));
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

  sc.blink(4, 200);

  if(strcmp(topic, update_topic) == 0){
    sc.checkUpdate((char *)payload);
    return;
  }
  
  memset(payload_value, 0, sizeof(payload_value));
  strncpy(payload_value, (char *)payload, length);

  if (strcmp(payload_value, "true") == 0) {
    //digitalWrite(LED_PIN, HIGH);
    unlock(5000);
  }
  else if (strcmp(payload_value, "false") == 0) {
    //digitalWrite(LED_PIN, LOW);
    lock();
  }
}

void loop() {
  
  do {
    if (!sc.mqtt.connected()) {
      sc.reconnect(sub_func);
    }
    sc.mqtt.loop();
    successRead = getID();
  }
  while (!successRead);
  Serial.println("");
  Serial.println("Publishing: ");
  sc.mqtt.publish(cardread_topic, cardID);
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
  sc.mqtt.publish(door_open_announce_topic, "true");
  unlock(5000);
}

void unlock(int unlock_time) {
  //unlock the door for unlock_time milliseconds. If unlock_time <=0, stay open
  sc.blink(2,200);
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
  sc.mqtt.publish(door_open_announce_topic, "false");
  lock();
}

void lock() { 
  //lock the door
  sc.blink(2, 200);
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
