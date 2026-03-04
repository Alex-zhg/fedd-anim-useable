#pragma once
#include <Arduino.h>

void initBuzzer();

// Non-blocking single beep — fires and returns immediately
// Uses LEDC PWM tone generation on ESP32
void beepOnce(int frequencyHz, int durationMs);

// Must be called from main loop to handle beep timeout (non-blocking)
void updateBuzzer();
