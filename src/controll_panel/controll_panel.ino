#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

#include <ESP8266WiFi.h>  // Use <WiFi.h> if using ESP32
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "UUID.h"
#include "secret.h"

#define FIRMWARE_VERSION "0.0.1" 

const char* DEVICE_ID = "dfaa202e";
const char* mqttPanelTopic = strcat("devices/",DEVICE_ID);
const char* mqttAvailableTopic = "room/available";

WiFiClient espClient;
PubSubClient client(espClient);

UUID uuid;

#define PIN       D4 
#define NUMPIXELS 2 

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

String statuses[NUMPIXELS];
bool incomingHit[NUMPIXELS];




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
  uuid.generate();
  headers["messageId"] = uuid;



  JsonObject data  = doc.createNestedObject("data");
  data["deviceState"] = "ACTIVE";
  

  JsonObject deviceIdentification  = data.createNestedObject("deviceIdentification");
  deviceIdentification["deviceType"] = "controllPanel";
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

  JsonDocument doc;
  deserializeJson(doc, message);

  for (int i = 0; i < NUMPIXELS; i++) {
    statuses[i] = String(doc["data"]["deviceOptions"]["componentStatuses"][i]["status"]);
    incomingHit[i] = doc["data"]["deviceOptions"]["componentStatuses"][i]["incomingHit"];
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
  statuses[i] = "healthy";
  incomingHit[i] = false;
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
  if(millis()/250%2==0 && incomingHit[i]){
      pixels.setPixelColor(i, pixels.Color(0, 0, 0));
    }else{
      if(statuses[i] == "healthy"){
        pixels.setPixelColor(i, pixels.Color(38, 0, 0));
      }else if(statuses[i] == "broken"){
        pixels.setPixelColor(i, pixels.Color(20, 38, 0));
      }else if(statuses[i] == "destroyed"){
        pixels.setPixelColor(i, pixels.Color(0, 38, 0));
      }else if(statuses[i] == "off"){
        pixels.setPixelColor(i, pixels.Color(0, 0, 0));
      }
    }
}
pixels.show();


}

