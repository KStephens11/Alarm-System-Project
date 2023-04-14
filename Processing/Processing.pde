// Serial library
import processing.serial.*;
Serial serial;

//Colour of rooms
int roomColour[] = {color(255, 255, 255), color(255, 255, 255), color(255, 255, 255), color(255, 255, 255), color(100, 100, 100)};
//Size of house
int houseSizeX = 600, houseSizeY = 300;
int housePosX = 50, housePosY = 50;
// Variable to hold inout from arduino mega
int input;
String alarmState = "Disarmed";
int countdown = 0;
int passwordCount = 0;
String passwordEntered = "";
String zoneState = "No Zones Active";

void setup() {
  // Set size of window
  size(900, 400);
  // Try to use /dev/ttyACM0 otherwise use COM3
  try {
    serial = new Serial(this, "/dev/ttyACM0",57600);
  }
  catch(Exception RuntimeException) {
    serial = new Serial(this, "COM3",57600);
  }
}

void draw() {
  print(input);
  // Set background colour. Also needed for info to change.
  background(255);
   // While data is being sent
   while (serial.available() > 0) {
     // Set information from arduino as input
     input = serial.read();
     // Check if input is between 10 and 30, these numbers are reserved for countdown
     if (input < 30 & input > 10) {
      // Count down must be subtracted by 11 as the countdown sent from the arduino will start from 1 to 11, having it send 0 cause problems with displaying the countdown.
      countdown = (input - 11);
      print(countdown);
     }
     // Check in data sent is between 1 and 10, these numbers are reserved for the keypad.
     if (input <= 10 & input >= 1) {
       // password enterd is added to variable passwordEntered
       passwordEntered += input - 1;
       // Everytime a number is entered the count will increase
       passwordCount++;
       // If the counter reaches 7 set the count to 0 and empty passwordEntered. Password entered is then set to the last entered key.
       if (passwordCount == 7) {
        passwordCount = 0;
        passwordEntered = "";
        passwordEntered += input - 1;
       }

     }
     
     // Switch statement to handle sent events from the arduino
     switch(input) {
      case 254:
        println("Alarm Armed");
        alarmState = "Armed";
        break;
      case 253:
        println("Alarm Disarmed");
        // Set all rooms back to default colour
        for (int x = 0; x < 3; x++){
          roomColour[x] = color(255, 255, 255);
        }
        roomColour[4] = color(100, 100, 100);
        alarmState = "Disarmed";
        break;
      case 252:
        println("No Zones Active");
        zoneState = "No Zones Active";
        break;
      case 251:
        // Check if alarm is armed before drawing entry exit as activated. Without this the zone would activate even with alarm disarmed.
        if(alarmState == "Armed"){
          println("Entry/Exit Active");
          zoneState = "Entry/Exit Active";
          roomColour[4] = color(255, 0, 0);
        }
        break;
      case 250:
        println("Zone 0 Activated");
        zoneState = "Zone 0 Activated";
        roomColour[0] = color(255, 0, 0);
        break;
      case 249:
        println("Zone 1 Activated");
        zoneState = "Zone 1 Activated";
        roomColour[1] = color(255, 0, 0);
        break;
      case 248:
        passwordEntered = "";
        passwordCount = 0;
        break;
      case 247:
        passwordEntered = "";
        passwordCount = 0;
        break;
      case 246:
        println("Door 0 Activated");
        zoneState = "Door 0 Activated";
        roomColour[0] = color(255, 0, 0);
        break;
        
      case 245:
        println("Door 1 Activated");
        zoneState = "Door 1 Activated";
        roomColour[1] = color(255, 0, 0);
        break;
      }
    }
  
  
   
  // Colour of the hallway
  fill(roomColour[4]);
  // Draw house
  rect(housePosX, housePosY, houseSizeX, houseSizeY);
  // Creates 2 rooms
  for(int x = 0; x < 2; x++) {
    // Set room colour
    fill(roomColour[x]);
    // Draw rooms
    rect(housePosX + (houseSizeX/2 * x), housePosY, houseSizeX/2+1, (houseSizeY / 1.5));
  }
  // Information next to house
  // Text is set to size 16
  textSize(16);
  // ??????? check this  out
  fill(0, 0, 0);
  // Information on alarm status, countdown and entered password
  text("Zone State: " + zoneState, 680, 80);
  text("Alarm State: " + alarmState, 680, 100);
  text("Countdown: " + (countdown), 680, 120); 
  text("Password: " + passwordEntered, 680, 140); 
}
