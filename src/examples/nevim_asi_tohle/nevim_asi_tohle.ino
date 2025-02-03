// Define pin numbers
#define BUTTON_PIN D6  // Change to the actual GPIO you use for the button
#define LED_PIN LED_BUILTIN // Onboard LED (inverted logic on ESP8266)
const int lockPin = 14;     

bool state = true;
int locked_time = 0;

void checkLock(){
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


void setup() {
    pinMode(BUTTON_PIN, INPUT_PULLUP); // Internal pull-up resistor enabled
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH); // Turn LED off initially (inverted logic)
    pinMode(lockPin, OUTPUT);
    digitalWrite(lockPin, LOW);
}

void loop() {
    checkLock();
    delay(5);
}
