#include <WiFi.h>
#include <esp_now.h>
#include <WebServer.h>
#include <Wire.h>
#include <Arduino.h>
#include "UbidotsEsp32Mqtt.h"
#include <SparkFun_APDS9960.h>

#define I2C_SDA 13  //bus
#define I2C_SCL 12  //bus

//WiFi
const char* ssid = "NTNU-IOT";  //wifi name
const char* password = "";      //Wifi password
WebServer server(80);           // basic server

//ubidots
const char* UBIDOTS_TOKEN = "BBUS-AcTJ6ccXQVpNyEBYHJ1dRor0GteJmb";
const char* DEVICE_LABEL = "zumoesp";
const char* VARIABLE_LABELa = "akselerasjon";
const char* VARIABLE_LABELd = "discount";
const char* VARIABLE_LABELs = "strompris";
const char* VARIABLE_LABELr = "redZone";
const char* VARIABLE_LABELg = "greenZone";
const char* VARIABLE_LABELb = "blueZone";
Ubidots ubidots(UBIDOTS_TOKEN);

//RGBsensor
SparkFun_APDS9960 apds = SparkFun_APDS9960();
int redZone, greenZone, blueZone = 0;
unsigned long previousMillisRGBsensor = 0;
uint16_t ambient_light, red_light, green_light, blue_light = 0;

//discount
float discount;

//clock
float hour = 0.0;

//acceleration
float acceleration = 0;

//I2C
float newBattery = 0;
float newAcceleration = 0;
float newDiscount = 0;
float receivedBattery;
float receivedDiscount;

struct sendInfo {
  float s;
  float d;
  float p;
};

sendInfo info;

String generateBatteryHTML(float receivedBattery) {
  String content;
  content += "<div style='grid-column: 15; grid-row: 26; background-color: transparent; width: 150px; border: 1px solid transparent; border-radius: 5px;'>";  // battery bar
  content += "<div style='color: yellow; grid-column: 15; grid-row: 28;'><h3>" + String(receivedBattery) + "%</h3>";                                          // place text and make it yellow for better visibility

  //makes the battery indicator change color for dramatic effect
  if (receivedBattery > 60) {  // battery indicator is green when its over 60%
    content += "<div style='height: 15px; width: " + String(receivedBattery) + "%; background-color: green; border-radius: 5px;'></div>";
  }
  if (receivedBattery <= 60 && receivedBattery > 10) {  //yellow below 60%
    content += "<div style='height: 15px; width: " + String(receivedBattery) + "%; background-color: yellow; border-radius: 5px;'></div>";

  } else if (receivedBattery <= 10) {  //red below 10%
    content += "<div style='height: 15px; width: " + String(receivedBattery) + "%; background-color: red; border-radius: 5px;'></div>";
  }
  content += "</div>";
  return content;
}

//////////////esp now///////////////////


uint8_t broadcastAddress[] = { 0x44, 0x17, 0x93, 0x5e, 0x45, 0x0c };  // addresse til joshua sin
//uint8_t broadcastAddress[] = { 0x44, 0x17, 0x93, 0x5e, 0x48, 0x34 };  // addresse til eivind sin

// Variable to store if sending data was successful
String success;

//same on both zumo's
typedef struct struct_message {
  char msg1[100];
  char msg2[100];
} struct_message;

struct_message incomingData;
struct_message outgoingData;
bool car2car = false;
bool FromCar2Car = false;
const unsigned long chargeInterval = 5000;

char incomingMsg1[100];  //100 byte message
char incomingMsg2[100];

void OnDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status == ESP_NOW_SEND_SUCCESS) {  // Fixed: corrected condition
    success = "Delivery Success :)";
  } else {
    success = "Delivery Fail :(";
  }
}

// Callback when data is received
void OnDataRecv(const uint8_t* mac, const uint8_t* incomingData, int len) {
  // Cast incomingData to the struct_message type
  struct_message* receivedData = (struct_message*)incomingData;

  strcpy(incomingMsg1, receivedData->msg1);
  strcpy(incomingMsg2, receivedData->msg2);

  Serial.print("Bytes received: ");
  Serial.println(len);
  // Print the received messages
  Serial.print("Message 1: ");
  Serial.println(receivedData->msg1);
  Serial.print("Message 2: ");
  Serial.println(receivedData->msg2);
}


unsigned long espNowMillis;

esp_now_peer_info_t peerInfo;
// esp now FINITO//////////////////

