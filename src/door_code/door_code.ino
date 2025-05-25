/*
comands to test the programe on mosquitto-client

mosquitto_pub  -h localhost -u "mqtt" -P "mqtt" -t "room/locker" -m "1"
mosquitto_sub -h localhost -u "mqtt" -P "mqtt" -t "room/status"
mosquitto_pub  -h localhost -u "mqtt" -P "mqtt" -t "room/available"

*/
#include "secret.h"



#include <ESP8266WiFi.h>  // Use <WiFi.h> if using ESP32
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define FIRMWARE_VERSION "0.0.6" 
const char* DEVICE_ID = "dfaa202d";

// detaily sítě

const char* mqttLockerTopic = strcat("devices/",DEVICE_ID);
const char* mqttAvailableTopic = "room/available";

WiFiClient espClient;
PubSubClient client(espClient);

const int ledPin = LED_BUILTIN;  // integrovaný pin na EPS8266
const int lockPin = 14;          // pin pro odemčení zámku
const int green = 5;             // pin zelené indikační led
const int red = 4;               // pin červené indikační led
const int blue = 2;

#define BUTTON_PIN D6 

bool state = false;

short opening_retries = 0;
bool should_be_closed = true;


void checkLock(){
  indication();
  if (digitalRead(BUTTON_PIN) == LOW) { // doors closed (active low)
    if(millis()%500 < 20){
      if(state){
        digitalWrite(lockPin, HIGH);
        delay(500);
        digitalWrite(lockPin, LOW);
        delay(500);

        opening_retries++;
        if(opening_retries > 5){
          mqtt_response("ERROR: can't open");
        }
        return;
      }

      if(should_be_closed == false){
        should_be_closed =  true;
        mqtt_response("doors closed");
      }
    }
  }else{                        // doors opened
    opening_retries = 0;
    if(state == false && millis()%1000 < 10 && should_be_closed == true){ // logic is closed(false) and should closed(true) 
      mqtt_response("ERROR: states not corespoding");
    }
  }
}


//odpověď na dotaz dostupnosti
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
  deviceOptions["logic_state"] = state;
  deviceOptions["fyzical_state"] = digitalRead(BUTTON_PIN) ? true : false;
  deviceOptions["message"] = message;


  String jsonString;
  serializeJson(doc, jsonString);
  const char* output = jsonString.c_str();
  client.publish("hub/input", output);
  //Serial.println(output);
}

//rgb ledka
void rgb(bool r,bool g,bool b){
  digitalWrite(green, 1-g);
  digitalWrite(red, 1-r);
  digitalWrite(blue, 1-b);
}

//přepínání indikačních diod
void indication(){
  if(state){
    rgb(0,1,0);
  }else{
    rgb(1,0,0);
  }

}

// Funkce pro připojení k wifi
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

// Callback funkce pro mqtt příchozí zprávy
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Message received on topic: ");
  Serial.print(topic);
  Serial.print(" with message: ");
  Serial.println(message);
  Serial.println(strcmp(topic, mqttAvailableTopic) == 0);

  if(strcmp(topic, mqttLockerTopic) == 0){
    if (message == "1") {
      state = true;     //nastavení logického stavu
      should_be_closed = false;
      mqtt_response("unlocked");
    }else if (message == "0") {
      state = false;    //nastavení logického stavu
      mqtt_response("locked");
    }
  }else if(strcmp(topic, mqttAvailableTopic) == 0){
        mqtt_response("alive");
        Serial.println("alive");
  }
}

// Function to connect to the MQTT broker
void reconnect() {
  rgb(1,0,1);
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("WemosClient")) {
      Serial.println("connected");
      client.subscribe(mqttLockerTopic);  // Subscribe do topiců
      client.subscribe(mqttAvailableTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  pinMode(15,OUTPUT);
  digitalWrite(15,LOW);

  pinMode(lockPin, OUTPUT);
  pinMode(green, OUTPUT);
  pinMode(red, OUTPUT);
  pinMode(blue, OUTPUT);

  //nastavení detekce zavřených dvířek
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  //pro jistotu nastavení zámku na low
  digitalWrite(lockPin, LOW);

  //indikace načítání fialovou barvou 
  rgb(1,0,1);

  //Seriový monitor
  Serial.begin(115200);
  Serial.println("setup runned");

  // Connect to Wi-Fi
  setupWifi();

  // Connect to MQTT broker
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);



  // Send an initial message when starting up
  if (client.connect("WemosClient")) {
    mqtt_response("Started and connected");
    client.publish("hub/input", "Wemos started");
    client.subscribe(mqttLockerTopic);
    client.subscribe(mqttAvailableTopic);
  }

}

void loop() {
  if (!client.connected()) {
    reconnect();
    }else{
    client.loop();
    delay(5);
    checkLock();
    }
}
