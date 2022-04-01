/* Firgelli Automations
 *  
 * Program enables momentary direction control of actuator using push button
 */

// pin map
int FPWM = 6;   
int RPWM = 5;
int opticalPin = 2;   // encoder
int tempSen = A15;
int doorIn = 22;
int xRayCtrl = 23;
int fanCtrl = 25;

int driveDirection = 0; //-1 retracting, 0 stopped, 1 is extending

int counter =0;
int prevCounter = 700;
int extensionCount = 0;
int retractionCount = 0;
int pulseTotal =0;

// motor parameter
int fulllyExtend = 200;
int forwardSpeed = 100;
int backwardSpeed = 255;
int retractTime = 6000;

byte motorControlFlag = 0; // 0 - stop, 1 - drive forward, 2 - reach the end, 3 - drive backward,
byte doorStat = 0; // 0 - all closed, 1 - open doors
byte fanControlFlag = 0; // 0 - normal, 1 - overheat (low), 2 - overheat (high)
bool xRayPause = false;

float xRayTemp = 0;
float tempThreshFanOff = 55; // tempature for fan to turn off (degree C)
float tempThreshFanOn = 60; // temperature for fan to turn on (degree C)
float tempThreshXrayOn = 110; // temperature for xRay to turn back on
float tempThreshXrayOff = 115; // temperature for xRay to turn off 

int sampleNum = 0; // number of samples
int sampleCount = 0;

char command = 'n'; // 'n' - do nothing, 's' - start, 'r' - resume, 'c' - cancel, 'o' - device status
String input = "";

void count_0(){
  //This interrupt function increments a counter corresponding to changes in the optical pin status
  counter += driveDirection;
}

void setup() {
  // setup timer for compare counter
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  OCR1A = 31250;
  OCR1B = 15624;
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS12);
  TIMSK1 |= (1 << OCIE1A);
  interrupts();
  
  // Set DI/O pins
  pinMode(FPWM, OUTPUT);
  pinMode(RPWM, OUTPUT);
  pinMode(opticalPin, INPUT_PULLUP);
  // pinMode(switchPin, INPUT_PULLUP);
  pinMode(doorIn, INPUT_PULLUP);
  pinMode(xRayCtrl, OUTPUT);
  pinMode(fanCtrl, OUTPUT);

  Serial.begin(115200);

  //Interrup for optical encoder
  attachInterrupt(digitalPinToInterrupt(opticalPin), count_0, RISING);
  motorControlFlag = 3;
}

ISR(TIMER1_COMPA_vect) {
  if (motorControlFlag == 3) {
    if (prevCounter == counter) {
      if (xRayPause) {
        motorControlFlag = 0;
      } else {
        motorControlFlag = 1;
      }
      counter = 0;
    }
    prevCounter = counter;
  }

  if (digitalRead(doorIn) == 0) {
    doorStat = 1;
    xRayPause = true;
    // Serial.println("error-1-doors open");
  } else {
    doorStat = 0;
  }

  // Check temperature
  int temp = analogRead(tempSen);
  xRayTemp = 200.0 / 1024 * temp;
}

ISR(TIMER1_COMPB_vect) {
  
}

void loop(){
  
  
  if (xRayTemp < tempThreshFanOff)  {
    fanControlFlag = 0;
    digitalWrite(fanCtrl, LOW);
    // Serial.println("status-fan off");
  } else if (xRayTemp < tempThreshFanOn) {
    fanControlFlag = 1;
    digitalWrite(fanCtrl, HIGH);
    // Serial.println("status-fan on");
  } else if (xRayTemp < tempThreshXrayOn) {
    fanControlFlag = 1;
    // Serial.println("status-xray temp ok");
  } else {
    fanControlFlag = 2;
    xRayPause = true;
    digitalWrite(fanCtrl, HIGH);
    digitalWrite(xRayCtrl, LOW);
    // Serial.println("error-xRay Module Overheat");
  }

  switch (motorControlFlag) {
    case 0:
      driveActuator(0, 0);
      if (command == 'r') {
        Serial.println(doorStat);
        if (doorStat == 0 && fanControlFlag != 2) {
          motorControlFlag = 1;
          xRayPause = false;
        } else {
          Serial.println("error-cannot resume");
        }
        command = 'n';
        
      }
      break;
    case 1:
      if (sampleCount < sampleNum) {
        if (counter >= fulllyExtend) {
            motorControlFlag = 2;
          } else {
            if (xRayPause) {
              digitalWrite(xRayCtrl, LOW);
            } else {
              digitalWrite(xRayCtrl, HIGH);
            }
            driveActuator(1, forwardSpeed);
          }
      } else {
        sampleCount = 0;
        sampleNum = 0;
        motorControlFlag = 0;
      }
      break;

    case 2:
      Serial.println("status-end reached");
      digitalWrite(xRayCtrl, LOW);
      driveActuator(0, 0);
      if (!xRayPause) {
        sampleCount++;
      }
      motorControlFlag = 3;
      prevCounter = counter;
      break;

    case 3:
      driveActuator(-1, backwardSpeed);
      break;
  }
  
}

void SerialEvent() {
  while (Serial.available()) {
    char inChar = (char) Serial.read();
    input += inChar;
    if (inChar == '\n') {
      break;
    }
  }
  command = input.charAt(0);
  switch (command) {
    case 's': // start scanning
      sampleNum = input.substring(2).toInt();
      motorControlFlag = 1;
      break;
    case 'c': // cancel scanning
      sampleNum = 0;
      sampleCount = 0;
      break;
    case 'o': // get device status
      String output = "status-";
      output += motorControlFlag == 0 ? "free-" : "busy-";
      output += xRayPause == 0 ? "off-" : "on-";
      output += doorIn == 0 ? "close-" : "open-";
      output += String(xRayTemp, 2);
      Serial.println(output);
      break;
  }
}

void driveActuator(int direction, int speed){
  driveDirection = direction;
  switch(direction){
    case 1:       //extension
      analogWrite(FPWM, speed);
      analogWrite(RPWM, 0);
      break;
   
    case 0:       //stopping
      analogWrite(FPWM, 0);
      analogWrite(RPWM, 0);
      break;

    case -1:      //retraction
      analogWrite(FPWM, 0);
      analogWrite(RPWM, speed);
      break;
  }
}
