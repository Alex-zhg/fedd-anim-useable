// claude generated

#include "webserver.h"
#include "config.h"
#include "fsm.h"
#include <WiFi.h>
#include <WebServer.h>

static WebServer server(WEB_PORT);

// ---------------------------------------------------------------------------
// HTML + JS for the live web interface
// Served as a single self-contained page.
// Uses JS fetch() to poll /telemetry endpoint every TELEMETRY_MS ms.
// ---------------------------------------------------------------------------
static const char INDEX_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Animatronic Gargoyle</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      background: #0a0a0a;
      color: #e0e0e0;
      font-family: 'Courier New', monospace;
      display: flex;
      flex-direction: column;
      align-items: center;
      min-height: 100vh;
      padding: 24px;
    }
    h1 {
      color: #cc3300;
      font-size: 1.6rem;
      letter-spacing: 3px;
      text-transform: uppercase;
      margin-bottom: 8px;
    }
    .subtitle { color: #555; font-size: 0.8rem; margin-bottom: 28px; }

    .status-bar {
      display: flex;
      gap: 24px;
      margin-bottom: 28px;
      flex-wrap: wrap;
      justify-content: center;
    }
    .stat {
      background: #161616;
      border: 1px solid #2a2a2a;
      border-radius: 8px;
      padding: 14px 22px;
      text-align: center;
      min-width: 120px;
    }
    .stat-label { font-size: 0.65rem; color: #666; text-transform: uppercase; letter-spacing: 2px; }
    .stat-value { font-size: 1.5rem; font-weight: bold; margin-top: 4px; }

    #state-value {
      padding: 4px 14px;
      border-radius: 4px;
      font-size: 1rem;
      font-weight: bold;
      letter-spacing: 2px;
    }
    .state-SCANNING { background: #001a33; color: #3399ff; border: 1px solid #3399ff; }
    .state-TRACKING { background: #1a0000; color: #ff3300; border: 1px solid #ff3300; }
    .state-LOST     { background: #1a0a00; color: #ff8800; border: 1px solid #ff8800; }
    .state-DANGER   { background: #1a0000; color: #ff0000; border: 1px solid #ff0000;
                      animation: dangerPulse 0.2s infinite alternate; }
    @keyframes dangerPulse { from { opacity: 1; } to { opacity: 0.4; } }

    /* Radar */
    .radar-container {
      position: relative;
      width: 340px;
      height: 180px;
      margin-top: 8px;
    }
    canvas#radar {
      width: 340px;
      height: 180px;
      border-radius: 0 0 170px 170px;
      background: #050f05;
      border: 1px solid #1a3a1a;
    }

    .log {
      margin-top: 24px;
      width: 100%;
      max-width: 500px;
      background: #0d0d0d;
      border: 1px solid #1e1e1e;
      border-radius: 6px;
      padding: 12px;
      font-size: 0.75rem;
      color: #33ff66;
      height: 120px;
      overflow-y: auto;
    }
  </style>
</head>
<body>
  <h1>&#x1F47E; Gargoyle</h1>
  <div class="subtitle">NCSU IEEE ESP32-S3 | Live Monitor</div>

  <div class="status-bar">
    <div class="stat">
      <div class="stat-label">State</div>
      <div class="stat-value"><span id="state-value" class="state-SCANNING">SCANNING</span></div>
    </div>
    <div class="stat">
      <div class="stat-label">Head Angle</div>
      <div class="stat-value"><span id="angle-value">90</span>°</div>
    </div>
    <div class="stat">
      <div class="stat-label">Distance</div>
      <div class="stat-value"><span id="dist-value">---</span><span style="font-size:0.8rem"> mm</span></div>
    </div>
  </div>

  <div class="radar-container">
    <canvas id="radar" width="340" height="180"></canvas>
  </div>

  <div class="log" id="log">Connecting to gargoyle...<br></div>

  <script>
    const canvas = document.getElementById('radar');
    const ctx    = canvas.getContext('2d');
    const W = canvas.width, H = canvas.height;
    const CX = W / 2, CY = H;
    const R  = H - 10;

    let currentAngle = 90;
    let currentDist  = -1;
    let currentState = 'SCANNING';
    const MAX_DIST_MM = 2000;

    function drawRadar(angle, distMM, state) {
      ctx.clearRect(0, 0, W, H);

      // Background arcs
      ctx.strokeStyle = '#1a3a1a';
      ctx.lineWidth   = 1;
      [0.25, 0.5, 0.75, 1.0].forEach(r => {
        ctx.beginPath();
        ctx.arc(CX, CY, R * r, Math.PI, 2 * Math.PI);
        ctx.stroke();
      });

      // Spoke lines every 30°
      for (let a = 0; a <= 180; a += 30) {
        const rad = (a * Math.PI) / 180;
        ctx.beginPath();
        ctx.moveTo(CX, CY);
        ctx.lineTo(CX + R * Math.cos(Math.PI - rad), CY - R * Math.sin(Math.PI - rad));
        ctx.strokeStyle = '#1a3a1a';
        ctx.stroke();
      }

      // Sweep line
      const sweepRad = ((180 - angle) * Math.PI) / 180;
      const sweepColor = state === 'DANGER' ? '#ff3300' :
                         state === 'TRACKING' ? '#ff6600' :
                         state === 'LOST' ? '#ff8800' : '#00ff44';
      ctx.beginPath();
      ctx.moveTo(CX, CY);
      ctx.lineTo(CX + R * Math.cos(sweepRad), CY - R * Math.sin(sweepRad));
      ctx.strokeStyle = sweepColor;
      ctx.lineWidth   = 2;
      ctx.stroke();

      // Object dot (if in range)
      if (distMM > 0 && distMM < MAX_DIST_MM) {
        const normDist = distMM / MAX_DIST_MM;
        const dotR     = R * (1 - normDist);
        const dotX     = CX + dotR * Math.cos(sweepRad);
        const dotY     = CY - dotR * Math.sin(sweepRad);
        ctx.beginPath();
        ctx.arc(dotX, dotY, 6, 0, 2 * Math.PI);
        ctx.fillStyle = state === 'DANGER' ? '#ff0000' : '#00ff88';
        ctx.fill();
        // Glow
        ctx.beginPath();
        ctx.arc(dotX, dotY, 10, 0, 2 * Math.PI);
        ctx.fillStyle = (state === 'DANGER' ? 'rgba(255,0,0,0.2)' : 'rgba(0,255,136,0.2)');
        ctx.fill();
      }

      // Angle label
      ctx.fillStyle = '#444';
      ctx.font      = '10px Courier New';
      ctx.fillText('0°', 4, H - 4);
      ctx.fillText('90°', W/2 - 8, 12);
      ctx.fillText('180°', W - 28, H - 4);
    }

    const logEl  = document.getElementById('log');
    const stateEl = document.getElementById('state-value');
    const angleEl = document.getElementById('angle-value');
    const distEl  = document.getElementById('dist-value');

    function addLog(msg) {
      logEl.innerHTML += msg + '<br>';
      logEl.scrollTop  = logEl.scrollHeight;
      // Keep log from growing too large
      const lines = logEl.innerHTML.split('<br>');
      if (lines.length > 40) {
        logEl.innerHTML = lines.slice(-30).join('<br>');
      }
    }

    async function poll() {
      try {
        const res  = await fetch('/telemetry');
        const data = await res.json();

        currentAngle = data.angle;
        currentDist  = data.dist;
        currentState = data.state;

        stateEl.textContent  = currentState;
        stateEl.className    = 'state-' + currentState;
        angleEl.textContent  = currentAngle.toFixed(1);
        distEl.textContent   = currentDist > 0 ? currentDist : '---';

        drawRadar(currentAngle, currentDist, currentState);
        addLog(`[${currentState}] ${currentAngle.toFixed(1)}° | ${currentDist > 0 ? currentDist + 'mm' : 'no object'}`);
      } catch (e) {
        addLog('Connection lost...');
      }
    }

    // Initial draw
    drawRadar(90, -1, 'SCANNING');

    // Poll server
    setInterval(poll, )rawhtml" + String(TELEMETRY_MS) + R"rawhtml();
  </script>
</body>
</html>
)rawhtml";

// ---------------------------------------------------------------------------
// Route handlers
// ---------------------------------------------------------------------------
static void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

static void handleTelemetry() {
  FSMState s;
  float    angle;
  int      dist;

  if (xSemaphoreTake(telemetryMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
    s     = telemetry.state;
    angle = telemetry.headAngle;
    dist  = telemetry.distanceMM;
    xSemaphoreGive(telemetryMutex);
  } else {
    server.send(503, "application/json", "{\"error\":\"busy\"}");
    return;
  }

  String json = "{\"state\":\"";
  json += stateName(s);
  json += "\",\"angle\":";
  json += String(angle, 1);
  json += ",\"dist\":";
  json += String(dist);
  json += "}";

  server.send(200, "application/json", json);
}

static void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

// ---------------------------------------------------------------------------
// Web Task
// ---------------------------------------------------------------------------
void webTask(void* pvParameters) {
  Serial.printf("[WIFI] Connecting to %s ...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    vTaskDelay(pdMS_TO_TICKS(500));
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WIFI] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("[WIFI] Open http://%s in a browser on the NCSU network.\n",
                  WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n[WIFI] Failed to connect. Web interface unavailable.");
    vTaskDelete(NULL);
    return;
  }

  server.on("/",          handleRoot);
  server.on("/telemetry", handleTelemetry);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("[WEB] Server started.");

  while (true) {
    server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(2)); // Yield frequently
  }
}
