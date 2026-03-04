#pragma once
#include <Arduino.h>

// Connect to WiFi and start the web server
// Runs as a FreeRTOS task
void webTask(void* pvParameters);
