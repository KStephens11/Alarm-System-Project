// Librarys
#include <Keypad.h>
#include <Password.h>
#include <MFRC522.h>
//https://github.com/miguelbalboa/rfid
#include "SR04.h"

// Defines the Slave Select pin and reset pin numbers, control the communication between the arduino and the RFID scanner
#define SS_PIN 53
#define RST_PIN 49

// Pins for HC-SR04 ultrasonic sensor
#define TRIG_PIN 24
#define ECHO_PIN 25

// Code that is sent to ESP and Processing
#define STATE 255
#define ARMED 254
#define DSARM 253
#define NZONE 252
#define ENTRY 251
#define ZONE0 250
#define ZONE1 249
#define KCHCK 248
#define KRSET 247

// An array to store authorized rfid bytes
// 188 125 31 73
byte rfidBytes[4] = {188, 125, 31, 73};

// Sets maximum password length and counter for the currently entered password length
byte maxPasswordLength = 6; 
byte currentPasswordLength = 0;

// Define how many  rows and columns the keypad has
const byte ROWS = 4; // Four rows
const byte COLS = 4; // Four columns
 
//Defines the keys used on the keypad
char keys[ROWS][COLS] = {
{'1','2','3','A'},
{'4','5','6','B'},
{'7','8','9','C'},
{'*','0','#','D'}
};

// Connects the rows and columns of the keypad to the pins of the arduino
byte rowPins[ROWS] = {32, 33, 34, 35};
byte colPins[COLS] = {36, 37, 38, 39};

// Create keypad object and within it has the makeKeymap function with the values needed to create the keypad object
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

// Create rfid object with the pin values of SS_PIN and RST_PIN given
MFRC522 rfid(SS_PIN, RST_PIN);

// Creates object password with the password value of 1234
Password password = Password("1234");

// Creates object for the ultrasonic sensor called sr04 using trig and echo pins.
SR04 sr04 = SR04(ECHO_PIN,TRIG_PIN);

// Pins for sensors and led
const int LED = LED_BUILTIN;
const int SPEAKER = 48;

// Pins for arming the alarm and its zones
//const int ON_OFF = 12;
const int ENTRY_EXIT = 11;
const int ZONE[2] = {10,24};
// Zone 0 door is pin 44 and 1 is 42
const int ZONEDR[2] = {42,44};

// Variables used to hold the values of the alarms state and the zone states
int ON_OFF_STATE;
int ENTRY_EXIT_STATE;
int ZONE_STATE[2];
int ZONEDR_STATE[2];

// Variables to track if the alarm and zones have been activated, set to low to ensure they are not set off when starting
int ON_OFF_ACTIVATED = LOW;
int ENTRY_EXIT_ACTIVATED = LOW;
int ZONE_ACTIVATED = LOW;

// Distance detected by the ultrasonic sensor
long distance;
int readBytes;

void setup() {
  Serial.begin(57600); // Starts serial connection
  Serial1.begin(115200); // Start serial connection with esp8266
  SPI.begin(); // Initializes the SPI bus, used to communicate with the rfid scanner
  rfid.PCD_Init(); // Initializes the RFID Scanner

  // Sets pinmodes
  pinMode(LED, OUTPUT);
  pinMode(SPEAKER, OUTPUT);
  pinMode(ENTRY_EXIT, INPUT);

  // Pinmodes for zones
  pinMode(ZONE[0], INPUT);
  pinMode(ZONE[1], OUTPUT);

  // Pinmodes for doors
  pinMode(ZONEDR[0], INPUT);
  pinMode(ZONEDR[1], INPUT);

  //Tell esp that alarm is disarmed and no zones active on startup
  Serial1.write(DSARM);
  Serial1.flush();
  Serial1.write(NZONE);
  
}

// Main loop
void loop() {

  // Function check for input from esp
  myESP();

  // Call function to check if RFID has scanned a card, will arm and disarm the alarm if scanned.
  myRFID();

  // Calls function to check keypad and what is entered and if it matches the password. If it matches the alarm arms.
  myKey();

  // When the alarm is activated it will begin checking the Entry exit and zones
  if(ON_OFF_ACTIVATED) {

    distance = sr04.Distance();

    // Check if entry exit zone has been activated
    ENTRY_EXIT_STATE = digitalRead(ENTRY_EXIT);

    // Checks if the one of the zones were activated
    for (int x = 0; x < 2; x++) {
      ZONE_STATE[x] = digitalRead(ZONE[x]);
      ZONEDR_STATE[x] = digitalRead(ZONEDR[x]);
    }

    // When the entry exit zone is activated it will countdown 10 seconds to allow the person to enter the password
    if(ENTRY_EXIT_STATE) {
      countdown(10);
      Serial.write(ENTRY);
      ENTRY_EXIT_ACTIVATED = HIGH;
      Serial1.write(ENTRY);
    }
    // Check Ultrasonic sensor if there is an object within 8cm
    if (distance < 8) {
      ZONE_STATE[1] = HIGH;
    }

    // For loop to check if a zone has been activated
    for (int x = 0; x < 2; x++) {
      if(ZONE_STATE[x]) {
        Serial.write(250-x);
        Serial1.write(250-x);
        ZONE_ACTIVATED = HIGH;
      }
      //Check if zone door has been activated
      if(ZONEDR_STATE[x]) {
        //Serial.print("Zone ");
        //Serial.print(x);
        //Serial.println(" Activated");
        Serial.write(246-x);
        Serial1.write(246-x);
        ZONE_ACTIVATED = HIGH;
      }

    }

  }

  // This checks if the alarm is activated and if one of the zones are activated, if so it will flash the LED
  // ans starts playing the buzzer. It will look for the keypads input, which is inside the myDelay function
  // RFID was not added intentionally so the alarm will only be deactivated once tripped with the keypad
  while(ON_OFF_ACTIVATED&&ENTRY_EXIT_ACTIVATED||ON_OFF_ACTIVATED&&ZONE_ACTIVATED) {
    flash();
    tone(SPEAKER, 10000, 100);
    myDelay(100);
    noTone(SPEAKER);
    myRFID();
  }

}

