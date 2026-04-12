#include <Arduino.h>
#include <Servo.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include "Controls.h"

#define SDA	P36
#define SCL	P37
#define SHUTDOWN P34
// #define INT P22
#define ADDR 0x30
#define DETECT_RANGE 500

Adafruit_VL53L0X tof = Adafruit_VL53L0X();
TwoWire bus = TwoWire(1);
// bool detected;
// VL53L0X_RangingMeasurementData_t measure;
// int tofReading;

bool checkInRange(int, int, int);
void stopIdle(bool);
void turnHeadRight(int);
void turnHeadLeft(int);
void readTof(int);

void idle(void *);
void reading(void *);

TaskHandle_t Idle;
TaskHandle_t Read;

int SensorDistance;

void setup()
{
	// delay(1000);
	Serial.begin(115200);

	/*
	while (!Serial)
	{
		delay(1);
	}
	*/

	if (!bus.begin(SDA, SCL))
	{
		Serial.println("Failed to initialize i2c bus!");
	}

	Serial.println("Initialized i2c bus!");

	digitalWrite(SHUTDOWN, LOW);

	if (!tof.begin(ADDR, true, &bus))
	{
		Serial.println("Initialization of VL53L0X failed!");
		while(true);
	}

	digitalWrite(SHUTDOWN, HIGH);
	Serial.println("Successfully initialized VL53L0X!");

	// digitalWrite(INT, HIGH);
	initControls();

	xTaskCreatePinnedToCore(
		idle,
		"Idle",
		10000,
		NULL,
		1,
		&Idle,
		0
	);

	xTaskCreatePinnedToCore(
		reading,
		"Read",
		200000,
		NULL,
		1,
		&Read,
		1
	);
}

void loop()
{

}

/*
void readTof(int idealRange)
{
	tofReading = tof.rangingTest(&measure, false);
	if(checkInRange(measure.RangeStatus, measure.RangeMilliMeter, idealRange))
	{
		Serial.println("Blocked!");
		stopHead();
	}

	else 
	{
		Serial.println("Not blocked.");
	}
}
*/

bool checkInRange(int status, int distance, int range)
{
	return status != 4 && distance <= range;
}

void turnHeadRight()
{
    for(int i = NECK_MIN_ANGLE; i < NECK_MAX_ANGLE; i++)
    {
		// readTof(idealRange);
        neck.write(i);
        currentNeckAngle = i;
        // tof.rangingTest(&measure, false);
    	vTaskDelay(NECK_TURN_DELAY / portTICK_PERIOD_MS);
    }
}

void turnHeadLeft()
{
    for(int i = NECK_MAX_ANGLE; i > NECK_MIN_ANGLE; i--)
    {
		// readTof(idealRange);
        neck.write(i);
        currentNeckAngle = i;
		// tof.rangingTest(&measure, false);
        vTaskDelay(NECK_TURN_DELAY / portTICK_PERIOD_MS);
    }
}

void idle(void *parameters)
{
	for(;;)
	{
		centerHead();

		turnHeadLeft();
		vTaskDelay(NECK_STOP_DELAY / portTICK_PERIOD_MS);

		turnHeadRight();
		vTaskDelay(NECK_STOP_DELAY / portTICK_PERIOD_MS);
	}
}

void reading(void *parameters)
{
	for(;;)
	{
		VL53L0X_RangingMeasurementData_t measure;
		tof.rangingTest(&measure, false);
		SensorDistance = measure.RangeMilliMeter;
		Serial.println(SensorDistance);
		if(checkInRange(measure.RangeStatus, SensorDistance, DETECT_RANGE))
		{
			digitalWrite(RED_LED_PIN, HIGH);
            digitalWrite(BLUE_LED_PIN, LOW);
			Serial.println("Blocked!");
			Serial.println(SensorDistance);
			stopHead();
		}
		else 
		{
			digitalWrite(RED_LED_PIN, LOW);
            digitalWrite(BLUE_LED_PIN, HIGH);
		}
		// vTaskDelay(50 / portTICK_PERIOD_MS);
	}
}