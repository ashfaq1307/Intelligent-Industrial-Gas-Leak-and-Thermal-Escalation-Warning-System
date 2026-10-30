
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LiquidCrystal_I2C.h>

// Enter your Wi-Fi credentials locally.
// Do not share your Wi-Fi password publicly.
const char* WIFI_SSID = "Vivo Y21 5g";
const char* WIFI_PASSWORD = "12345678";

// Hardware pins
const int MQ2_PIN = 32;
const int LM35_PIN = 33;
const int SDA_PIN = 21;
const int SCL_PIN = 22;
const int BUZZER_PIN = 18;

const int GREEN_LED_PIN = 25;
const int YELLOW_LED_PIN = 26;
const int RED_LED_PIN = 27;

// Physical buzzer is still disconnected
const bool BUZZER_CONNECTED = true;

LiquidCrystal_I2C lcd(0x27, 16, 2);
WebServer server(80);

// Demonstration thresholds, not certified safety limits
const int GAS_ELEVATED_ADC = 1000;
const int GAS_CRITICAL_ADC = 1800;
const float ELEVATED_TEMP_C = 35.0;
const float CRITICAL_TEMP_C = 50.0;

const unsigned long SAMPLE_INTERVAL_MS = 2000;
unsigned long previousMillis = 0;

enum HazardLevel { NORMAL, ELEVATED, CRITICAL };

int gasADC = 0;
float temperatureC = 0.0;
HazardLevel hazardLevel = NORMAL;
bool readingsAvailable = false;