// Function to flash the LED, has a one second delay between each state
void flash() {
  digitalWrite(LED, HIGH);  // turn the LED on (HIGH is the voltage level)
  myDelay(1000);                      // wait for a second
  digitalWrite(LED, LOW);   // turn the LED off by making the voltage LOW
  myDelay(1000);                      // wait for a second
}

// Function used to countdown from the defined period to 0
void countdown(int period){
  period++;
  int x = 0;
  while(x<period){
    //Serial.print("Countdown "); Serial.println(period-x);
    // need to add 10 to stop conflict with keypad
    Serial.write((period-x) + 10);
    Serial.flush();
    Serial1.write((period-x) + 10);
    myDelay(1000);
    x++;
  }

}

// Function used as an alternative to the delay() function
// myKey function included to allow keypad input during a delay
void myDelay(int period) {
  unsigned long timeNow = millis();
  while (millis() < timeNow + period) {myKey();myESP();}
}

// Function to start the process of taking inputs from the keypad, processing the keys and checking the password
// It sends its inputted key to the processNumberKey function and start the checkPassword and resetPassword function
void myKey(){
  char key = keypad.getKey();
   if (key != NO_KEY){
      myDelay(60); 
      switch (key){
      case 'A': break; 
      case 'B': break; 
      case 'C': break; 
      case 'D': break; 
      case '#': checkPassword(); Serial.write(KCHCK); break;
      case '*': resetPassword(); Serial.write(KRSET); break;
      default: processNumberKey(key);
      }
   }
}

// Function used to process the inputted keys, used to print the inputted key and to keep track of the password kenght and append keys to the password
// When password is above the max password length it will start the checkPassword function
void processNumberKey(char key) {
   Serial.write((key+1)-'0');
   Serial1.write((key+1)-'0');
   currentPasswordLength++;
   password.append(key);
   if (currentPasswordLength == maxPasswordLength) {
      checkPassword();
   } 
}

// Function to check if the entered password matches the set password, used the password.evaluate to achieve this
// When password is correct OK is printed and the alarmSet function is called
// It will reset the entered password each time to allow the password to be re-entered 
void checkPassword() {
   if (password.evaluate()){
      alarmSet();
   } 
   resetPassword();
}

// Function to reset password, uses function from library to reset password and sets current password length to 0
void resetPassword() {
   password.reset(); 
   currentPasswordLength = 0; 
}

// Function to check if there is a new card detected, scan the card and check if the uid bytes matches the ones set by rfidBytes
// if it matches x is incremented, if x == 4, a one second delay is called and then alarmSet
void myRFID() {
    if (rfid.PICC_IsNewCardPresent()) {
      rfid.PICC_ReadCardSerial();
      readBytes = 0;
      for(int i = 0; i < 4; i++ ){
        if (rfid.uid.uidByte[i] == rfidBytes[i]){
          readBytes++;
        }
      }

      if (readBytes == 4) {
        myDelay(1000);
        alarmSet();
        readBytes=0;
        for(int i = 0; i < 4; i++ ){
          rfid.uid.uidByte[i] == 0;
        }
      }

      
      
  }
}

// Function to arm and disarm the alarm, the function will toggle ON_OFF_ACTIVATED and print a message depending on if it has been toggled to 0 or 1
// The fucntion also sets the zones to low
// A countdown of 8 seconds is set when armed to allow the person to leave the building
void alarmSet(){
  ON_OFF_ACTIVATED = !ON_OFF_ACTIVATED;
  ENTRY_EXIT_ACTIVATED = LOW;
  ZONE_ACTIVATED = LOW;
  Serial1.write(NZONE);
  Serial.write(NZONE);
  Serial.flush();
  if (ON_OFF_ACTIVATED) {
    Serial.write(ARMED);
    Serial.flush();
    Serial1.write(ARMED);
    countdown(8);
  }
  else {
    Serial.write(DSARM);
    Serial.flush();
    Serial1.write(DSARM);
  }
}

void myESP(){
  while(Serial1.available()>0) {
    int mes = Serial1.read();
    if (mes == STATE) {
      alarmSet();
    }

    if (mes < 11 & mes >= 1) {
      processNumberKey(mes - 1);
    }
  }
}