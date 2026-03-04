// =============================================================================
// claude generated
// animatronic.ino — Main Entry Point
// Animatronic Gargoyle | NCSU IEEE Dev Board | ESP32-S3-WROOM-1
//
// Required Libraries (install via Arduino Library Manager):
//   - "VL53L0X" by Pololu
//   - "ESP32Servo" by Kevin Harrington
//   - "WebServer" (included with esp32 Arduino core)
//
// Board: "ESP32S3 Dev Module"
// Flash Mode: QIO / 80MHz
// PSRAM: Disabled (unless using WROOM-2)
// USB CDC On Boot: Enabled (for Serial output via USB)
// =============================================================================

#include "config.h"
#include "sensor.h"
#include "motion.h"
#include "leds.h"
#include "buzzer.h"
#include "fsm.h"
#include "webserver.h"

// ---------------------------------------------------------------------------
// Sensor task — runs on Core 0, polls VL53L0X and feeds shared telemetry
// The FSM reads directly via readDistanceMM() which is thread-safe
// (VL53L0X continuous mode, single reader)
// ---------------------------------------------------------------------------
void sensorTask(void* pvParameters) {
  if (!initSensor()) {
    Serial.println("[SENSOR] FATAL: Could not initialize VL53L0X. Halting sensor task.");
    vTaskDelete(NULL);
    return;
  }
  // Sensor runs in continuous mode — reads are non-blocking from FSM task
  // This task simply keeps the sensor alive and handles watchdog
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// ---------------------------------------------------------------------------
// LED + Buzzer update task — handles non-blocking pulse/beep timing
// Runs frequently to keep timing accurate
// ---------------------------------------------------------------------------
void outputTask(void* pvParameters) {
  while (true) {
    updateLEDs();
    updateBuzzer();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n============================================");
  Serial.println("  Animatronic Gargoyle — NCSU IEEE ESP32-S3");
  Serial.println("============================================");

  // Initialize hardware
  initLEDs();
  initBuzzer();
  initServos();

  // Brief startup animation — sweep jaw and blink eyes
  beepOnce(800, 80);
  delay(100);
  beepOnce(1200, 80);
  setLEDSolid(COLOR_WHITE);
  delay(300);
  setLEDSolid(COLOR_OFF);

  // Spawn FreeRTOS tasks
  // Core 0: Sensor + Web (less time-critical)
  // Core 1: FSM + Output (motion-critical)
  xTaskCreatePinnedToCore(sensorTask,  "Sensor",  STACK_SENSOR, NULL, PRI_SENSOR, NULL, 0);
  xTaskCreatePinnedToCore(webTask,     "Web",     STACK_WEB,    NULL, PRI_WEB,    NULL, 0);
  xTaskCreatePinnedToCore(fsmTask,     "FSM",     STACK_FSM,    NULL, PRI_FSM,    NULL, 1);
  xTaskCreatePinnedToCore(outputTask,  "Output",  2048,         NULL, 2,          NULL, 1);

  Serial.println("[SETUP] All tasks launched.");
}

// ---------------------------------------------------------------------------
// Loop — unused (all logic is in FreeRTOS tasks)
// ---------------------------------------------------------------------------
void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000)); // Keep watchdog happy
}
