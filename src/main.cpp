#include <AccelStepper.h>
#include <ESP32Servo.h>

// Define stepper motor connections and motor interface type. Motor interface type must be set to 1 when using a driver:
const int dirPin = 32;
const int stepPin = 14;
const int servoPin = 33;
const int calibrationPin = 15;
const int motorInterfaceType = 1;

const int PIN_COUNT = 100;
const int STEPS_PER_TURN = 600;
const int WRAP_MOVE = 18;

const int ARM_EXT = 160;
const int ARM_RTCT = 90;
const int ARM_DELAY = 300;

#define INPUT_SIZE 30

// Create a new instance of the AccelStepper class:
AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, dirPin);
Servo armServo;

int currentPin = 0;
int pinOffset = 0;

void setup()
{
  // Set the maximum speed in steps per second:
  Serial.begin(9600);
  stepper.setMaxSpeed(1000);
  armServo.attach(servoPin);
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
  int posDiff = pinDiff * (STEPS_PER_TURN / PIN_COUNT);
  stepper.setAcceleration(100);
  stepper.moveTo(posDiff);
  while (stepper.distanceToGo() != 0)
  {
    stepper.run();
  }
  currentPin = pin;
}

void wrapPin()
{
  armServo.write(ARM_RTCT);
  delay(ARM_DELAY);
  stepper.move(WRAP_MOVE);
  while (stepper.distanceToGo() != 0)
  {
    stepper.run();
  }
  delay(ARM_DELAY);
  armServo.write(ARM_EXT);
}

void loop()
{
  if (Serial.available())
  {
    String command = Serial.readStringUntil('\n');
    int nextPin = command.toInt();
    goToPin(nextPin);
    wrapPin();

    Serial.print("Current Pin: ");
    Serial.println(currentPin);
  }
}