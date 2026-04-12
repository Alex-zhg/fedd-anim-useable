#include <Arduino.h>
#include <Servo.h>
#include <Adafruit_VL53L0X.h>
#include <Wire.h>

#define SDA P36
#define SCL P37
// #define ADDR 0x30

#define NECK_PIN P4
#define JAW_PIN P5

#define NECK_MIN_ANGLE 0
#define NECK_MAX_ANGLE 180
#define NECK_CENTER_ANGLE 90

#define JAW_MIN_ANGLE 0
#define JAW_MAX_ANGLE 90

#define NECK_TURN_DELAY 5
#define NECK_STOP_DELAY 1000

#define JAW_TURN_DELAY 5
#define JAW_STOP_DELAY 100

#define RED_LED_PIN P28
#define BLUE_LED_PIN P27

Servo neck;
Servo jaw;
int currentNeckAngle;
int stoppedNeckAngle;

// TwoWire bus = TwoWire(1);
// Adafruit_VL53L0X tof = Adafruit_VL53L0X();
// VL53L0X_RangingMeasurementData_t measure;

void stopHead();
void centerHead();
void continueMotion();

void initControls()
{
    neck.attach(NECK_PIN);
    jaw.attach(JAW_PIN);
    // bus.begin(SDA, SCL);
    // tof.begin(ADDR, &bus);

    // neck.write(NECK_CENTER_ANGLE);

    currentNeckAngle = NECK_CENTER_ANGLE;
    centerHead();

    jaw.write(JAW_MIN_ANGLE);

    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(BLUE_LED_PIN, OUTPUT);
    
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(BLUE_LED_PIN, HIGH);
}

void centerHead()
{
    if(currentNeckAngle < NECK_CENTER_ANGLE)
    {
        for(currentNeckAngle; currentNeckAngle < NECK_CENTER_ANGLE; currentNeckAngle++)
        {
            neck.write(currentNeckAngle);
        }
    }

    else if(currentNeckAngle > NECK_CENTER_ANGLE)
    {
        for(currentNeckAngle; currentNeckAngle > NECK_CENTER_ANGLE; currentNeckAngle--)
        {
            neck.write(currentNeckAngle);
        }
    }

    else 
    {
        currentNeckAngle = NECK_CENTER_ANGLE;
    }
}

/*
void turnHeadRight(bool detected)
{
    for(int i = NECK_MIN_ANGLE; i < NECK_MAX_ANGLE; i++)
    {
        neck.write(i);
        currentNeckAngle = i;
        tof.rangingTest(&measure, false);
        delay(NECK_TURN_DELAY);

        if(detected)
        {
            stopHead(); 
        }
    }
}
*/

/*
void turnHeadLeft(bool detected)
{
    for(int i = NECK_MAX_ANGLE; i > NECK_MIN_ANGLE; i--)
    {
        neck.write(i);
        currentNeckAngle = i;
        delay(NECK_TURN_DELAY);

        if(detected)
        {
            stopHead();
        }
    }
}
*/

void operateJaw()
{
  for(int i = JAW_MIN_ANGLE; i < JAW_MAX_ANGLE; i++)
  {
    jaw.write(i);
    delay(JAW_TURN_DELAY);
  }

  delay(JAW_STOP_DELAY);
  
  for(int i = JAW_MAX_ANGLE; i > JAW_MIN_ANGLE; i--)
  {
    jaw.write(i);
    delay(JAW_TURN_DELAY);
  }
}

void stopHead()
{
    neck.write(currentNeckAngle);
    neck.detach();

    stoppedNeckAngle = currentNeckAngle;

    // operateJaw();
    delay(2000);

    continueMotion();
}

void continueMotion()
{
    neck.attach(NECK_PIN);
    // neck.write(stoppedNeckAngle);
}