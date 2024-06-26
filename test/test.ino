#include <Wire.h>
#include <Zumo32U4.h>
#include <Arduino.h>

Zumo32U4OLED display;
Zumo32U4Motors motors;
Zumo32U4Encoders encoders;

bool sportMode = false;
bool EcoMode = false;
bool speedUpdated = false;
int speed = 50;
int currentSpeed;
int regularSpeed = 300;
int sportSpeed = 400;
int EcoSpeed = 200;

//acceleration
bool w = false;
bool s = false;
unsigned long accelerationStart;
int targetSpeed;
int accelerationVariable;

//float speed = 0;
//float total_speed = 0; //for moving average
//int counter = 0;

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
  display.setLayout21x8();  // 21 * 8 characters (21 row - X, 8 column - Y)
  display.clear();
  //lineSensors.initFiveSensors();
}

void loop() {
  receiveEvent();
  acceleration();
  //speed = UpdateSpeed(speed, counter, total_speed);
  //battery_level = UpdateBattery(battery_level, speed);

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
        targetSpeed = 400;
        accelerationVariable = 3;
        display.clear();
        display.print("SportMode chosen");
        break;
      case 'l':
        targetSpeed = 200;
        accelerationVariable = 1;
        display.clear();
        display.print("EcoMode chosen");
        break;
      case 'n':
        targetSpeed = 300;
        accelerationVariable = 2;
        display.clear();
        display.print("Regular mode chosen");
        break;
      case 'w':  // Move forward
        motors.setSpeeds(speed, speed);
        w = true;
        break;
      case 's':  // Move backward
        motors.setSpeeds(speed, speed);
        s = true;
        break;
      case 'a':  // Turn left
        motors.setSpeeds(-speed, speed);
        break;
      case 'd':  // Turn right
        motors.setSpeeds(speed, -speed);
        break;
      /*case 'f'
        FollowLine(); // ikke integrert enda
        break; */
      case 'x':
        w = false;  //resets speed for acceleration
        s = false;  //resets speed for acceleration
        break;
      default:  // Stop
        motors.setSpeeds(0, 0);
        break;
    }
  }
}

void sendData() {
  byte speedArray[3];                   // has to be an array on I2C
  speedArray[0] = speed >> 16;          // Most significant byte
  speedArray[1] = (speed >> 8) & 0xFF;  // Least significant byte
  speedArray[2] = speed & 0xFF;
  Wire.write(speedArray, 3);  // Send the byte array over I2C
  Wire.endTransmission();     //complete the transmission
}


// Joshua is here (testing git pushing on a branch and pulling)
void FollowLine() {
}

void acceleration() {
  //forward
  if (w) {
    if (millis() - accelerationStart >= 10) {
      if (speed < targetSpeed) {
        speed += accelerationVariable;
      }
      if (speed >= targetSpeed) {
        speed = targetSpeed;
      }
      accelerationStart = millis();
    }
  } 
  if (s) {
    if (millis() - accelerationStart >= 10) {
      if (speed > 0) {
        speed -= accelerationVariable;
      }
      if (speed <= 0) {
        speed = 0;
      }
      accelerationStart = millis();
    }
  }
}
