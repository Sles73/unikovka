// Define pin numbers
#define BUTTON_PIN D6  // Change to the actual GPIO you use for the button
#define LED_PIN LED_BUILTIN // Onboard LED (inverted logic on ESP8266)

void setup() {
    pinMode(BUTTON_PIN, INPUT_PULLUP); // Internal pull-up resistor enabled
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH); // Turn LED off initially (inverted logic)
}

void loop() {
    if (digitalRead(BUTTON_PIN) == LOW) { // Button pressed (active low)
        digitalWrite(LED_PIN, LOW);  // Turn LED on
        delay(200);  // Flash duration
        digitalWrite(LED_PIN, HIGH); // Turn LED off
        delay(200);  // Pause before checking again
    }
}
