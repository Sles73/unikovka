/*
comands to test the program on mosquitto-client

mosquitto_pub  -h localhost -u "mqtt" -P "mqtt" -t "home/led" -m "1"
mosquitto_sub -h localhost -u "mqtt" -P "mqtt" -t "home/status"
*/

#include <ESP8266WiFi.h>  // Use <WiFi.h> if using ESP32
#include <PubSubClient.h>
#include "secret.h"
// Replace these with your network a0nd MQTT broker details

const char* mqttServer = "192.168.55.146";
const int mqttPort = 1883;
const char* mqttUser = "mqtt";
const char* mqttPassword = "mqtt";
const char* mqttTopic = "home/led";

WiFiClient espClient;
PubSubClient client(espClient);

const int ledPin = LED_BUILTIN;  // Pin for built-in LED (typically D0 for Wemos)
const int lockPin = 14;
const int green = 5;
const int red = 4;

void indication(bool state){
  if(state){
    digitalWrite(green,LOW);
    digitalWrite(red,HIGH);
  }else{
    digitalWrite(green,HIGH);
    digitalWrite(red,LOW);
  }

}
// Function to connect to Wi-Fi
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

// Callback function to handle received MQTT messages
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Message received on topic: ");
  Serial.print(topic);
  Serial.print(" with message: ");
  Serial.println(message);

  
  // Control the LED based on received message
  if (message == "1") {
    digitalWrite(ledPin, HIGH);  // Turn ON LED
  } else if (message == "0") {
    digitalWrite(ledPin, LOW);  // Turn OFF LED
  }
  

/*
  if (message == "1") {
    digitalWrite(ledPin, HIGH);  // Turn ON LED
    digitalWrite(lockPin, HIGH);
    indication(true);
    delay(500);
    digitalWrite(ledPin, LOW);
    digitalWrite(lockPin, LOW);
    indication(false);
  }*/
}

// Function to connect to the MQTT broker
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("WemosClient", mqttUser, mqttPassword)) {
      Serial.println("connected");
      client.subscribe(mqttTopic);  // Subscribe to the LED control topic
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  // Initialize serial monitor
  Serial.begin(115200);
  Serial.println("setup runned");

  pinMode(15,OUTPUT);
  digitalWrite(15,LOW);

  // Setup built-in LED pin
  pinMode(ledPin, OUTPUT);
  pinMode(lockPin, OUTPUT);
  pinMode(green, OUTPUT);
  pinMode(red, OUTPUT);

  digitalWrite(ledPin, LOW);  // Start with the LED off
  digitalWrite(lockPin, LOW);
  digitalWrite(green, HIGH);
  digitalWrite(red, LOW);



  

  



  // Connect to Wi-Fi
  setupWifi();

  // Connect to MQTT broker
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  // Send an initial message when starting up
  if (client.connect("WemosClient", mqttUser, mqttPassword)) {
    client.publish("home/status", "Wemos started");
    client.subscribe(mqttTopic);
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  delay(5);
}
