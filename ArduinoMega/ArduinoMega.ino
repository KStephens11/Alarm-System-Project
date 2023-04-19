// Librarys
#include <Keypad.h>
#include <Password.h>
#include <MFRC522.h>
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

// Pins for speaker and LED
const int LED = LED_BUILTIN;
const int SPEAKER = 48;

// Pins for Entry exit and zones
const int ENTRY_EXIT = 11;
const int ZONE[2] = {10,24};
const int ZONEDR[2] = {42,44};

// Variables used to hold the values of the alarms state and the zone states
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

// Used to keep track of the time just before the alarm enters the while loop when it trips
unsigned long timeNowAlarm;

void setup() {
  // Start serial connections with ESP and processing
  Serial.begin(57600); // For processing
  Serial1.begin(115200); // For ESP
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
  Serial1.write(NZONE);
  Serial1.flush();
}

// Main loop
void loop() {

  // Function check for input from esp
  myESP();

  // Call function to check if RFID has scanned a card, will arm and disarm the alarm if scanned.
  myRFID();

  // Calls function to check keypad and what is entered, if it matches the password the alarm is armed.
  myKey();

  // When the alarm has beeen activated the programme will enter this if statement
  if(ON_OFF_ACTIVATED) {

    // Check distance of the ultrasonic sensor
    distance = sr04.Distance();

    // Read entry exit sensor
    ENTRY_EXIT_STATE = digitalRead(ENTRY_EXIT);

    // Reads the sensors for the zones sensors and doors
    for (int x = 0; x < 2; x++) {
      ZONE_STATE[x] = digitalRead(ZONE[x]);
      ZONEDR_STATE[x] = digitalRead(ZONEDR[x]);
    }

    // Executed when entry exit is opened
    if(ENTRY_EXIT_STATE) {
      // Count down from 10 seconds to give time for user to enter password and disarm the alarm.
      countdown(10);
      // Entry Exit Activates alarm, will be overwritten if the alarm is disarmed.
      ENTRY_EXIT_ACTIVATED = HIGH;
      // Write to ESP and Processing
      Serial.write(ENTRY);
      Serial1.write(ENTRY);
    }
    // Check Ultrasonic sensor if there is an object within 8cm
    if (distance < 8) {
      ZONE_STATE[1] = HIGH;
    }

    // For loop to check if a zone has been activated
    for (int x = 0; x < 2; x++) {
      if(ZONE_STATE[x]) {
        // Write to ESP and Processing
        Serial.write(250-x);
        Serial1.write(250-x);
        // Zone Activated
        ZONE_ACTIVATED = HIGH;
      }
      //Check if zone door has been activated
      if(ZONEDR_STATE[x]) {
        // Write to ESP and Processing
        Serial.write(246-x);
        Serial1.write(246-x);
        // Zone Activated
        ZONE_ACTIVATED = HIGH;
      }

    }
    
  }
  // Saves current time until the alarm enters the while loop
  timeNowAlarm = millis();

  // This checks if the alarm is activated and if one of the zones or entry exit is activated
  // Will play sound the alarm and get input to disarm from Keypad, RFID and ESP
  while(ON_OFF_ACTIVATED&&ENTRY_EXIT_ACTIVATED||ON_OFF_ACTIVATED&&ZONE_ACTIVATED) {
    // playSpeaker will halt flashLED until it is done, this will make the led flash only after the speaker is done.
    playSpeaker(10000);
    // Flashes LED
    flashLED();
    // Check rfid as its not included in myDelay
    myRFID();
    // My delay checks Keypad and ESP
    myDelay(1000);
   
  }

}

// Function to play speaker, will only play for specified lenght of time.
void playSpeaker(int limit) {
  // Checks if the current time is less than the time saved before the loop plus the lenght of time it will play for.
  while (millis() < timeNowAlarm + limit) {
    myRFID();                 // Check rfid
    tone(SPEAKER, 4000, 500); // buzzer plays noise at 4000hz for 500ms
    myDelay(1000);            // wait 1 second
    myRFID();                 // check rfid
    noTone(SPEAKER);          // speaker is set to not play anything
    myDelay(1000);            // wait 1 seconc
  }
}

// Function to flash LED
void flashLED() {
  digitalWrite(LED, HIGH);  // turn the LED on (HIGH is the voltage level)
  myDelay(500);             // wait 500ms
  digitalWrite(LED, LOW);   // turn the LED off by making the voltage LOW
  myDelay(500);             // wait 500ms
}

// Function used to countdown from the defined period to 0
void countdown(int period){
  period++;
  int x = 0;
  // x will count upwards until it matches with period
  while(x<period){
    // need to add 10 to stop conflict with keypad
    // Sends period-x to have the values count downwards
    Serial.write((period-x) + 10);
    Serial.flush();
    Serial1.write((period-x) + 10);
    myDelay(1000);
    // Checks RFID card
    myRFID();
    x++;
  }

}

// Function used as an alternative to the delay() function
// myKey and myESP are checked. myRFID does not.
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

// Function used to process the inputted keys, used to print the inputted key and to keep track of the password lenght and append keys to the password
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
    //Check if there is new card
    if (rfid.PICC_IsNewCardPresent()) {
      // Read the card
      rfid.PICC_ReadCardSerial();
      // Set readBytes to 0 before reading
      readBytes = 0;
      // For loop to compare stored uid bytes with scanned rfid cards bytes
      for(int i = 0; i < 4; i++ ){
        if (rfid.uid.uidByte[i] == rfidBytes[i]){
          // Increment every match
          readBytes++;
        }
        // Once all matches delay for 1 second and set toggle alarm
        if (readBytes == 4) {
          myDelay(1000);
          alarmSet();
        }
      }
  }
}

// Function to arm and disarm the alarm, the function will toggle ON_OFF_ACTIVATED to achieve this. Will send status to esp and processing
// The fucntion also sets the zones to low
// A countdown of 8 seconds is set when armed to allow the person to leave the building
void alarmSet(){
  // toggle alarm state and set zones and exit entry to low
  ON_OFF_ACTIVATED = !ON_OFF_ACTIVATED;
  ENTRY_EXIT_ACTIVATED = LOW;
  ZONE_ACTIVATED = LOW;
  // Write NoZones to ESP and Processing
  Serial1.write(NZONE);
  Serial.write(NZONE);
  Serial1.flush();
  Serial.flush();
  // If the zone is now toggled to armed
  if (ON_OFF_ACTIVATED) {
    // Send event code armed to esp and processing
    Serial.write(ARMED);
    Serial1.write(ARMED);
    Serial1.flush();
    Serial.flush();
    // Count down from 8 seconds giving time to leave
    countdown(8);
  }
  // If alarm is now toggled to disarm
  else {
    // Send disarm event to ESP and Processing
    Serial.write(DSARM);
    Serial1.write(DSARM);
    Serial1.flush();
    Serial.flush();
  }
}

// Fucntion to check if there is any input from the blynk app through the ESP
void myESP(){
  // While loop to read all data that has been sent
  while(Serial1.available()>0) {
    // stores data as mes
    int mes = Serial1.read();
    // If the message was the event code state the toggle the alarm.
    if (mes == STATE) {
      alarmSet();
    }
  }
}