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
int limitEndPin = 18;
int limitStartPin = 19;

int driveDirection = 0; //-1 retracting, 0 stopped, 1 is extending

int counter =0;
int prevCounter = 700;
int extensionCount = 0;
int retractionCount = 0;
int pulseTotal =0;

// motor parameter
int forwardSpeed = 255;
int backwardSpeed = 255;
int retractTime = 6000;

byte motorControlFlag = 0; // 0 - stop, 1 - drive forward, 2 - reach the end, 3 - drive backward,
bool doorStat = false; // false - all closed, true - open doors
bool fanControlFlag = false; // false - normal, true - overheat (low)
bool xRayOHFlag = false;
bool scanFailed = false;		// flag for showing if the x-ray has been turn off due to door open or overheating
bool xRayStat = false;		// x-ray status 
bool scanStart = false;		// flaf for showing motion is from a scan instead of homing operation

float xRayTemp = 0;
float tempThreshNormal = 55; // tempature for fan to turn off (degree C)
float tempThreshFanOn = 60; // temperature for fan to turn on (degree C)
// float tempThreshXrayNormal = 55; // temperature for xRay to turn back on
float tempThreshXrayOverheat = 80; // temperature for xRay to turn off 

bool serialConnected = false; // check if connected to computer
char command = 'n'; // 'n' - do nothing, 's' - start, 'r' - recieved, 'c' - connection, 'o' - device status, 'p' - pause
String input = "";
unsigned long sendTime = -1;
bool recieved = true;
bool sent = true;
int timeout = 500;

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
  

  //setup timer for sending status
  TCCR3A = 0;
  TCCR3B = 0;
  TCNT3 = 0;

  OCR3A = 50000;
  TCCR3B |= (1 << WGM12);
  TCCR3B |= (1 << CS32);
  TIMSK3 |= (1 << OCIE3A);

  interrupts();
  // Set DI/O pins
  pinMode(FPWM, OUTPUT);
  pinMode(RPWM, OUTPUT);
  pinMode(opticalPin, INPUT_PULLUP);
  pinMode(doorIn, INPUT_PULLUP);
	pinMode(limitEndPin, INPUT_PULLUP);
	pinMode(limitStartPin, INPUT_PULLUP);
  pinMode(xRayCtrl, OUTPUT);
  pinMode(fanCtrl, OUTPUT);

  digitalWrite(xRayCtrl, HIGH);
  digitalWrite(fanCtrl, HIGH);

  Serial.begin(115200);
	// homing
  driveActuator(-1, backwardSpeed);
  while (digitalRead(limitStartPin) == HIGH) {}
  driveActuator(0, 0);
  Serial.println("result-home complete");

  //Interrupt for two limit switches
	attachInterrupt(digitalPinToInterrupt(limitEndPin), reachEnd, FALLING);
	attachInterrupt(digitalPinToInterrupt(limitStartPin), reachStart, FALLING);
}

void reachEnd() {
  Serial.println("Reach End");
	xRayStat = false;
	motorControlFlag = 2;
}

void reachStart() {
  Serial.println("Reach Start");
	motorControlFlag = 0;
}

ISR(TIMER1_COMPA_vect) {

  if (digitalRead(doorIn) == 0) {
    doorStat = true;
    scanFailed = true; 	//turn off x-ray
    if (motorControlFlag == 1) {
      reachEnd(); // stop acurator and retact back to starting position
    }
  } else {
    doorStat = false;
  }

  // Check temperature
  int temp = analogRead(tempSen);
  xRayTemp = 200.0 / 1024 * temp;

  // control fan based on x-ray temp
  if (xRayTemp < tempThreshNormal)  { // less than the fan threshold - fan off and x-ray normal
    fanControlFlag = false;
    digitalWrite(fanCtrl, HIGH);
  } else if (xRayTemp > tempThreshFanOn) {	// x-ray heat to above fan on threshold - fan on and x-ray norma;
    fanControlFlag = true;
    digitalWrite(fanCtrl, LOW);
  } 

  // Control x-ray
  digitalWrite(xRayCtrl, xRayStat ? HIGH : LOW);
}

ISR(TIMER3_COMPA_vect) {
  Serial.println(generateStatusMessage());

}

void getCommandFromSerial() {
  if (Serial.available()) {
    input = Serial.readStringUntil('\n');
  }
  command = input.charAt(0);
  if (command == 's') {
		if (xRayOHFlag) {
			Serial.println("error-xray overheat");
		} else if (doorStat) {
			Serial.println("error-door open");
		} else {
			xRayStat = true;
      scanStart = true;
			motorControlFlag = 1;
			Serial.println("action-start scanning");
		}
	} else if (command == 'r') {
		if (serialConnected) {
			recieved = true;
		}
	} else if (command == 'c') {
		serialConnected = true;
		String message = "action-connected";
		message += "-";
		message += String(tempThreshNormal, 2);
		message += "-";
		message += String(tempThreshFanOn, 2);
		message += "-";
		message += String(tempThreshXrayOverheat, 2);
		Serial.println(message);
	}

	//check for message timeout
	// if (!recieved && (millis() - sentTime > timeout)) {
	// 	serialConnected = false;
	// }
}

String generateStatusMessage() {
  String status = "status-";
  status += motorControlFlag == 0 ? "free-" : "busy-";
  status += xRayStat ? "on-" : "off-";
  status += xRayOHFlag == 0 ? "yes-" : "no-";
  status += doorStat == 0 ? "close-" : "open-";
  status += fanControlFlag ? "off-" : "on-";
  status += String(xRayTemp, 2);
  return status;
}

void loop(){
  getCommandFromSerial();

	// control x-ray based on x-ray temp
	if (xRayTemp < tempThreshNormal) {
		xRayOHFlag = false;
  } else if (xRayTemp > tempThreshXrayOverheat) {
    scanFailed = true;
		xRayOHFlag = true;
    xRayStat = false;
		if (motorControlFlag == 1) {
      reachEnd();
    }
  }

	// control motor by the control flag
	if (motorControlFlag == 0) {
		driveActuator(0, 0);
	} else if (motorControlFlag == 1) {
		driveActuator(1, forwardSpeed);
	} else if (motorControlFlag == 2) {
		driveActuator(0, 0);
		Serial.println("action-camera off");
		if (scanFailed) {
			Serial.println("result-scan failed");
		} else {
			Serial.println("result-scan complete");
		}
		motorControlFlag = 3;
	} else {
		driveActuator(-1, backwardSpeed);
	}

  command = 'n';
	input = "";
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
