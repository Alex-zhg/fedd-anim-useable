#include <Arduino.h>
#include <Servo.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include "Controls.h"
#include <WiFi.h>
#include <WebServer.h>

#define SDA	P23
#define SCL	P24
#define SHUTDOWN P25
// #define INT P22
#define ADDR 0x30
#define DETECT_RANGE 500
#define BZ P39
#define BEEP_TIME_IDLE 250
#define BEEP_TIME_DANGER 250 / 3
#define DANGER_FREQ 880
#define IDLE_FREQ 1320

Adafruit_VL53L0X tof = Adafruit_VL53L0X();
TwoWire bus = TwoWire(1);
// bool detected;
// VL53L0X_RangingMeasurementData_t measure;
// int tofReading;

bool checkInRange(int, int, int);
void stopIdle(bool);
void turnHeadRight();
void turnHeadLeft();
void readTof(int);

void idle(void *);
void reading(void *);

TaskHandle_t Idle;
TaskHandle_t Read;

int SensorDistance;
int currentNeckAngle;

const char* WIFI_SSID = "ncsu";
const char* WIFI_PASS = "";

WebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<title>Robot Radar</title>
<style>
  body { background: #111; color: #fff; display: flex; justify-content: center; margin-top: 50px; font-family: monospace; }
  .radar-wrap { background: #000; display: flex; flex-direction: column; align-items: center; padding: 1rem; gap: 0.75rem; border-radius: 8px; border: 1px solid #333;}
  .readout { font-size: 13px; display: flex; gap: 24px; }
  .readout span { display: flex; flex-direction: column; align-items: center; gap: 2px; }
  .readout .label { color: #555; font-size: 11px; text-transform: uppercase; letter-spacing: 1px; }
  .green { color: #0f0; }
  .red { color: #f00; }
</style>
</head>
<body>
<div class="radar-wrap">
  <canvas id="radarCanvas" width="400" height="220"></canvas>
  <div class="readout">
    <span>
      <span class="label">Angle</span>
      <span class="value green" id="outAngle">—</span>
    </span>
    <span>
      <span class="label">Distance</span>
      <span class="value green" id="outDist">—</span>
    </span>
    <span>
      <span class="label">Mode</span>
      <span class="value" id="outMode">—</span>
    </span>
  </div>
</div>
<script>
const canvas = document.getElementById("radarCanvas");
const ctx = canvas.getContext("2d");
const cx = 200, cy = 210, R = 200;
const MAX_DIST = 1000; 
const DETECT_THRESH = 500;

function drawRadar(angle, distance) {

  ctx.fillStyle = "#000";
  ctx.fillRect(0, 0, canvas.width, canvas.height);

  for (let i = 1; i <= 4; i++) {
    ctx.beginPath();
    ctx.arc(cx, cy, R * i / 4, Math.PI, 0);
    ctx.strokeStyle = "#0f0";
    ctx.lineWidth = 0.5;
    ctx.stroke();
  }

  ctx.beginPath();
  ctx.moveTo(cx - R, cy);
  ctx.lineTo(cx + R, cy);
  ctx.strokeStyle = "#0f0";
  ctx.lineWidth = 0.5;
  ctx.stroke();

  const sa = Math.PI - (angle * Math.PI / 180);
  ctx.beginPath();
  ctx.moveTo(cx, cy);
  ctx.lineTo(cx + Math.cos(sa) * R, cy - Math.sin(sa) * R);
  ctx.strokeStyle = "#0f0";
  ctx.lineWidth = 1.5;
  ctx.stroke();

  let blipDistRatio = distance / MAX_DIST;
  if(blipDistRatio > 1) blipDistRatio = 1;
  
  const bx = cx + Math.cos(sa) * blipDistRatio * R;
  const by = cy - Math.sin(sa) * blipDistRatio * R;

  const isClose = distance <= DETECT_THRESH;
  ctx.beginPath();
  ctx.arc(bx, by, 6, 0, Math.PI * 2);
  ctx.fillStyle = isClose ? "#f00" : "#0f0";
  ctx.fill();

  const colorClass = isClose ? "red" : "green";
  
  document.getElementById("outAngle").textContent = angle + "°";
  document.getElementById("outAngle").className = "value " + colorClass;

  document.getElementById("outDist").textContent = distance + "mm";
  document.getElementById("outDist").className = "value " + colorClass;

  const modeEl = document.getElementById("outMode");
  modeEl.textContent = isClose ? "DETECTED" : "SCANNING";
  modeEl.className = "value " + colorClass;
}

setInterval(() => {
  fetch('/data')
    .then(res => res.json())
    .then(data => {
      drawRadar(data.angle, data.distance);
    })
    .catch(err => console.log("Fetch error:", err));
}, 100);
</script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleData() {
  String json = "{";
  json += "\"angle\":" + String(currentNeckAngle) + ",";
  json += "\"distance\":" + String(SensorDistance);
  json += "}";
  server.send(200, "application/json", json);
}

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

	WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nIP:");
    Serial.println(WiFi.localIP());

	server.on("/", handleRoot);
    server.on("/data", handleData);
    server.begin();

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

	digitalWrite(BLUE_LED_PIN, LOW);
	digitalWrite(RED_LED_PIN, LOW);
}

void loop()
{
	server.handleClient();
    vTaskDelay(10 / portTICK_PERIOD_MS);
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
	digitalWrite(BLUE_LED_PIN, HIGH);
	digitalWrite(RED_LED_PIN, LOW);

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
			/*
			tone(BZ, DANGER_FREQ, BEEP_TIME_DANGER);
			tone(BZ, DANGER_FREQ, BEEP_TIME_DANGER);
			tone(BZ, DANGER_FREQ, BEEP_TIME_DANGER);
			*/
		}
		else 
		{
			digitalWrite(RED_LED_PIN, LOW);
            digitalWrite(BLUE_LED_PIN, HIGH);
			// tone(BZ, IDLE_FREQ, BEEP_TIME_IDLE);
		}
		// vTaskDelay(50 / portTICK_PERIOD_MS);
	}
}