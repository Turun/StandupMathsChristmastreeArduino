/*
  ESP32 LEDStrip AP + Webserver + Adafruit NeoPixel control
  - AP: SSID "LEDStrip", PW "LEDStrip"
  - HTTP server on port 8080
  - Endpoints:
      GET  /                     -> index.html
      GET  /static/style.css
      GET  /static/script/main.js
      GET  /static/script/ui.js
      GET  /static/script/merge_directions.js
      GET  /static/script/capture_unidirectional.js
      POST /configure_leds       -> JSON dict string->bool, e.g. {"0":true,"1":false}
      POST /set_led_positions    -> JSON positions saved to nonvolatile storage
      GET  /get_saved_led_positions -> returns saved JSON positions (if any)
*/

#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <Preferences.h>

#define LED_BUILTIN 2
#define STRIP_PIN       13
#define STRIP_NUMPIXELS 100

Adafruit_NeoPixel pixels(STRIP_NUMPIXELS, STRIP_PIN, NEO_GRB + NEO_KHZ800);

WebServer server(80);
Preferences prefs;

// Keep boolean state of LEDs
bool ledState[STRIP_NUMPIXELS];

// -------------------- Static files (served by the ESP) --------------------
// These are minimal functional implementations so the web UI in your Flask logs works.
// You can replace these strings with the content of your actual files if desired.

const char index_html[] PROGMEM = R"rawliteral(
<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <title>LED Strip Control</title>
  <link rel="stylesheet" href="/static/style.css">
</head>
<body>
  <h1>LED Strip Control</h1>
  <div id="led-grid"></div>
  <button id="save-positions">Save positions (dummy)</button>
  <script src="/static/script/ui.js"></script>
  <script src="/static/script/main.js"></script>
  <script src="/static/script/merge_directions.js"></script>
  <script src="/static/script/capture_unidirectional.js"></script>
</body>
</html>
)rawliteral";

const char style_css[] PROGMEM = R"rawliteral(
body { font-family: Arial, sans-serif; margin: 20px; }
#led-grid { display: grid; grid-template-columns: repeat(8, 40px); gap: 8px; max-width: 360px; }
.led-btn { width: 40px; height: 40px; border-radius: 6px; border: 1px solid #444; cursor: pointer; }
.led-on { background: #00aaff; }
.led-off { background: #333; color: #fff; }
)rawliteral";

const char main_js[] PROGMEM = R"rawliteral(
// main.js - sets up the UI for 16 LEDs and posts states to /configure_leds
document.addEventListener('DOMContentLoaded', function () {
  const N = 16;
  const grid = document.getElementById('led-grid');
  const state = new Array(N).fill(false);

  function render() {
    grid.innerHTML = '';
    for (let i=0;i<N;i++) {
      const btn = document.createElement('button');
      btn.className = 'led-btn ' + (state[i] ? 'led-on' : 'led-off');
      btn.textContent = i;
      btn.addEventListener('click', () => {
        state[i] = !state[i];
        sendState();
        render();
      });
      grid.appendChild(btn);
    }
  }

  async function sendState() {
    // convert to dict with string keys as in your Flask code
    const obj = {};
    for (let i=0;i<N;i++) obj[String(i)] = state[i];
    try {
      await fetch('/configure_leds', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify(obj)
      });
    } catch (e) {
      console.error('Failed to send state', e);
    }
  }

  // optionally fetch saved LED positions (not required), left as a placeholder
  document.getElementById('save-positions').addEventListener('click', async () => {
    const positions = {}; // fill with dummy or captured positions in real UI
    positions["0"] = [0.0, 0.0, 0.0];
    try {
      await fetch('/set_led_positions', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify(positions)
      });
      alert('Positions saved');
    } catch (e) {
      alert('Failed to save positions');
    }
  });

  render();
});
)rawliteral";

const char ui_js[] PROGMEM = R"rawliteral(
// ui.js - small helper file (placeholder)
console.log('ui.js loaded');
)rawliteral";

const char merge_directions_js[] PROGMEM = R"rawliteral(
// merge_directions.js - placeholder implementation
console.log('merge_directions.js loaded');
)rawliteral";

const char capture_unidirectional_js[] PROGMEM = R"rawliteral(
// capture_unidirectional.js - placeholder implementation
console.log('capture_unidirectional.js loaded');
)rawliteral";

// -------------------------------------------------------------------------

