#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

#include <ESP8266WiFi.h>  // Use <WiFi.h> if using ESP32
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "secret.h"

#define FIRMWARE_VERSION "0.0.1" 

const char* DEVICE_ID = "dfaa202e";


const char* mqttPanelTopic = strcat("devices/",DEVICE_ID);
const char* mqttAvailableTopic = "room/available";

WiFiClient espClient;
PubSubClient client(espClient);

#define PIN        D4 
#define NUMPIXELS 2 

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);


int status[NUMPIXELS];




void setupWifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Connected to WiFi");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("WemosClient")) {
      Serial.println("connected");
      client.subscribe(mqttPanelTopic);  // Subscribe do topiců
      client.subscribe(mqttAvailableTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void mqtt_response(char* message){
  DynamicJsonDocument doc(3072);

  JsonObject headers  = doc.createNestedObject("headers");
  headers["messageType"] = "deviceStatusEvent";
  headers["deviceId"] = DEVICE_ID;


  JsonObject data  = doc.createNestedObject("data");
  data["deviceState"] = "ACTIVE";

  JsonObject deviceIdentification  = data.createNestedObject("deviceIdentification");
  deviceIdentification["deviceType"] = "Locker";
  deviceIdentification["deviceId"] = DEVICE_ID;
  
  JsonObject deviceOptions  = data.createNestedObject("deviceOptions");
  deviceOptions["message"] = message;


  String jsonString;
  serializeJson(doc, jsonString);
  const char* output = jsonString.c_str();
  client.publish("hub/input", output);
  //Serial.println(output);
}


void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Message received on topic: ");
  Serial.print(topic);
  Serial.print(" with message: ");
  Serial.println(message);

  
if(strcmp(topic, mqttPanelTopic) == 0){
  
    if (message == "g") {
      mqtt_response("green");
      status[0] = 1;
    }else if (message == "bg") {
      mqtt_response("blinking green");
      status[0] = 2;
    }else if (message == "o") {
      mqtt_response("orange");
      status[0] = 3;
    }else if (message == "bo") {
      mqtt_response("blinking orange");
      status[0] = 4;
    }else if (message == "r") {
      mqtt_response("red");
      status[0] = 5;
    }else if (message == "br") {
      mqtt_response("blinking red");
      status[0] = 6;
    }else if (message == "off") {
      mqtt_response("off");
      status[0] = 0;
    }
  }else if(strcmp(topic, mqttAvailableTopic) == 0){
        mqtt_response("alive");
        Serial.println("alive");
  }
}


void setup() {
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif

for (int i = 0; i < NUMPIXELS; i++) {
  status[i] = 0;
}

Serial.begin(115200);
pixels.begin();
delay(50);
pixels.clear(); 
pixels.setPixelColor(0, pixels.Color(0, 38, 38));
pixels.show();

// Connect to Wi-Fi
setupWifi();
// Connect to MQTT broker
client.setServer(mqttServer, mqttPort);
client.setCallback(callback);

// Send an initial message when starting up
if (client.connect("WemosClient")) {
  mqtt_response("conected");
  client.subscribe(mqttPanelTopic);
  client.subscribe(mqttAvailableTopic);
  }
}

void loop() {

if (!client.connected()) {
  reconnect();
}
client.loop();
delay(5);

for(int i=0; i<NUMPIXELS; i++) {
  if(status[i] == 0){
    pixels.setPixelColor(i, pixels.Color(0, 0, 0));
  }else if (status[i] == 1){
    pixels.setPixelColor(i, pixels.Color(38, 0, 0));
  }else if (status[i] == 2){
    if(millis()/250%2==0){
      pixels.setPixelColor(i, pixels.Color(38, 0, 0));
    }else{
      pixels.setPixelColor(i, pixels.Color(0, 0, 0));
    }
  }else if (status[i] == 3){
    pixels.setPixelColor(i, pixels.Color(20, 38, 0));
  }else if (status[i] == 4){
    if(millis()/250%2==0){
      pixels.setPixelColor(i, pixels.Color(20, 38, 0));
    }else{
      pixels.setPixelColor(i, pixels.Color(0, 0, 0));
    }
  }else if (status[i] == 5){
    pixels.setPixelColor(i, pixels.Color(0, 38, 0));
  }else if (status[i] == 6){
    if(millis()/250%2==0){
      pixels.setPixelColor(i, pixels.Color(0, 38, 0));
    }else{
      pixels.setPixelColor(i, pixels.Color(0, 0, 0));
    }
  }
  
}
pixels.show();


}