// ---------------- Dashboard HTML ----------------
const char INDEX_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Industrial Safety Monitor</title>
<style>
*{box-sizing:border-box}
body{margin:0;background:#0b1220;color:#e8eef8;font-family:Arial,sans-serif}
header{padding:22px;background:#152238;border-bottom:1px solid #334155}
main{max-width:1050px;margin:auto;padding:16px}
h1{font-size:24px;margin:0 0 7px}
h2{font-size:17px;margin:0 0 18px}
.muted{color:#aab8cc;font-size:13px}
#connection{display:inline-block;margin-top:12px;padding:7px 12px;
border-radius:20px;background:#334155;font-size:13px}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));
gap:14px;margin:16px 0}
.card{background:#17253a;border:1px solid #34435b;border-radius:13px;
padding:18px;min-width:0}
.value{font-size:29px;font-weight:bold;margin:13px 0}
.normal{color:#4ade80}
.elevated{color:#facc15}
.critical{color:#fb7185}
#hazardBanner{padding:15px;border-radius:10px;background:#243449;
border-left:5px solid #94a3b8;margin-top:18px}
#hazard{font-size:23px;font-weight:bold;margin:6px 0}
.ledrow{display:flex;align-items:center;gap:9px;margin:16px 0}
.dot{width:13px;height:13px;border-radius:50%;background:#64748b;flex:none}
.dot.on{background:#4ade80;box-shadow:0 0 9px #4ade80}
canvas{display:block;width:100%;height:210px}
footer{font-size:12px;color:#9aabc1;line-height:1.7;padding:16px 0}
</style>
</head>
<body>
<header>
  <h1>Industrial Safety Monitor</h1>
  <div class="muted">Gas Leak &amp; Thermal Escalation Warning System</div>
  <span id="connection">Connecting to ESP32...</span>
</header>
<main>
  <div id="hazardBanner">
    <div class="muted">CURRENT HAZARD LEVEL</div>
    <div id="hazard">WAITING FOR DATA</div>
    <div id="hazardNote" class="muted">Receiving first sensor sample...</div>
  </div>

  <div class="grid">
    <section class="card">
      <div class="muted">MQ-2 Gas Sensor</div>
      <div id="gas" class="value">--</div>
      <div class="muted">ADC reading (0–4095), not ppm</div>
    </section>
    <section class="card">
      <div class="muted">LM35 Temperature</div>
      <div id="temp" class="value">-- °C</div>
      <div class="muted">Temperature reading</div>
    </section>
    <section class="card">
      <div class="muted">Buzzer Command</div>
      <div id="buzzer" class="value">--</div>
      <div id="buzzerNote" class="muted">Physical buzzer disconnected</div>
    </section>
  </div>

  <div class="grid">
    <section class="card">
      <h2>Warning LED States</h2>
      <div class="ledrow"><span id="greenDot" class="dot"></span>
        Green LED: <b id="green">--</b></div>
      <div class="ledrow"><span id="yellowDot" class="dot"></span>
        Yellow LED: <b id="yellow">--</b></div>
      <div class="ledrow"><span id="redDot" class="dot"></span>
        Red LED: <b id="red">--</b></div>
    </section>
    <section class="card">
      <h2>Connection Information</h2>
      <p>ESP32: <b id="esp">Connecting</b></p>
      <p>Sample interval: <b>2 seconds</b></p>
      <p>Last update: <b id="updated">--</b></p>
      <p class="muted">Charts show the last 60 browser samples.</p>
    </section>
  </div>

  <div class="grid">
    <section class="card">
      <h2>Gas Reading Trend</h2>
      <canvas id="gasChart"></canvas>
      <div class="muted">MQ-2 ADC values</div>
    </section>
    <section class="card">
      <h2>Temperature Trend</h2>
      <canvas id="tempChart"></canvas>
      <div class="muted">Degrees Celsius</div>
    </section>
  </div>

  <footer>
    Educational prototype only. Sensor thresholds require proper calibration.
    MQ-2 ADC readings are not gas concentration in ppm. The buzzer command
    does not confirm a physical alarm. Do not use this prototype as a
    certified industrial safety system.
  </footer>
</main>

<script>
const gasHistory = [];
const tempHistory = [];
const MAX_POINTS = 60;
let lastSampleKey = '';

function drawChart(id, values, unit) {
  const canvas = document.getElementById(id);
  const width = Math.max(220, canvas.clientWidth);
  const height = 210;
  const dpr = window.devicePixelRatio || 1;

  canvas.width = Math.round(width * dpr);
  canvas.height = Math.round(height * dpr);

  const ctx = canvas.getContext('2d');
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  ctx.clearRect(0, 0, width, height);

  const left = 42, right = 12, top = 18, bottom = 28;
  const plotW = width - left - right;
  const plotH = height - top - bottom;

  if (plotW <= 0 || plotH <= 0) return;

  let min = 0, max = 100;
  if (values.length) {
    min = Math.min(...values);
    max = Math.max(...values);
    if (max === min) {
      const padding = Math.max(Math.abs(max) * 0.1, 1);
      min -= padding;
      max += padding;
    } else {
      const padding = (max - min) * 0.15;
      min -= padding;
      max += padding;
    }
    if (id === 'gasChart') min = Math.max(0, min);
  }

  ctx.font = '11px Arial';
  ctx.fillStyle = '#aab8cc';
  ctx.strokeStyle = '#34435b';
  ctx.lineWidth = 1;

  for (let i = 0; i <= 4; i++) {
    const y = top + plotH * i / 4;
    const val = max - (max - min) * i / 4;
    ctx.beginPath();
    ctx.moveTo(left, y);
    ctx.lineTo(width - right, y);
    ctx.stroke();
    ctx.fillText(val.toFixed(unit === '°C' ? 1 : 0), 2, y + 4);
  }

  if (!values.length) {
    ctx.fillText('Waiting for samples...', left + 5, top + plotH / 2);
    return;
  }

  ctx.strokeStyle = '#60a5fa';
  ctx.lineWidth = 2;
  ctx.beginPath();

  values.forEach((value, index) => {
    const x = left + (values.length === 1 ? plotW / 2 :
      index * plotW / (MAX_POINTS - 1));
    const y = top + plotH - ((value - min) / (max - min)) * plotH;
    if (index === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  });
  ctx.stroke();

  const last = values[values.length - 1];
  const lastX = left + (values.length === 1 ? plotW / 2 :
    (values.length - 1) * plotW / (MAX_POINTS - 1));
  const lastY = top + plotH - ((last - min) / (max - min)) * plotH;

  ctx.fillStyle = '#60a5fa';
  ctx.beginPath();
  ctx.arc(lastX, lastY, 3.5, 0, Math.PI * 2);
  ctx.fill();
}

function updateLED(name, isOn) {
  document.getElementById(name).textContent = isOn ? 'ON' : 'OFF';
  document.getElementById(name + 'Dot').className =
    'dot' + (isOn ? ' on' : '');
}

async function refresh() {
  try {
    const response = await fetch('/api', {cache:'no-store'});
    if (!response.ok) throw new Error('HTTP ' + response.status);
    const d = await response.json();

    document.getElementById('connection').textContent = 'ESP32 ONLINE';
    document.getElementById('esp').textContent = 'Online';
    document.getElementById('esp').className = 'normal';

    if (!d.readingsAvailable) return;

    document.getElementById('gas').textContent = d.gasADC;
    document.getElementById('temp').textContent =
      Number(d.temperatureC).toFixed(1) + ' °C';

    const hazard = document.getElementById('hazard');
    hazard.textContent = d.hazard;
    hazard.className = d.hazard.toLowerCase();

    const banner = document.getElementById('hazardBanner');
    const styles = {
      NORMAL: ['#163a2b', '#4ade80', 'No configured threshold exceeded'],
      ELEVATED: ['#40371a', '#facc15', 'Elevated warning threshold reached'],
      CRITICAL: ['#451f2a', '#fb7185', 'Critical warning threshold reached']
    };
    const style = styles[d.hazard] || styles.NORMAL;
    banner.style.background = style[0];
    banner.style.borderLeftColor = style[1];
    document.getElementById('hazardNote').textContent = style[2];

    document.getElementById('buzzer').textContent =
      d.buzzerCommand ? 'ALARM REQUESTED' : 'OFF';
    document.getElementById('buzzer').className =
      'value ' + (d.buzzerCommand ? 'critical' : 'normal');
    document.getElementById('buzzerNote').textContent =
      d.buzzerConnected ? 'Hardware configured as connected' :
      'Physical buzzer disconnected; no sound confirmed';

    updateLED('green', d.leds.green);
    updateLED('yellow', d.leds.yellow);
    updateLED('red', d.leds.red);

    // Add a chart point only once for each new ESP32 sample.
    const sampleKey = d.sampleId;
    if (sampleKey !== lastSampleKey) {
      lastSampleKey = sampleKey;
      gasHistory.push(Number(d.gasADC));
      tempHistory.push(Number(d.temperatureC));
      if (gasHistory.length > MAX_POINTS) gasHistory.shift();
      if (tempHistory.length > MAX_POINTS) tempHistory.shift();
      drawChart('gasChart', gasHistory, '');
      drawChart('tempChart', tempHistory, '°C');
    }

    document.getElementById('updated').textContent =
      new Date().toLocaleTimeString();
  } catch (error) {
    document.getElementById('connection').textContent =
      'CONNECTION LOST';
    document.getElementById('esp').textContent = 'Offline / unreachable';
    document.getElementById('esp').className = 'critical';
  }
}

window.addEventListener('resize', () => {
  drawChart('gasChart', gasHistory, '');
  drawChart('tempChart', tempHistory, '°C');
});

refresh();
setInterval(refresh, 2000);
</script>
</body>
</html>
)HTML";

// ---------------- Sensor and server logic ----------------

const char* hazardName(HazardLevel level) {
    switch (level) {
        case NORMAL: return "NORMAL";
        case ELEVATED: return "ELEVATED";
        case CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

void setOutputs(HazardLevel level) {
    digitalWrite(GREEN_LED_PIN, level == NORMAL ? HIGH : LOW);
    digitalWrite(YELLOW_LED_PIN, level == ELEVATED ? HIGH : LOW);
    digitalWrite(RED_LED_PIN, level == CRITICAL ? HIGH : LOW);
    digitalWrite(BUZZER_PIN, level == CRITICAL ? HIGH : LOW);
}

void updateLCD() {
    char line1[17];
    char line2[17];

    snprintf(line1, sizeof(line1), "Gas ADC: %d", gasADC);
    snprintf(line2, sizeof(line2), "Temp: %.1f C", temperatureC);

    lcd.setCursor(0, 0);
    lcd.print("                ");
    lcd.setCursor(0, 0);
    lcd.print(line1);

    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print(line2);
}

void handleRoot() {
    server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

void handleAPI() {
    String json;
    json.reserve(280);

    json = "{";
    json += "\"gasADC\":" + String(gasADC) + ",";
    json += "\"temperatureC\":" + String(temperatureC, 1) + ",";
    json += "\"hazard\":\"" + String(hazardName(hazardLevel)) + "\",";
    json += "\"readingsAvailable\":";
    json += readingsAvailable ? "true," : "false,";
    json += "\"sampleId\":" + String(previousMillis) + ",";
    json += "\"buzzerCommand\":";
    json += (hazardLevel == CRITICAL) ? "true," : "false,";
    json += "\"buzzerConnected\":";
    json += BUZZER_CONNECTED ? "true," : "false,";
    json += "\"leds\":{";
    json += "\"green\":";
    json += (hazardLevel == NORMAL) ? "true," : "false,";
    json += "\"yellow\":";
    json += (hazardLevel == ELEVATED) ? "true," : "false,";
    json += "\"red\":";
    json += (hazardLevel == CRITICAL) ? "true" : "false";
    json += "}}";

    server.send(200, "application/json", json);
}

void setup() {
    Serial.begin(115200);
    delay(300);

    pinMode(MQ2_PIN, INPUT);
    pinMode(LM35_PIN, INPUT);

    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(YELLOW_LED_PIN, OUTPUT);
    pinMode(RED_LED_PIN, OUTPUT);
    setOutputs(NORMAL);

    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    analogReadResolution(12);
    analogSetPinAttenuation(MQ2_PIN, ADC_11db);
    analogSetPinAttenuation(LM35_PIN, ADC_11db);

    Wire.begin(SDA_PIN, SCL_PIN);
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Safety System");
    lcd.setCursor(0, 1);
    lcd.print("Connecting WiFi");

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.println();
    Serial.println("Industrial Safety Dashboard");

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - start < 15000) {
        delay(250);
    }

    if (WiFi.status() == WL_CONNECTED) {
        server.on("/", HTTP_GET, handleRoot);
        server.on("/api", HTTP_GET, handleAPI);
        server.begin();

        Serial.print("Dashboard: http://");
        Serial.println(WiFi.localIP());

        lcd.clear();
        lcd.print("WiFi Connected");
        lcd.setCursor(0, 1);
        lcd.print("Dashboard Ready");
    } else {
        Serial.println("Wi-Fi unavailable; dashboard not accessible.");
        lcd.clear();
        lcd.print("WiFi Unavailable");
    }
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        server.handleClient();
    }

    const unsigned long now = millis();
    if (now - previousMillis < SAMPLE_INTERVAL_MS) {
        return;
    }
    previousMillis = now;

    gasADC = analogRead(MQ2_PIN);
    const uint32_t milliVolts = analogReadMilliVolts(LM35_PIN);
    temperatureC = milliVolts / 10.0f;

    if (gasADC >= GAS_CRITICAL_ADC ||
        temperatureC >= CRITICAL_TEMP_C) {
        hazardLevel = CRITICAL;
    } else if (gasADC >= GAS_ELEVATED_ADC ||
               temperatureC >= ELEVATED_TEMP_C) {
        hazardLevel = ELEVATED;
    } else {
        hazardLevel = NORMAL;
    }

    readingsAvailable = true;
    setOutputs(hazardLevel);
    updateLCD();

    Serial.print("Gas ADC: ");
    Serial.print(gasADC);
    Serial.print(" | Temperature: ");
    Serial.print(temperatureC, 1);
    Serial.print(" C | Status: ");
    Serial.println(hazardName(hazardLevel));
}
