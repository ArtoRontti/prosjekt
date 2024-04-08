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
      case 'f'
        FollowLine(0);
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


// Joshua is here (testing git pushing on a branch and pulling)
// linjefølging
void FollowLine(int modus) // Two modes: standard and PID-regulation
{
  int lineSensorValue = lineSensors.readLine(lineSensorValues);
  int16_t position = lineSensorValue;

  if (modus == 0) // PID-REGULATION
  {
    // Serial.println("case 0");
    int16_t error = position - 2000;

    // Get motor speed difference using proportional and derivative
    // PID terms (the integral term is generally not very useful
    // for line following).  Here we are using a proportional
    // constant of 1/4 and a derivative constant of 6, which should
    // work decently for many Zumo motor choices.  You probably
    // want to use trial and error to tune these constants for your
    // particular Zumo and line course.
    int16_t speedDifference = (error / 4) + 6 * (error - lastError); // Kp = 1/4, error is too big to be used as SpeedDifference itself

    lastError = error;

    // Get individual motor speeds.  The sign of speedDifference
    // determines if the robot turns left or right.
    int16_t leftSpeed = 400 + speedDifference; // speedDifference > 0 when position > 2000 (position of line is to the right), turns left
    int16_t rightSpeed = 400 - speedDifference;

    // Constrain our motor speeds to be between 0 and maxSpeed.
    // One motor will always be turning at maxSpeed, and the other
    // will be at maxSpeed-|speedDifference| if that is positive,
    // else it will be stationary.  For some applications, you
    // might want to allow the motor speed to go negative so that
    // it can spin in reverse.

    leftSpeed = constrain(leftSpeed, 0, 400);
    rightSpeed = constrain(rightSpeed, 0, 400);

    motors.setSpeeds(leftSpeed, rightSpeed);
  }
  else if (modus == 1)
  {
    // Serial.println("inside case 1");
    // Serial.println(lineSensorValue);

    if (lineSensorValue < 1800)
    {
      motors.setSpeeds(-100, 150);
    }
    else if (lineSensorValue > 2200)
    {
      motors.setSpeeds(150, -100);
    }
    else
    {
      motors.setSpeeds(200, 200);
    }
  }
}
