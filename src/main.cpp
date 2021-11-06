#include <AccelStepper.h>
#include <ESP32Servo.h>

// Define stepper motor connections and motor interface type. Motor interface type must be set to 1 when using a driver:
const int dirPin = 32;
const int stepPin = 14;
const int servoPin = 33;
const int motorInterfaceType = 1;

#define INPUT_SIZE 30

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

// Create a new instance of the AccelStepper class:
AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, dirPin);
Servo armServo;

void setup()
{
  // Set the maximum speed in steps per second:
  Serial.begin(9600);
  stepper.setMaxSpeed(1000);
  armServo.attach(servoPin);
}

void loop()
{
  if (Serial.available())
  {
    String command = Serial.readStringUntil('\n');
    int angle = command.toInt();
    stepper.setAcceleration(100);
    stepper.moveTo(angle);
    while (stepper.distanceToGo() != 0)
    {
      stepper.run();
    }
    armServo.write(90);
    delay(500);
    stepper.moveTo(angle+18);
    while (stepper.distanceToGo() != 0)
    {
      stepper.run();
    }
    delay(500);
    armServo.write(160);

    Serial.print("Stepper: ");
    Serial.println(angle);
  }
}