#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Arduino.h>
#include "UbidotsEsp32Mqtt.h"

#define I2C_SDA 13  //bus
#define I2C_SCL 12  //bus


//WiFi
const char* ssid = "NTNU-IOT";  //wifi name
const char* password = "";      //Wifi password
WebServer server(80);           // basic server

//I2C
int receivedData;  // data from arduino

//ubidots
const char* UBIDOTS_TOKEN = "BBUS-AcTJ6ccXQVpNyEBYHJ1dRor0GteJmb";
const char* DEVICE_LABEL = "esp32";
const char* VARIABLE_LABEL = "Akselerasjon";
const char* VARIABLE_LABEL_1 = "Parkeringsplass1";
const char* VARIABLE_LABEL_2 = "Parkeringsplass2";
const char* VARIABLE_LABEL_EL = "Strompris";
Ubidots ubidots(UBIDOTS_TOKEN);
int cheapPrice = 2;  // used to regulate electric bill based on driving patterns
int regularPrice = 4;
int highPrice = 6;
int lastValue = 0;
int electricValue = 0;
int lastElectricValue = 0;

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
  ubidots.subscribeLastValue(DEVICE_LABEL, VARIABLE_LABEL_EL);
  ubidots.subscribeLastValue(DEVICE_LABEL, VARIABLE_LABEL_1);
  ubidots.subscribeLastValue(DEVICE_LABEL, VARIABLE_LABEL_2);

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
    content += ".keyboard {";
    content += "  display: grid;";
    content += "  grid-template-columns: repeat(4, 60px);";  // 4 columns
    content += "  grid-gap: 10px;";                          // spacing between keys
    content += "}";
    content += ".key {";
    content += "  width: 100%;";
    content += "  height: 60px;";
    content += "  border: 2px solid #000;";
    content += "  text-align: center;";
    content += "  line-height: 60px;";
    content += "  font-size: 20px;";
    content += "}";
    content += ".pressed {";                  // New CSS class for pressed keys
    content += "  background-color: green;";  // Set background color to green
    content += "}";
    content += "</style>";
    content += "</head><body>";
    content += "<h1>Zumo controls</h1>";
    content += "<h2>Use the following keys to control the Zumo:</h2>";
    content += "<div class='keyboard'>";
    content += "<div id='keyW' class='key'>W</div>";  // Add an id to each key for easier manipulation
    content += "<div id='keyA' class='key'>A</div>";
    content += "<div id='keyS' class='key'>S</div>";
    content += "<div id='keyD' class='key'>D</div>";
    content += "<div id ='keyF'class='key'>F</div>";
    content += "<div id = 'keyC'class='key'>C</div>";
    content += "</div>";
    content += "</body></html>";
    // acceleration buttons
    content += "</style>";
    content += "</head><body>";
    content += "<h2>Acceleration controls</h2>";
    content += "<div class ='keyboard'>";
    content += "<div id='keyH' class='key'>Sport</div>";    //H button for high acceleration/sport mode
    content += "<div id='keyL' class='key'>Eco</div>";      //L button for low acceleration/eco mode
    content += "<div id='keyN' class='key'>regular</div>";  // N button for regular acceleration/ regular mode
    content += "</div>";
    content += "</body></html>";
    // charge buttons
    content += "</style>";
    content += "</head><body>";
    content += "<h2>charging options</h2>";
    content += "<h3>Battery: insert something to show battery level</h3>";
    content += "<div class ='keyboard'>";
    content += "<div id='key1' class='key'>regular</div>";  //1 for regular charge
    content += "<div id='key2' class='key'>Fast</div>";     //2 for fast charge
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
      Wire.beginTransmission(0x1E);  // sets arduino as slave.
      Wire.write(key[0]);                                     //sends one byte. the first of the key that gets pressed
      Wire.endTransmission();
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
    ubidots.subscribeLastValue(DEVICE_LABEL, VARIABLE_LABEL_EL);
  }
  ubidots.loop();

  sendToUbidots();
}



void sendToUbidots() {
  //speed
  Wire.requestFrom(0x1E, 3);  //request info from slave with address 0x1E

  if (Wire.available() == 3) {
    receivedData = Wire.read();                                        // Read the first byte
    int secondByte = Wire.read();                                      // Read the second byte
    int thirdByte = Wire.read();                                       // Read the third byte
    int value = (receivedData << 16) | (secondByte << 8) | thirdByte;  // Combine bytes into an integer value
    //Serial.println("received data from slave: " + String(value));
    if (value != lastValue) {

      float floatValue = float(value);
      //ubidots.add(VARIABLE_LABEL, floatValue);  //sends data from "value" to ubidots
      //ubidots.publish();
      lastValue = value;
    }  //ubidots' stem users can only send 4000dots per day. thats why we only send the values if they change.
  }
  // no point in sending the same value several times
  //electric price shown on chart on ubidots
  if (electricValue != lastElectricValue) {
    float priceToAdd;
    if (electricValue == 400) {  // 400 is the value for sportSpeed
      priceToAdd = highPrice;
    } else if (electricValue == 200) {  //200 is the value for EcoSpeed
      priceToAdd = cheapPrice;
    } else if (electricValue == 300) {  //300 is the value for regular speed
      priceToAdd = regularPrice;
    } else {
      priceToAdd = regularPrice;
    }
   // ubidots.add(VARIABLE_LABEL_EL, priceToAdd);
   // ubidots.publish();
    lastElectricValue = electricValue;
  }

}
