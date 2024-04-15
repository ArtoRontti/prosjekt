#include <Wire.h>
#include <Zumo32U4.h>
#include <Arduino.h>

//hei
Zumo32U4OLED display;
Zumo32U4Motors motors;
Zumo32U4Encoders encoders;
Zumo32U4LineSensors lineSensors;

bool sportMode = false;
bool EcoMode = false;
bool speedUpdated = false;
float speed = 300;
float driveSpeed = 0;
float regularSpeed = 300;
float sportSpeed = 400;
float EcoSpeed = 200;

float LAST_TIME = 0; // brukt for a oppdatere speed hver 100 ms
float total_speed = 0; //for moving average
int counter = 0;

float battery_level = 100;


void setup() {
  Serial.begin(115200);
  Serial.println("Setup initiated..");
  Wire.begin(0x1E);              // Zumo I2C address
  Wire.onReceive(sendData);      //sends data to master
  Wire.beginTransmission(0x6B);  // OLED I2C address, making it a slave to the Arduino
  Wire.onReceive(receiveEvent);

  encoders.init();
  display.init();
  display.setLayout21x8(); // 21 * 8 characters (21 row - X, 8 column - Y)
  display.clear();
  lineSensors.initFiveSensors();
}

void loop() {
  receiveEvent(); 
  
  if (millis() - LAST_TIME >= 100) { // moving average update speed every 100 ms (20 data points) 
    speed = UpdateSpeed(speed, counter, total_speed, millis(), LAST_TIME); // returnerer en float
    
    LAST_TIME = millis();
  }
  battery_level = UpdateBattery(speed, battery_level);

  

  SpeedDisplay(speed); // display speed on LCD(can add control of display switching)
  BatteryStatusDisplay(battery_level, 100, millis());

  if (speedUpdated) {  // only send speed data to master if speed gets changed
    sendData();
    speedUpdated = false;
  } else {
  }
  

}

void receiveEvent() {
  while (Wire.available()) {
    char mode = Wire.read();
    Serial.println(mode);

    switch (mode) {  // switch case that takes commands from the master(esp32)
      case 'h':
        driveSpeed = sportSpeed;
        speedUpdated = true;
        //display.clear();
        //display.print("SportMode chosen");
        break;
      case 'l':
        driveSpeed = EcoSpeed;
        speedUpdated = true;
        //display.clear();
        //display.print("EcoMode chosen");
        break;
      case 'n':
        driveSpeed = regularSpeed;
        speedUpdated = true;
        //display.clear();
        //display.print("Regular mode chosen");
        break;
      case 'w':  // Move forward
        motors.setSpeeds(driveSpeed, driveSpeed);
        break;
      case 's':  // Move backward
        motors.setSpeeds(-driveSpeed, -driveSpeed);
        break;
      case 'a':  // Turn left
        motors.setSpeeds(-driveSpeed, driveSpeed);
        break;
      case 'd':  // Turn right
        motors.setSpeeds(driveSpeed, -driveSpeed);
        break;
      case 'q':  //angle left
        motors.setSpeeds(driveSpeed / 2, speed);
        break;
      case 'e':  //angle right
        motors.setSpeeds(speed, speed / 2);
        break;
      case 'f':
        FollowLine(); // ikke integrert enda
        break;
      case 'x':
        motors.setSpeeds(0, 0);
        break;
      default:  // Stop
        motors.setSpeeds(0, 0);
        break;
    }
  }
}

void sendData() {
  byte speedArray[3];            // has to be an array on I2C
  //speedArray[0] = speed >> 16;    // Most significant byte
  //speedArray[1] = (speed >> 8) & 0xFF;  // Least significant byte
  //speedArray[2] = speed & 0xFF;
  Wire.write(speedArray, 3);     // Send the byte array over I2C
  Wire.endTransmission();    //complete the transmission
}


// Joshua is here (testing git pushing on a branch and pulling)
void FollowLine(){ // trenger for testing

}

void SpeedDisplay(float average_speed)
{

    // display average
    display.gotoXY(0, 5);
    display.print("average:");
    display.gotoXY(11, 5);
    display.print(average_speed);
    display.gotoXY(17, 5);
    display.print("cm/s");
}

float UpdateSpeed(float speed, int counter, float total_speed, float CURRENT_TIME, float LAST_TIME)
{
  float rotationsRight = encoders.getCountsAndResetRight() / (75.81 * 12); // CpR = 75.81 * 12 <=> C/CpR = R
  float rotationsLeft = encoders.getCountsAndResetLeft() / (75.81 * 12);
  float moving = (rotationsRight != 0 || rotationsLeft != 0);
  float distanceRight = rotationsRight * (3.5 * 3.14);                  // centimeters (3.5 cm)
  float distanceLeft = rotationsLeft * (3.5 * 3.14);                    // centimeters
  float distanceDifferential = (distanceRight + distanceLeft) / 2;      // in centimeters
  // float distanceTraveled += (distanceRight + distanceLeft) / (2 * 100);       // in meters
  speed = (1000 * (distanceDifferential)) / (CURRENT_TIME - LAST_TIME); // dv/dt = (avg(dr,dl)) / dt (cm / s) *1000 since CURRENT_TIME is ms

  if (counter == 0)
  {
    total_speed = 0; // restart average every 2 s
  }
  total_speed += speed;
  counter += 1;
  counter %= 20; // 0.1*20 = 2 s, moving average with 20 data points, restart after 20

  // moving average
  float average_speed = total_speed / counter;
  
  return average_speed;
}

float UpdateBattery(float avgSpeed, float battery_level) // UPDATES AND WATCHES BATTERY LEVEL
{
  
  battery_level -= abs(( (0.5 / 100) * avgSpeed) * ((battery_level + 1) / 1000)); // battery_level is reduced linearly by both distance and average speed (simple function) (~10 rounds around the track before running out of battery)

  return battery_level;
}

void BatteryStatusDisplay(float battery_level, float account_balance, float CURRENT_TIME)
{

  display.gotoXY(0, 0);
  display.print("SOFTWARE BATTERY");

  // display battery_level
  display.gotoXY(0, 2);
  display.print("level:");
  display.gotoXY(11, 2);
  display.print(battery_level);
  display.gotoXY(17, 2);
  display.print("\%");

  // display account_balance
  display.gotoXY(0, 5);
 // display.print("balance:");
  display.gotoXY(11, 5);
  //display.print(account_balance);
  display.gotoXY(17, 5);
  //display.print("kr");

  // display time
  display.gotoXY(11, 7);
  float time = CURRENT_TIME / 1000; // in seconds
  display.print(time);
  display.gotoXY(17, 7);
  display.print("sec");
}

