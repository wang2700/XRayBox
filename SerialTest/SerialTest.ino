void setup() {
  Serial.begin(115200);
}

void loop() {
  Serial.println("Recieved from Arduino");
  if (Serial.available()) {
    while(Serial.available()) {
      char input = Serial.read();
    }
  }
  delay(1000);
}