void setup() {
  //esp now
  WiFi.mode(WIFI_STA);
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Transmitted packet
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);


  //get best performance of esp
  WiFi.setSleep(false);
  //I2C
  Wire.begin();
  Serial.begin(115200);

  //ubidots
  ubidots.connectToWifi(ssid, password);
  ubidots.setCallback(callback);
  ubidots.setup();
  ubidots.reconnect();
  ubidots.subscribeLastValue(DEVICE_LABEL, VARIABLE_LABELa);
  ubidots.subscribeLastValue(DEVICE_LABEL, VARIABLE_LABELd);
  ubidots.subscribeLastValue(DEVICE_LABEL, VARIABLE_LABELs);
  ubidots.subscribeLastValue(DEVICE_LABEL, VARIABLE_LABELr);
  ubidots.subscribeLastValue(DEVICE_LABEL, VARIABLE_LABELg);
  ubidots.subscribeLastValue(DEVICE_LABEL, VARIABLE_LABELb);


  // Connect to Wi-Fi
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());


  //self made website to control zumo
  server.on("/", HTTP_GET, []() {
    String content = "<html><head><title>Arduino Keyboard</title>";
    content += "<meta http-equiv='refresh' content='1'>";  // refresh website every second
    content += "<style>";
    content += "body{";
    content += "background-image: url('https://img.freepik.com/premium-photo/detailed-look-inside-futuristic-racing-formula-car-s-cockpit-highlighting-driver-s-focus_875722-4628.jpg');";  //background image
    content += "background-repeat: no-repeat;";
    content += "background-size: cover;";  //making sure it covers the whole screen
    content += "}";
    content += ".keyboard {";
    content += "  display: grid;";
    content += "  grid-template-columns: repeat(22, 60px);";  // 22 columns/rows horisontally
    //content += "  grid-template-rows: repeat(10, 60px);"; // 10 rows vertically
    content += "  grid-gap: 15px;";  // spacing between keys
    content += "}";
    content += ".key {";
    content += "  width: 70px;";
    content += "  height: 60px;";
    content += "  border: 2px solid yellow;";  // for better visibility because of background
    content += "  text-align: center;";
    content += "  line-height: 60px;";
    content += "  font-size: 20px;";
    content += "  color: yellow;";  //for better visibility because of background
    content += "}";
    content += ".pressed {";                  // New CSS class for pressed keys
    content += "  background-color: green;";  // Set background color to green to indicate that a key is pressed
    content += "}";
    content += "h1, h2 {";          // CSS rule for headlines
    content += "  color: yellow;";  // Set color to orange for better visibility through background
    content += "}";
    content += "</style>";
    content += "</head><body>";
    content += "<div class='keyboard'>";  // ad key id for easier manipulation
    // driving
    content += "<div id='keyD' class='key' style='grid-column: 12; grid-row: 26;'>D</div>";  // move key to make website look cool
    content += "<div id='keyS' class='key' style='grid-column: 11; grid-row: 26;'>S</div>";  // move key to make website look cool
    content += "<div id='keyA' class='key' style='grid-column: 10; grid-row: 26;'>A</div>";  // move key to make website look cool
    content += "<div id='keyW' class='key' style='grid-column: 11; grid-row: 24;'>W</div>";  // move key to make website look cool
    //acceleration modes
    content += "<div id='keyH' class='key' style ='grid-column: 6; grid-row: 22;'>Sport</div>";    // H for sport mode/sport acceleration
    content += "<div id='keyL' class='key' style ='grid-column: 6; grid-row: 24;'>Eco</div>";      // L for Eco mode/eco acceleration
    content += "<div id='keyN' class='key' style ='grid-column: 6; grid-row: 26;'>regular</div>";  // N button for regular acceleration/ regular mode
    //charging modes
    content += "<div id='key1' class='key' style ='grid-column: 15; grid-row: 22;'>Charge</div>";   //1 for regular charge
    content += "<div id='key2' class='key' style ='grid-column: 15; grid-row: 24;'>FromCar</div>";  //2 for car to car charge
    content += "<div id='key3' class='key' style ='grid-column: 17; grid-row: 26;'>ToCar</div>";  //3 for car to car charge
    //update battery level on website
    content += generateBatteryHTML(receivedBattery);  // to be able to update and show realtime battery level
    content += "</div>";
    content += "</body></html>";

    content += "<script>";
    content += "let debounceTimer;";
    content += "window.addEventListener('keydown', function(event) {";
    content += "    if (debounceTimer) {";
    content += "        clearTimeout(debounceTimer);";
    content += "    }";
    content += "    var key = event.key;";
    content += "    var xhr = new XMLHttpRequest();";
    content += "    xhr.open('GET', '/key?key=' + key , true);";
    content += "    xhr.send();";
    content += "    document.getElementById('key' + key.toUpperCase()).classList.add('pressed');";  // Add 'pressed' class to corresponding key
    content += "    debounceTimer = setTimeout(function() {";
    content += "        debounceTimer = null;";
    content += "    }, 100);";  // Adjust debounce duration as needed
    content += "});";
    //function to stop a function when a key is released
    content += "window.addEventListener('keyup', function(event) {";
    content += "    var key = event.key;";
    content += "    var xhr = new XMLHttpRequest();";
    content += "    xhr.open('GET', '/key?key=' + key, true);";  // Include the released key in the request URL
    content += "    xhr.send();";
    content += "    var xhr = new XMLHttpRequest();";
    content += "    xhr.open('GET', '/key?key=x', true);";  // Send the 'x' key after releasing any key. this is used to stop the zumo when a key is released
    content += "    xhr.send();";
    content += "    document.getElementById('key' + key.toUpperCase()).classList.remove('pressed');";  // Remove 'pressed' class from corresponding key
    content += "});";
    content += "</script>";
    server.send(200, "text/html", content);
  });



  server.on("/key", HTTP_GET, []() {
    if (server.hasArg("key")) {
      String key = server.arg("key");
      Serial.println("Key pressed: " + key);
      server.send(200, "text/plain", "Key pressed: " + key);  //sends key from website to esp
      Wire.beginTransmission(8);                              // sets arduino as slave.
      Wire.write(key[0]);                                     //sends one byte. the first of the key that gets pressed
      Wire.endTransmission();

      if (key == "1") {
        receivedDiscount = 0.00;  // resets charging discount after charging to promote economic driving
      }

      else if (key == "2") {
        car2car = true;
      } else if (key == "3") {
        FromCar2Car = true;
      }
    } else {
      server.send(400, "text/plain", "Bad request");
    }
  });

  server.begin();
  Serial.println("HTTP server started");
  rgbSensorSetup();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]");

  String payloadStr = "";
  for (int i = 0; i < length; i++) {
    payloadStr += (char)payload[i];
  }
  Serial.print(payloadStr);
}

