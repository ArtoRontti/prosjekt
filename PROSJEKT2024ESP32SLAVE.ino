#include <Wire.h>
#include <Zumo32U4.h>
#include <Arduino.h>

Zumo32U4OLED display;
Zumo32U4Motors motors;

bool sportMode = false;
bool EcoMode = false;
bool speedUpdated = false;
int speed = 300;
int regularSpeed = 300;
int sportSpeed = 400;
int EcoSpeed = 200;

void setup() {
  Serial.begin(115200);
  Serial.println("Setup initiated..");
  Wire.begin(0x1E);              // Zumo I2C address
  Wire.onReceive(sendData);      //sends data to master
  Wire.beginTransmission(0x6B);  // OLED I2C address, making it a slave to the Arduino
  Wire.onReceive(receiveEvent);
  display.init();  //initialize display
  display.gotoXY(0, 0);
  display.print("Welcome");
  delay(1000);
  display.clear();
  display.setLayout21x8();
}

void loop() {
  receiveEvent();

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
        speed = sportSpeed;
        speedUpdated = true;
        display.clear();
        display.print("SportMode chosen");
        break;
      case 'l':
        speed = EcoSpeed;
        speedUpdated = true;
        display.clear();
        display.print("EcoMode chosen");
        break;
      case 'n':
        speed = regularSpeed;
        speedUpdated = true;
        display.clear();
        display.print("Regular mode chosen");
        break;
      case 'w':  // Move forward
        motors.setSpeeds(speed, speed);
        break;
      case 's':  // Move backward
        motors.setSpeeds(-speed, -speed);
        break;
      case 'a':  // Turn left
        motors.setSpeeds(-speed, speed);
        break;
      case 'd':  // Turn right
        motors.setSpeeds(speed, -speed);
        break;
      case 'q':  //angle left
        motors.setSpeeds(speed / 2, speed);
        break;
      case 'e':  //angle right
        motors.setSpeeds(speed, speed / 2);
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
  speedArray[0] = speed >> 16;    // Most significant byte
  speedArray[1] = (speed >> 8) & 0xFF;  // Least significant byte
  speedArray[2] = speed & 0xFF;
  Wire.write(speedArray, 3);     // Send the byte array over I2C
  Wire.endTransmission();    //complete the transmission
}
