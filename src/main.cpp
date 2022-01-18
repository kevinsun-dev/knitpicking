#include <AccelStepper.h>
#include <ESP32Servo.h>

// Define stepper motor connections and motor interface type. Motor interface type must be set to 1 when using a driver:
const int dirPin = 12;
const int stepPin = 13;
const int servoPin = 23;
const int calibrationPin = 4;
const int motorInterfaceType = 1;

const int PIN_COUNT = 75;
const int STEPS_PER_TURN = 600;
const int WRAP_MOVE = (STEPS_PER_TURN / PIN_COUNT);
const long CALIB_DEBOUNCE = 200;

const int ARM_EXT = 180;
const int ARM_RTCT = 20;
const int ARM_DELAY = 500;

#define INPUT_SIZE 30

// Create a new instance of the AccelStepper class:
AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, dirPin);
Servo armServo;

int currentPin = 0;
int currentPosition = 0;
int pinOffset = 0;
bool negativeTurnBoost = false;

void setup()
{
  // Set the maximum speed in steps per second:
  Serial.begin(9600);
  stepper.setMaxSpeed(2000);
  stepper.setAcceleration(100);
  armServo.attach(servoPin);
  Serial.println("Beginnning Calibration");
  stepper.move(STEPS_PER_TURN);
  int indexingR = LOW;
  int prevReading = LOW;
  int time = 0;
  while (stepper.distanceToGo() != 0)
  {
    indexingR = digitalRead(calibrationPin);
    if (indexingR == HIGH && prevReading == LOW && millis() - time > CALIB_DEBOUNCE){
      currentPosition = stepper.distanceToGo();
      time = millis();
    }
    prevReading = indexingR;
    stepper.run();
  }
  stepper.move(-currentPosition+(0.32*STEPS_PER_TURN) + 4);
  stepper.runToPosition();
  stepper.setCurrentPosition(0);
  Serial.println("Ready");
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void goToPin(int pin)
{
  int pinDiff = pin - currentPin;
  if (abs(pin - (currentPin + PIN_COUNT)) < abs(pinDiff))
    pinDiff = pin - (currentPin + PIN_COUNT);
  int posDiff = pin * ((double) STEPS_PER_TURN / PIN_COUNT);
  stepper.moveTo(posDiff);
  if (stepper.distanceToGo() < 0){
    negativeTurnBoost = true;
  }
  stepper.runToPosition();
  currentPin = pin;
}

void wrapPin()
{
  armServo.write(ARM_RTCT);
  delay(ARM_DELAY);
  if (negativeTurnBoost){
    stepper.move(WRAP_MOVE + 4);
  } else {
    stepper.move(WRAP_MOVE);
  }
  stepper.runToPosition();
  armServo.write(ARM_EXT);
  delay(ARM_DELAY);
  if (negativeTurnBoost){
    stepper.move(-WRAP_MOVE - 2);
    negativeTurnBoost = false;
  } else {
    stepper.move(-WRAP_MOVE + 1);
  }
  stepper.runToPosition();
}

void loop()
{
  if (Serial.available())
  {
    String command = Serial.readStringUntil('\n');
    int nextPin = command.toInt();
    goToPin(nextPin);
    wrapPin();

    while (Serial.available() > 0)
    {
    }
    Serial.print("Current Pin: ");
    Serial.print(currentPin);
    Serial.println(".");
  }
}