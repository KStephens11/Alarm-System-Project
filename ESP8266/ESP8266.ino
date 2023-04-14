// Information needed to connect the esp8266 to Blynk's servers
#define BLYNK_TEMPLATE_ID "TMPLkSS4gKa4"
#define BLYNK_TEMPLATE_NAME "espTemplate"
#define BLYNK_AUTH_TOKEN "BNJHKOiDBXxR88L70ikMH07huP5K5aER"

// Librarys used by the esp8266
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

// Wifi credentials
char ssid[] = "AndroidAP_6143";
char pass[] = "ConnectToThis123";

// State stores the virtual pin 0 state, which is the button on the blynk app
int state;
// Input is used to store any data recieved from the arduino
int input;

/*
BLYNK_WRITE is triggered whenever the the button is pressed.
State is being set to the value of the virtual pin using:
state = param.asInt();
*/
BLYNK_WRITE(V0){
  state = param.asInt();
}

void setup()
{
  // Start serial connection with arduino at baud 115200.
  Serial.begin(115200);
  // Starts connection to blynk server
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  /*
  Blynk.virtualWrite is used to write data to a virtual pin.
  At the setup Alarm disarmed and No Zones Active are written
  to their pins to make sure that they are displayed on the app
  when the esp starts.
  */
  Blynk.virtualWrite(V2, "Alarm Disarmed");
  Blynk.virtualWrite(V3, "No Zones Active");
  Blynk.virtualWrite(V4, 0);
}

void loop()
{
  // Runs the Blynk communication loop. This is used to check for new data and sending data.
  Blynk.run();

  //Check if data is being sent through serial
  while(Serial.available()>0) {
    // input is set to recieved data
    input = Serial.read();

    // Checking for numbers between 1 - 15. This is the countdown value.
    if (input < 30 & input > 10) {
      Blynk.virtualWrite(V4, input - 11);
    }

    /*
    Switch statement to check the value being sent to the esp in the variable input.
    If the value is 254 that means the alarm is armed, if its 253 then it's disarmed and so on.
    This will send information to blynks servers to be accessed on the app.

    Blynk.logEvent("alarm_tripped", String("Zone # Activated")); is used to send a notification to the app that the alarm has been tripped.
    alarm_tripped is the event code and after that is a custom description that will be semt to the phone when activated.

    */
    switch(input) {
      case 254:
        Blynk.virtualWrite(V2, "Alarm Armed");
        break;
      case 253:
        Blynk.virtualWrite(V2, "Alarm Disarmed");
        break;
      case 252:
        Blynk.virtualWrite(V3, "No Zones Active");
        break;
      case 251:
        Blynk.virtualWrite(V3, "Entry/Exit Active");
        Blynk.logEvent("alarm_tripped", String("Entry/Exit Activated"));
        break;
      case 250:
        Blynk.virtualWrite(V3, "Zone 0 Activated");
        Blynk.logEvent("alarm_tripped", String("Zone 0 Activated"));
        break;
      case 249:
        Blynk.virtualWrite(V3, "Zone 1 Activated");
        Blynk.logEvent("alarm_tripped", String("Zone 1 Activated"));
        break;
      case 246:
        Blynk.virtualWrite(V3, "Door 0 Activated");
        Blynk.logEvent("alarm_tripped", String("Door 0 Activated"));
        break;
      case 245:
        Blynk.virtualWrite(V3, "Door 1 Activated");
        Blynk.logEvent("alarm_tripped", String("Door 1 Activated"));
        break;

    }

  }

  /* 
  This is used to arm and disarm the alarm through the app.
  This checks the value of state and if it is high, it will write
  255 to the arduino. When the arduino recieves it it will arm/disarm the alarm.
  Serial.flush() is used to make the esp wait until the data has been sent before moving on in the program.
  */
  if (state) {
    Serial.write(255);
    Serial.flush();
    delay(100);
    
  }
  // Used to send the input variable to the app, doesn't have any use.
  Blynk.virtualWrite(V1, input);

}