void loop() {
  server.handleClient();
  if (!ubidots.connected()) {
    ubidots.reconnect();
  }
  ubidots.loop();

  clockTime();
  receiveData();
  findAacceleration();
  zoneCheck();
  carToCarReceive();
  CarToCarSend();
}


void receiveData() {
  Wire.requestFrom(8, sizeof(info));  // Request data from slave address 8

  Wire.readBytes((uint8_t*)&info, sizeof(sendInfo));  // reads the received info

  //access received data
  receivedDiscount = info.d;
  receivedBattery = info.p;

  if (acceleration != newAcceleration) {
    ubidots.add(VARIABLE_LABELa, acceleration);
    ubidots.publish();
    newAcceleration = acceleration;
  }

  if (receivedDiscount != newDiscount) {
    //Serial.print("Discount: ");
    //Serial.println(receivedDiscount);
    float yourElPrice = elPrice(hour) - (elPrice(hour) * receivedDiscount);
    ubidots.add(VARIABLE_LABELd, yourElPrice);  //deducted el-price
    ubidots.add(VARIABLE_LABELs, elPrice(hour));      //general el-price
    ubidots.publish();
    newDiscount = receivedDiscount;
  }

  if (receivedBattery != newBattery) {
    newBattery = receivedBattery;
  }
}
float elPrice(float hour) {//function for telling power price at differnt time of day
  float price = 0.0;

  if (hour >= 0 && hour < 6) {
    // Low price at night
    price = 25;
  } else if (hour >= 6 && hour < 18) {
    // high price in the day
    price = 50;
  } else {
    // decent price in the evening
    price = 30;
  }
  return price;
}

void clockTime(){//clock function for an alternativ time.
  //millis variables
  static unsigned long pastMillis = 0;
  const unsigned long timeIntervalMillis = 10000; //10 seconds reprsent 1 "hour".
    if (millis() - pastMillis >= timeIntervalMillis) {
    pastMillis = millis();
    hour += 1; // hour variable increas for every "hour"
    if (hour > 24) {
      hour = 0; // new day
    }
  }
}

