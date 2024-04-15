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

//line follower
unsigned int lineSensorValues[5];
int16_t lastError = 0;

//acceleration
bool w = false;
bool s = false;
unsigned long accelerationStart;
int targetSpeed;
int accelerationVariable;
static  int previousSpeed = 0;  // Initialize previous speed

//kWh/s
float powerRemaining = 0;
float lastPower = 0;
float powerUsed = 0;

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
    powerUsed = (lastPower - powerRemaining) / 0.1; // kWh/s (eller effekt/forbruk)
    lastPower = powerRemaining;
    LAST_TIME = millis();
  }
  battery_level = UpdateBattery(speed, battery_level);
  powerRemaining = (battery_level / 100) * 82; // 82 kWh i en elbils batteri (fra Tesla)

  acceleration();

  

  SpeedDisplay(speed); // display speed on LCD(can add control of display switching)
  BatteryStatusDisplay(battery_level, 100, millis());

  if (speedUpdated) {  // only send speed data to master if speed gets changed
    sendData();
    speedUpdated = false;
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
        //display.clear();
        //display.print("SportMode chosen");
        break;
      case 'l':
        targetSpeed = 200;
        accelerationVariable = 1;
        //display.clear();
        //display.print("EcoMode chosen");
        break;
      case 'n':
        targetSpeed = 300;
        accelerationVariable = 2;
        //display.clear();
        //display.print("Regular mode chosen");
        break;
      case 'w':  // Move forward
        motors.setSpeeds(driveSpeed, driveSpeed);
        w = true;
        break;
      case 's':  // Move backward
        motors.setSpeeds(-driveSpeed, -driveSpeed);
        s = true;
        break;
      case 'a':  // Turn left
        motors.setSpeeds(-driveSpeed, driveSpeed);
        break;
      case 'd':  // Turn right
        motors.setSpeeds(driveSpeed, -driveSpeed);
        break;
      case 'f':
        FollowLine(); // ikke integrert enda
        break;
      case 'x':
        w = false;
        s = false;
        motors.setSpeeds(0, 0);
        break;
      default:  // Stop
        motors.setSpeeds(0, 0);
        break;
    }
  }
}
void calibrate()
{
  unsigned long startMillis = millis();
  display.clear();
  display.setLayout11x4();
  display.gotoXY(1, 1);
  display.print("calibrate");
  while (millis() - startMillis < 5000) // lock in rotation while calibrating for 5 seconds
  {
    motors.setSpeeds(-200, 200);
    lineSensors.calibrate();
  }
  motors.setSpeeds(0, 0);
  display.clear();
  display.setLayout21x8();
}

// Joshua is here (testing git pushing on a branch and pulling)
void FollowLine() // Two modes: standard and PID-regulation
{
  calibrate();
  int lineSensorValue = lineSensors.readLine(lineSensorValues);
  int16_t position = lineSensorValue;

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

  //display forbruk/consumption/con
  display.gotoXY(0, 3);
  display.print("Forbruk:");
  display.gotoXY(11, 3);
  display.print(powerUsed);
  display.gotoXY(16, 3);
  display.print("kWh/s");

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


void acceleration() {
  //forward
  if (w) {
    if (millis() - accelerationStart >= 10) {
      if (driveSpeed < targetSpeed) {
        driveSpeed += accelerationVariable;
      }
      if (driveSpeed >= targetSpeed) {
        driveSpeed = targetSpeed;
      }
      accelerationStart = millis();
    }
  }
  if (s) {
    if (millis() - accelerationStart >= 10) {
      if (driveSpeed > 0) {
        driveSpeed -= accelerationVariable;
      }
      if (driveSpeed <= 0) {
        driveSpeed = 0;
      }
      accelerationStart = millis();
    }
  }
}

void sendData() {

  if (speed != previousSpeed) {    // Check if speed has changed
    int speedSend = speed;
    byte* byteSpeed = (byte*)&speedSend;          // Convert int to byte array
    Wire.write(byteSpeed, sizeof(int));     // Write byte array to I2C bus
    Wire.endTransmission();
    Serial.println(speed);
    previousSpeed = speed; // Update previous speed
  }
}