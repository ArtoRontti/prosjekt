/*////////
Denne koden er gjenbrukt fra forrige semester, med et par justeringer for når den skal stoppe.
*//////////

#include <Zumo32U4.h>

Zumo32U4OLED display;
Zumo32U4Motors motors;
Zumo32U4Encoders encoders;
Zumo32U4LineSensors lineSensors;

unsigned int lineSensorValues[5];
int16_t lastError = 0;

void setup() {
  Serial.begin(115200);
  lineSensors.initFiveSensors();
  calibrate();

}

void loop() {
  FollowLine();

}

void calibrate() {
  static unsigned long startMillis = millis();
  display.clear();
  display.setLayout11x4();
  display.gotoXY(1, 1);
  display.print("calibrate");
  while (millis() - startMillis <= 3000)  // lock in rotation while calibrating for 3 seconds
  {
    motors.setSpeeds(-200, 200);
    lineSensors.calibrate();
  }
  motors.setSpeeds(0, 0);
  display.clear();
  display.setLayout21x8();
}

void FollowLine()  // Two modes: standard and PID-regulation
{
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
  int16_t speedDifference = (error / 4) + 6 * (error - lastError);  // Kp = 1/4, error is too big to be used as SpeedDifference itself

  lastError = error;

  // Get individual motor speeds.  The sign of speedDifference
  // determines if the robot turns left or right.
  int16_t leftSpeed = 200 + speedDifference;  // speedDifference > 0 when position > 2000 (position of line is to the right), turns left
  int16_t rightSpeed = 200 - speedDifference;

  // Constrain our motor speeds to be between 0 and maxSpeed.
  // One motor will always be turning at maxSpeed, and the other
  // will be at maxSpeed-|speedDifference| if that is positive,
  // else it will be stationary.  For some applications, you
  // might want to allow the motor speed to go negative so that
  // it can spin in reverse.

  leftSpeed = constrain(leftSpeed, 0, 200);
  rightSpeed = constrain(rightSpeed, 0, 200);

  if(lineSensorValues[0] >= 1000 && lineSensorValues[4] >= 1000){
    motors.setSpeeds(0,0);
    delay(2000);
    motors.setSpeeds(50,50);
    delay(70);
  }

  motors.setSpeeds(leftSpeed, rightSpeed);
}
