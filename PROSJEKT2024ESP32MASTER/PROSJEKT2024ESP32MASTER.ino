#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Arduino.h>
#include "UbidotsEsp32Mqtt.h"

#define I2C_SDA 13  //bus
#define I2C_SCL 12  //bus

//WiFi
const char* ssid = "NTNU-IOT";  //wifi name
const char* password = "";         //Wifi password
WebServer server(80);                      // basic server

//ubidots
const char* UBIDOTS_TOKEN = "BBUS-AcTJ6ccXQVpNyEBYHJ1dRor0GteJmb";
const char* DEVICE_LABEL = "esp32";
const char* VARIABLE_LABEL = "Akselerasjon";
const char* VARIABLE_LABELD = "discount";
const char* VARIABLE_LABELE = "strompris";
Ubidots ubidots(UBIDOTS_TOKEN);

//discount
float discount;
float yourElPrice;
float elPrice;

//I2C
float newBattery = 0;
float newSpeed = 0;
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
  content += "<div style='grid-column: 15; grid-row: 26; background-color: transparent; width: 150px; border: 1px solid transparent; border-radius: 5px;'>"; // battery bar
  content += "<div style='color: yellow; grid-column: 15; grid-row: 28;'><h3>" + String(receivedBattery) + "%</h3>"; // place text and make it yellow for better visibility

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

void setup() {

  WiFi.setSleep(false);
  //I2C
  Wire.begin();
  Serial.begin(115200);


  //ubidots
  ubidots.connectToWifi(ssid, password);
  ubidots.setCallback(callback);
  ubidots.setup();
  ubidots.reconnect();
  ubidots.subscribeLastValue(DEVICE_LABEL, VARIABLE_LABEL);
  ubidots.subscribeLastValue(DEVICE_LABEL, VARIABLE_LABELD);
  ubidots.subscribeLastValue(DEVICE_LABEL, VARIABLE_LABELE);


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
    content += "<style>";
    content += "body{";
    content += "background-image: url('https://img.freepik.com/premium-photo/detailed-look-inside-futuristic-racing-formula-car-s-cockpit-highlighting-driver-s-focus_875722-4628.jpg');";  //background image
    content += "background-repeat: no-repeat;";
    content += "background-size: cover;";                                                                                                                                                   //making sure it covers the whole screen
    content += "}";
    content += ".keyboard {";
    content += "  display: grid;";
    content += "  grid-template-columns: repeat(22, 60px);";  // 22 columns/rows horisontally
    //content += "  grid-template-rows: repeat(10, 60px);"; // 10 rows vertically
    content += "  grid-gap: 15px;";                           // spacing between keys
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
    content += "<div class='keyboard'>"; // ad key id for easier manipulation
    // driving
    content += "<div id='keyD' class='key' style='grid-column: 12; grid-row: 26;'>D</div>";  // move key to make website look cool
    content += "<div id='keyS' class='key' style='grid-column: 11; grid-row: 26;'>S</div>";  // move key to make website look cool
    content += "<div id='keyA' class='key' style='grid-column: 10; grid-row: 26;'>A</div>";  // move key to make website look cool
    content += "<div id='keyW' class='key' style='grid-column: 11; grid-row: 24;'>W</div>";  // move key to make website look cool
    //acceleration modes
    content += "<div id='keyH' class='key' style ='grid-column: 6; grid-row: 22;'>Sport</div>"; // H for sport mode/sport acceleration
    content += "<div id='keyL' class='key' style ='grid-column: 6; grid-row: 24;'>Eco</div>";     // L for Eco mode/eco acceleration
    content += "<div id='keyN' class='key' style ='grid-column: 6; grid-row: 26;'>regular</div>";  // N button for regular acceleration/ regular mode
    //charging modes
    content += "<div id='key1' class='key' style ='grid-column: 15; grid-row: 22;'>Charge</div>";   //1 for regular charge
    content += "<div id='key2' class='key'style ='grid-column: 15; grid-row: 24;'>Car2Car</div>";  //2 for car to car charge
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
    } else {
      server.send(400, "text/plain", "Bad request");
    }
  });

  server.begin();
  Serial.println("HTTP server started");
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
    ubidots.subscribeLastValue(DEVICE_LABEL, VARIABLE_LABEL);
    ubidots.subscribeLastValue(DEVICE_LABEL, VARIABLE_LABELD);
    ubidots.subscribeLastValue(DEVICE_LABEL, VARIABLE_LABELE);
  }
  ubidots.loop();

  receiveData();
}


void receiveData() {
  Wire.requestFrom(8, sizeof(info));  // Request data from slave address 8

  if (Wire.available() != sizeof(info)) {  //checks that the size matches
    Serial.println("Received data size does not match struct size!");
    return;
  }

  Wire.readBytes((uint8_t*)&info, sizeof(sendInfo));  // reads the received info

  //access received data
  float receivedSpeed = info.s;
  receivedDiscount = info.d;
  receivedBattery = info.p;

  if (receivedSpeed != newSpeed) {
    Serial.print("Speed: ");
    Serial.println(receivedSpeed);
    ubidots.add(VARIABLE_LABEL, receivedSpeed);
    ubidots.publish();
    newSpeed = receivedSpeed;
  }

  if (receivedDiscount != newDiscount) {
    Serial.print("Discount: ");
    Serial.println(receivedDiscount);
    /*yourElprice = elprice - receivedDiscount;
    ubidots.add(VARIABLE_LABELD, yourElPrice);  //deducted el-price
    ubidots.add(VARIABLE_LABELE, elPrice);      //general el-price
    ubidots.publish();
    newDiscount = receivedDiscount;*/
  }

  if (receivedBattery != newBattery) {
    newBattery = receivedBattery;
  }
}
