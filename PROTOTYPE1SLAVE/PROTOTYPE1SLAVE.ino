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
  
  acceleration();

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
  while (millis() - startMillis < 3000) // lock in rotation while calibrating for 3 seconds
  {
    motors.setSpeeds(-200, 200);
    lineSensors.calibrate();
  }
  motors.setSpeeds(0, 0);
  display.clear();
  display.setLayout21x8();
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