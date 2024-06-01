#include "UbidotsEsp32Mqtt.h"

//sensorpins
const int pinCount = 3;
const int sensorPins[pinCount] = {25, 33, 32};

//sensor data
int sensorData[pinCount];

//Parking data
bool parkingSpace[pinCount] = {1, 1, 1};
//parking millis
const unsigned long checkFrequency = 1000;
unsigned long timerParking = millis();

//Ubidots
const char *UBIDOTS_TOKEN = "BBUS-AcTJ6ccXQVpNyEBYHJ1dRor0GteJmb";  // Put here your Ubidots TOKEN
const char *WIFI_SSID = "NTNU-IOT";      // Put here your Wi-Fi SSID
const char *WIFI_PASS = "";      // Put here your Wi-Fi password
const char *DEVICE_LABEL = "esp32-parking";   // Put here your Device label to which data  will be published
const char *VARIABLE_LABEL1 = "parking1"; // Put here your Variable label to which data  will be published
const char *VARIABLE_LABEL2 = "parking2";
const char *VARIABLE_LABEL3 = "parking3";
Ubidots ubidots(UBIDOTS_TOKEN);

//ubidots millis
const unsigned long PUBLISH_FREQUENCY = 5000; // Update rate in milliseconds
unsigned long timerUbi = millis();

void setup() {
  Serial.begin(115200);
  //Setting mode for sensorpins
  for (int i = 0; i < pinCount; i++) {
    pinMode(sensorPins[i], INPUT);
  }
  ubiSetup();
}

void loop() {
  if(millis() - timerParking > checkFrequency){
    checkParking();
    timerParking = millis();
  }
  ubiLoop();
}

void checkParking(){
  //threshold parkingspace
  const int threshold = 1000;
  for(int i = 0; i < pinCount; i++){
    //reading parking sensors
    sensorData[i] = analogRead(sensorPins[i]);
    //Check if space is occupied
    if(sensorData[i] > threshold){
      parkingSpace[i] = 0;
    }
    else{
      parkingSpace[i] = 1;
    }
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void ubiSetup(){
  // ubidots.setDebug(true);  // uncomment this to make debug messages available
  ubidots.connectToWifi(WIFI_SSID, WIFI_PASS);
  ubidots.setCallback(callback);
  ubidots.setup();
  ubidots.reconnect();
}

void ubiLoop(){
  static bool change = false;
  static bool prevParkingSpace[pinCount] = {1, 1, 1};
  if (!ubidots.connected())
  {
    ubidots.reconnect();
  }
  if (millis() - timerUbi > PUBLISH_FREQUENCY) // send to ubidots frequency
  {
    for (int i = 0; i < pinCount; i++) {
      //checking if there is a change i parking data
      if(parkingSpace[i] != prevParkingSpace[i]){
        change = true;
        prevParkingSpace[i] = parkingSpace[i];
      }
    }
    if(change){//Sending new parking data
      ubidots.add(VARIABLE_LABEL1, parkingSpace[0]);
      ubidots.add(VARIABLE_LABEL2, parkingSpace[1]);
      ubidots.add(VARIABLE_LABEL3, parkingSpace[2]);
      Serial.println(parkingSpace[0]);
      Serial.println(parkingSpace[1]);
      Serial.println(parkingSpace[2]);
      Serial.println("Sendt");
      ubidots.publish(DEVICE_LABEL);
      change = false;
    }
  }
  ubidots.loop();
}
