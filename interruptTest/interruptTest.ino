const byte ledPin = 13;
const byte interruptPin = 19;
volatile byte state = LOW;

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), blink, FALLING);
}

void loop() {
  digitalWrite(ledPin, state);
}

void blink() {
  static unsigned long last_interrupt_time = 0;
  if (millis() - last_interrupt_time > 10) {
    state = !state;
    Serial.print("Interrupt-");
    Serial.println(millis());
  }
  last_interrupt_time = millis();
}