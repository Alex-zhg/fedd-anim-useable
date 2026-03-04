#pragma once
#include <Arduino.h>

// Initialize the VL53L0X sensor over I2C
// Returns true on success, false if sensor not found
bool initSensor();

// Returns distance in millimeters.
// Returns -1 on read error or out-of-range.
int readDistanceMM();
