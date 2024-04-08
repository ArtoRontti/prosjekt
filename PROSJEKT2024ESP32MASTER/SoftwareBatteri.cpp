// myFunctions.cpp

#include "SoftwareBatteri.h"
#include <Arduino.h> // Include Arduino core library for Serial communication
#include <Zumo32U4.h>

float UpdateBattery(float avgSpeed, float battery_level) // UPDATES AND WATCHES BATTERY LEVEL
{
  
  battery_level -= abs(( (0.5 / 100) * avgSpeed) * ((battery_level + 1) / 1000)); // battery_level is reduced linearly by both distance and average speed (simple function) (~10 rounds around the track before running out of battery)

  return battery_level;
}

// DISPLAY FUNCTIONS
void BatteryStatusDisplay(float battery_level, float account_balance)
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
  display.print("balance:");
  display.gotoXY(11, 5);
  display.print(account_balance);
  display.gotoXY(17, 5);
  display.print("kr");

  // display time
  display.gotoXY(11, 7);
  float time = CURRENT_TIME / 1000; // in seconds
  display.print(time);
  display.gotoXY(17, 7);
  display.print("sec");
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

float UpdateSpeed(float speed, int counter, float total_speed)
{
  float rotationsRight = encoders.getCountsAndResetRight() / (75.81 * 12); // CpR = 75.81 * 12 <=> C/CpR = R
  float rotationsLeft = encoders.getCountsAndResetLeft() / (75.81 * 12);
  float moving = (rotationsRight != 0 || rotationsLeft != 0);
  float distanceRight = rotationsRight * (3.5 * 3.14);                  // centimeters (3.5 cm)
  float distanceLeft = rotationsLeft * (3.5 * 3.14);                    // centimeters
  float distanceDifferential = (distanceRight + distanceLeft) / 2;      // in centimeters
  float distanceTraveled += (distanceRight + distanceLeft) / (2 * 100);       // in meters
  speed = (1000 * (distanceDifferential)) / (CURRENT_TIME - LAST_TIME); // dv/dt = (avg(dr,dl)) / dt (cm / s) *1000 since CURRENT_TIME is ms

  if (CURRENT_TIME - PREV_UPDATE >= 100)
  { // takes note of speed every 100 ms for 60 s
    if (counter == 0)
    {
      total_speed = 0; // restart average every 60 s
    }
    total_speed += speed;
    counter += 1;
    counter %= 120;
  } // AVERAGE EVERY 60 S, RESTARTS

  // moving average
  average_speed = total_speed / counter;
  
  return average_speed
}


