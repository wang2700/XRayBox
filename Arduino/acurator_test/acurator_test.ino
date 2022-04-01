/* Firgelli Automations
 *  
 * Program enables momentary direction control of actuator using push button
 */
 
#include <elapsedMillis.h>
elapsedMillis timeElapsed;

// pin map
int FPWM = 6;   
int RPWM = 5;
int opticalPin = 2;   // encoder
// int switchPin = 3;    // control switch
int frontSw = 18;
int sampleInSw = 19;
int sampleOutSw = 20;
int xRayCtl = 23;

volatile long lastDebounceTime_0=0; //timer for when interrupt was triggered
#define falsepulseDelay 20 //noise pulse time, if too high, ISR will miss pulses. 

int Speed = 255;   //Full speed for actuator

int Direction; //-1 retracting, 0 stopped, 1 is extending

int counter =0;
int prev_Counter = -1;
int prevCounter =0;
int extensionCount = 0;
int retractionCount = 0;
int pulseTotal =0;
int forwardSpeed = 100;
int backwardSpeed = 255;

int sampleNum = 3; // number of samples

void setup() {
  // Set DI/O pins
  pinMode(FPWM, OUTPUT);
  pinMode(RPWM, OUTPUT);
  pinMode(opticalPin, INPUT_PULLUP);
  // pinMode(switchPin, INPUT_PULLUP);
  pinMode(frontSw, INPUT_PULLUP);
  pinMode(sampleInSw, INPUT_PULLUP);
  pinMode(sampleOutSw, INPUT_PULLUP);
  pinMode(xRayCtl, OUTPUT);

  Serial.begin(9600);

  //Interrup for optical encoder
  attachInterrupt(digitalPinToInterrupt(opticalPin), count, RISING);

  //Interrup for door detection
  // attachInterrupt(digitalPinToInterrupt(frontSw), )
  
  //Actuator Retract to original position
  Serial.println("Actuator retracting...");
  Direction = -1;
  driveActuator(-1, 255);
  Serial.println("Actuator fully retracted");
  delay(10000);
  
  counter = 0;
  Direction = 1;
  driveActuator(1, 100);
}

void loop(){
  Serial.println(counter);
  delay(500);
  
}

//This function moves the actuator to one of its limits
void moveTillLimit(int Direction, int Speed){   
  counter = 0; //reset counter variables
  prevCounter = 0;
  
  do {
        prevCounter = counter;
        timeElapsed = 0;
        while(timeElapsed < 200){ //keep moving until counter remains the same for a short duration of time
          driveActuator(Direction, Speed);
        }
  } while(compareCounter(prevCounter, counter)); //loop until all counts remain the same
}

bool compareCounter(volatile int prevCounter, volatile int counter){
  bool areUnequal = true;
  
  if(prevCounter == counter){
      areUnequal = false;
  } 
  else{ //if even one pair of elements are unequal the entire function returns true
      areUnequal = true;
  }
  return areUnequal;
}

void driveActuator(int Direction, int Speed){
  switch(Direction){
    case 1:       //extension
      analogWrite(FPWM, Speed);
      analogWrite(RPWM, 0);
      break;
   
    case 0:       //stopping
      analogWrite(FPWM, 0);
      analogWrite(RPWM, 0);
      break;

    case -1:      //retraction
      analogWrite(FPWM, 0);
      analogWrite(RPWM, Speed);
      break;
  }
}

void count(){
  //This interrupt function increments a counter corresponding to changes in the optical pin status
  counter += Direction;
}

// void doorOpened() {

// }