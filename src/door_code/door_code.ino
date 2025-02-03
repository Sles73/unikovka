/*
comands to test the programe on mosquitto-client

mosquitto_pub  -h localhost -u "mqtt" -P "mqtt" -t "home/led" -m "1"
mosquitto_sub -h localhost -u "mqtt" -P "mqtt" -t "home/status"

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define FIRMWARE_VERSION "0.0.3" 

// detaily sítě
***REMOVED***
***REMOVED***
const char* mqttServer = "192.168.55.2";
const int mqttPort = 1883;
const char* mqttUser = "mqtt";
const char* mqttPassword = "mqtt";
const char* mqttLockerTopic = "room/locker";
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
int locked_time = 0;


void checkLock(){
indication();

if (digitalRead(BUTTON_PIN) == LOW) { // Button pressed (active low)
        locked_time++;
        digitalWrite(LED_BUILTIN, HIGH);
        if(locked_time > 100 && state){
          digitalWrite(lockPin, HIGH);
          delay(500);
          digitalWrite(lockPin, LOW);
          delay(500);
        }
    }else{
      locked_time = 0;
      digitalWrite(LED_BUILTIN, LOW);
    }
}

//odpověď na dotaz dostupnosti
void mqtt_response(char* message){
  // Create a JSON document
  StaticJsonDocument<200> doc;

  // Add device name and elapsed time
  doc["name"] = "Locker";
  doc["time"] = millis();  // Time in seconds
  doc["firmware_version"] = FIRMWARE_VERSION;
  doc["state"] = state;
  doc["message"] = message;

  // Serialize JSON to string
  String jsonString;
  serializeJson(doc, jsonString);
  // Output JSON to Serial
  Serial.println(jsonString);
  client.publish("room/status", jsonString.c_str());

}

void rgb(bool r,bool g,bool b){
  digitalWrite(green, 1-g);
  digitalWrite(red, 1-r);
  digitalWrite(blue, 1-b);
}

//přepínání indikačních diod
void indication(){
  if(state){
    digitalWrite(green,LOW);
    digitalWrite(red,HIGH);
  }else{
    digitalWrite(green,HIGH);
    digitalWrite(red,LOW);
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

  /*
  // Control the LED based on received message
  if (message == "1") {
    digitalWrite(ledPin, HIGH);  // Turn ON LED
  } else if (message == "0") {
    digitalWrite(ledPin, LOW);  // Turn OFF LED
  }
  */

  if(strcmp(topic, mqttLockerTopic) == 0){
    if (message == "1") {
      state = true;
      mqtt_response("unlocked");
    }else if (message == "0") {
      state = false;
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
    if (client.connect("WemosClient", mqttUser, mqttPassword)) {
      Serial.println("connected");
      client.subscribe(mqttLockerTopic);  // Subscribe to the LED control topic
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

  // Setup built-in LED pin
  pinMode(ledPin, OUTPUT);
  pinMode(lockPin, OUTPUT);
  pinMode(green, OUTPUT);
  pinMode(red, OUTPUT);

  //the input if its closed
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  digitalWrite(ledPin, LOW);  // Start with the LED off
  digitalWrite(lockPin, LOW);

  rgb(1,0,1);

  // Initialize serial monitor
  Serial.begin(115200);
  Serial.println("setup runned");

  // Connect to Wi-Fi
  setupWifi();

  // Connect to MQTT broker
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  // Send an initial message when starting up
  if (client.connect("WemosClient", mqttUser, mqttPassword)) {
    mqtt_response("Started and connected");
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