void carToCarReceive() {
  static unsigned long lastMillisR = 0;
  if (car2car) {
    FromCar2Car = false;
  }

  if ((strcmp(incomingMsg1, "car2car charge available") == 0)) {
    if (car2car) {
      strcpy(outgoingData.msg1, "accepted car2car charge");
      //Serial.println("hei");
      espNowSend();
      if (millis() - lastMillisR >= chargeInterval) {  //5percent battery every 5 seconds
        lastMillisR = millis();
        if (receivedBattery >= 94.0) {
          strcpy(outgoingData.msg1, "car2car charge stopped");
          espNowSend();
          car2car = false;  //stops charging when battery is full, or with stop command
        }
        else if ((strcmp(incomingMsg1, "car2carCharge stopped") == 0)) {
          car2car = false;
        }
        Wire.beginTransmission(8);
        Wire.write('y');
        Wire.endTransmission();
      }
    }
  }
}

void CarToCarSend() {
  static unsigned long lastMillisS = 0;

  if (FromCar2Car) {
    car2car = false;
  }

  if ((receivedBattery >= 50.0) && FromCar2Car) {
    strcpy(outgoingData.msg1, "car2car charge available");
    espNowSend();
    if ((strcmp(incomingMsg1, "accepted car2car charge") == 0)) {
      Serial.println("hei");
      if (millis() - lastMillisS >= chargeInterval) {
        Serial.println(millis() - lastMillisS);
        lastMillisS = millis();
        Wire.beginTransmission(8);
        Wire.write('g');
        Wire.endTransmission();
        if ((receivedBattery <= 50.0) || (strcmp(incomingData.msg1, "car2car charge stopped")==0)) {
          FromCar2Car = false;
          strcpy(outgoingData.msg1, "car2car charge stopped");
          espNowSend();
        }
      }
    }
  }
}

void espNowSend(){//procedure for sending message with espNow
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t*)&outgoingData, sizeof(outgoingData));
      if (result == ESP_OK) {
        Serial.println("Sent with success");
      } else {
        Serial.println("Error sending the data");
      }
}

void rgbSensorSetup() {
  Serial.println();
  Serial.println(F("--------------------------------"));
  Serial.println(F("APDS-9960 - ColorSensor"));
  Serial.println(F("--------------------------------"));

  // Initialize APDS-9960 (configure I2C and initial values)
  if (apds.init()) {
    Serial.println(F("APDS-9960 initialization complete"));
  } else {
    Serial.println(F("Something went wrong during APDS-9960 init!"));
  }

  // Start running the APDS-9960 light sensor (no interrupts)
  if (apds.enableLightSensor(false)) {
    Serial.println(F("Light sensor is now running"));
  } else {
    Serial.println(F("Something went wrong during light sensor init!"));
  }

  // Wait for initialization and calibration to finish
  delay(500);
}

void zoneCheck() {
  //Millis variabels
  static unsigned long lastUbiZone = 0;
  const unsigned long ubiZoneInterval = 10000;
  const int intervalRGBsensor = 100;
  static char zone = 's';

  if (millis() - previousMillisRGBsensor >= intervalRGBsensor) {
    previousMillisRGBsensor = millis();
    // Read the light levels (ambient, red, green, blue)
    if (!apds.readAmbientLight(ambient_light) || !apds.readRedLight(red_light) || !apds.readGreenLight(green_light) || !apds.readBlueLight(blue_light)) {
      Serial.println("Error reading light values");
    }
    if (ambient_light > 5000) {//check wich light is strongest
      if (red_light > green_light + blue_light - 1000) {
        zone = 'r';
      } else if (green_light > red_light + blue_light - 2000) {
        zone = 'g';
      } else if (blue_light > red_light + green_light - 2000) {
        zone = 'b';
      }
      Serial.println("You are in Zone: " + String(zone));
    }
  }
  if (abs(acceleration) >= 2.25) {//If acceleration is to high, flagg the zone
    if (zone == 'r') {
      redZone += 1;
    } else if (zone == 'g') {
      greenZone += 1;
    } else if (zone == 'b') {
      blueZone += 1;
    }
    if(millis() - lastUbiZone >= ubiZoneInterval){//Send zones to ubidots every 10 sec
      lastUbiZone = millis();
      ubidots.add(VARIABLE_LABELr, redZone);
      ubidots.add(VARIABLE_LABELg, greenZone);
      ubidots.add(VARIABLE_LABELb, blueZone);
      ubidots.publish(DEVICE_LABEL);
    }
  }
}
void findAacceleration() {//Meassuer acceleration based on speed
  static float speedB, lastSpeedTime = 0;
  if (millis() - lastSpeedTime >= 10) {
    lastSpeedTime = millis();
    acceleration = info.s - speedB;
    speedB = info.s;
  }
}