// helper: safely get raw POST body
String getRequestBody() {
  if (server.hasArg("plain")) {
    return server.arg("plain"); // WebServer stores raw body in "plain"
  }
  return String();
}

// LED control helpers
void redrawPixels() {
  for (int i = 0; i < STRIP_NUMPIXELS; ++i) {
    if (ledState[i]) {
      pixels.setPixelColor(i, pixels.Color(100, 100, 100));
    } else {
      pixels.setPixelColor(i, pixels.Color(0, 0, 0));
    }
  }
  pixels.show();
}

// Convert current ledState to JSON string (used for storing)
String ledStateToJson() {
  StaticJsonDocument<256> doc;
  for (int i = 0; i < STRIP_NUMPIXELS; i++) {
    doc[String(i)] = ledState[i];
  }
  String out;
  serializeJson(doc, out);
  return out;
}

// Handlers
void handleRoot() {
  server.send_P(200, "text/html", index_html);
}

void handleStyle() {
  server.send_P(200, "text/css", style_css);
}

void handleMainJs() {
  server.send_P(200, "application/javascript", main_js);
}
void handleUiJs() {
  server.send_P(200, "application/javascript", ui_js);
}
void handleMergeDirectionsJs() {
  server.send_P(200, "application/javascript", merge_directions_js);
}
void handleCaptureUnidirectionalJs() {
  server.send_P(200, "application/javascript", capture_unidirectional_js);
}

void handleConfigureLEDs() {
  String body = getRequestBody();
  if (body.length() == 0) {
    server.send(400, "text/plain", "no body");
    return;
  }

  // parse JSON of format {"0": true, "1": false, ...}
  StaticJsonDocument<1024> doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    server.send(400, "text/plain", String("JSON parse error: ") + err.c_str());
    return;
  }

  // Update ledState for any keys in the JSON
  for (JsonPair kv : doc.as<JsonObject>()) {
    const char* key = kv.key().c_str();
    bool val = kv.value().as<bool>();
    int idx = atoi(key);
    if (idx >= 0 && idx < STRIP_NUMPIXELS) {
      ledState[idx] = val;
    }
  }

  // Redraw
  redrawPixels();

  // // ensure LEDs are lit briefly
  // delay(500);

  server.send(200, "text/plain", "success");
}

void handleSetLedPositions() {
  String body = getRequestBody();
  if (body.length() == 0) {
    server.send(400, "text/plain", "no body");
    return;
  }

  // validate JSON at least parses, but we just store the raw JSON string into Preferences
  DynamicJsonDocument doc(8192); // allow for larger payloads
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    server.send(400, "text/plain", String("JSON parse error: ") + err.c_str());
    return;
  }

  // Save the positions string to non-volatile prefs
  prefs.putString("led_positions", body);

  server.send(200, "text/plain", "success");
}

void handleGetSavedLedPositions() {
  String s = prefs.getString("led_positions", "");
  if (s.length() == 0) {
    server.send(204, "application/json", ""); // no content
  } else {
    server.send(200, "application/json", s);
  }
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // Initialize NeoPixel
  pixels.begin();
  pixels.clear();
  pixels.show();

  // initialize led state (all off)
  for (int i = 0; i < STRIP_NUMPIXELS; ++i) {
    ledState[i] = false;
  }

  // Preferences (non-volatile storage)
  prefs.begin("ledcfg", false);

  // Setup WiFi access point (SSID: LEDStrip, PW: LEDStrip)
  const char* ssid = "LEDStrip";
  const char* password = "LEDStrip";
  WiFi.softAP(ssid, password);
  IPAddress apIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(apIP);

  // Setup web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/static/style.css", HTTP_GET, handleStyle);
  server.on("/static/script/main.js", HTTP_GET, handleMainJs);
  server.on("/static/script/ui.js", HTTP_GET, handleUiJs);
  server.on("/static/script/merge_directions.js", HTTP_GET, handleMergeDirectionsJs);
  server.on("/static/script/capture_unidirectional.js", HTTP_GET, handleCaptureUnidirectionalJs);

  // API endpoints
  server.on("/configure_leds", HTTP_POST, handleConfigureLEDs);
  server.on("/set_led_positions", HTTP_POST, handleSetLedPositions);
  server.on("/get_saved_led_positions", HTTP_GET, handleGetSavedLedPositions);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started on port 80");
}

void loop() {
  server.handleClient();
}
