/*
  ESP32 LEDStrip AP + Webserver + Adafruit NeoPixel control
  - AP: SSID "LEDStrip", PW "LEDStrip"
  - HTTP server on port 8080
*/

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <Preferences.h>
#include <Arduino.h>
#include <math.h>

#include <ArduinoJson.h>

// ---- Include file-content headers here ----
#include "headers/templates/index_html.h"
#include "headers/static/style_css.h"
#include "headers/static/script/main_js.h"
#include "headers/static/script/ui_js.h"
#include "headers/static/script/merge_directions_js.h"
#include "headers/static/script/capture_unidirectional_js.h"
#include "headers/static/script/effects_js.h"
// Each header must define a PROGMEM const char array, e.g.:
//   const char index_html[] PROGMEM = R"rawliteral(...your html...)rawliteral";



#define LED_BUILTIN_PIN 2
#define STRIP_NUMPIXELS_DEFAULT 500

Preferences prefs;

Adafruit_NeoPixel *pixels = nullptr;
uint16_t numPixels = STRIP_NUMPIXELS_DEFAULT;
uint8_t STRIP_PIN_DEFAULT = 13;

// TODO: is this persisted across reboots? it should
bool* ledMask = nullptr; // dynamic array sized to numPixels, for turning off individual leds.

// Global base color (default 50, 50, 50)
uint8_t baseR = 50;
uint8_t baseG = 50;
uint8_t baseB = 50;


// save which effect is currently running
enum EffectType {
  EFFECT_NONE = 0,
  EFFECT_BLINK,
  EFFECT_ALL_ON,
  EFFECT_SWEEPING_PLANE,
  EFFECT_SWEEPING_PLANE_X,
  EFFECT_SWEEPING_PLANE_Y,
  EFFECT_SWEEPING_PLANE_Z,
};
EffectType currentEffect = EFFECT_NONE;
unsigned long effectStartTimeMs = 0;

// global data for sweeping planes
float* effect_sweeping_plane_zposs = nullptr;
float effect_sweeping_plane_hue = 0.0;


// Global array to store LED positions
struct Point {
  float x, y, z;
};
Point* ledPositions = nullptr;

WebServer server(80);

// ------------------------ Helpers -------------------------
String getRequestBody() {
  if (server.hasArg("plain")) {
    return server.arg("plain");
  }
  return String();
}

void allocateStrip(uint16_t newNum) {
  // Free previous LED state and positions
  if (pixels) {
    pixels->clear();
    pixels->show();
    delete pixels;
    pixels = nullptr;
  }
  if (ledMask) {
    // TODO: persist ledMask to new allocation
    /*
    like we had in `set num LEDs` code:
    ```
    bool *oldMask = ledMask;
    uint16_t oldNum = numPixels;
    allocateStrip((uint16_t)newNum);
    // copy old values
    uint16_t minN = (oldNum < numPixels) ? oldNum : numPixels;
    for (uint16_t i = 0; i < minN; ++i) {
      ledMask[i] = oldMask[i];
    }
    ```
    */
    delete[] ledMask;
    ledMask = nullptr;
  }
  if (ledPositions) {
    delete[] ledPositions;
    ledPositions = nullptr;
  }
  if (effect_sweeping_plane_zposs) {
    delete[] effect_sweeping_plane_zposs;
    effect_sweeping_plane_zposs = nullptr;
  }

  numPixels = newNum;
  
  // Allocate new state
  ledMask = new bool[numPixels];
  for (uint16_t i = 0; i < numPixels; ++i) {
    ledMask[i] = false;
  }

  // Allocate new positions array
  ledPositions = new Point[numPixels];

  effect_sweeping_plane_zposs = new float[numPixels];
  
  // Instantiate new NeoPixel
  pixels = new Adafruit_NeoPixel(numPixels, STRIP_PIN_DEFAULT, NEO_GRB + NEO_KHZ800);
  pixels->begin();
  pixels->clear();
  pixels->show();
}

void setPixelColor(uint16_t index, uint8_t r, uint8_t g, uint8_t b) {
  if (!pixels) return;
  if (index >= numPixels) return;
  if (ledMask[index]) {
    pixels->setPixelColor(index, pixels->Color(r, g, b));
  } else {
    pixels->setPixelColor(index, pixels->Color(0, 0, 0));
  }
  // pixels->show();
}

// pixels->show(), but guarded with a nullcheck
void showPixelColors(){
  if (!pixels){
    return;
  }
  pixels->show();
}

// set all unmasked pixels to the base color and show the result
void redrawPixels() {
  if (!pixels) return;
  for (uint16_t i = 0; i < numPixels; ++i) {
    if (ledMask[i]) {
      pixels->setPixelColor(i, pixels->Color(baseR, baseG, baseB));
    } else {
      pixels->setPixelColor(i, pixels->Color(0, 0, 0));
    }
  }
  pixels->show();
}

String ledMaskToJsonString() {
  JsonDocument doc;
  for (uint16_t i = 0; i < numPixels; ++i) {
    doc[String(i)] = ledMask[i];
  }
  String out;
  serializeJson(doc, out);
  return out;
}

void loadLedPositionsFromStorage() {
  String savedJson = prefs.getString("led_positions", "");  // Get the JSON from NV storage
  if (savedJson.length() == 0) {
    Serial.println("No LED positions found in storage.");
    return;
  }

  // Parse the JSON string to extract LED positions
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, savedJson);
  if (err) {
    Serial.println("Error parsing saved LED positions JSON.");
    return;
  }

  JsonObject obj = doc.as<JsonObject>();
  uint16_t newNum = obj.size();
  
  // Ensure that the number of LEDs in the JSON matches the number of LEDs you expect
  if (newNum != numPixels) {
    prefs.putUInt("num_leds", newNum);
    allocateStrip(newNum);
  }
  numPixels = newNum;

  // Deserialize and populate the ledPositions array
  for (JsonPair kv : obj) {
    int idx = atoi(kv.key().c_str());
    if (idx >= 0 && idx < numPixels) {
      JsonArray pos = kv.value().as<JsonArray>();
      ledPositions[idx].x = pos[0].as<float>();
      ledPositions[idx].y = pos[1].as<float>();
      ledPositions[idx].z = pos[2].as<float>();
    }
  }

  Serial.println("LED positions successfully loaded from storage.");
}

uint32_t prng_state = 12345;
uint32_t xorshift32() {
  //https://en.wikipedia.org/wiki/Xorshift
	/* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
	uint32_t x = prng_state;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return prng_state = x;
}

void stopAllEffects() {
  currentEffect = EFFECT_NONE;
  effectStartTimeMs = 0;

  // Save state to NVS
  prefs.putUInt("last_effect", (uint32_t)EFFECT_NONE);

  // reset LEDs to a known state
  for (uint16_t i = 0; i < numPixels; ++i) {
    setPixelColor(i, 0, 0, 0);
  }
  showPixelColors();
}

void startEffect(EffectType effect) {
  // stop whatever was running
  stopAllEffects();

  // start new effect
  currentEffect = effect;
  effectStartTimeMs = millis();

  // Save the new effect to NVS
  prefs.putUInt("last_effect", (uint32_t)effect);
}

// -------------------- The Effects -----------------------
void updateBlinkEffect() {
  // elapsed time since effect started
  unsigned long elapsed = millis() - effectStartTimeMs;

  char secs_on = 1;
  char secs_off = 1;
  bool on = ((elapsed / 1000) % (secs_on + secs_off)) <= secs_on;

  for (uint16_t i = 0; i < numPixels; ++i) {
    if (on) {
      setPixelColor(i, baseR, baseG, baseB); 
    } else {
      setPixelColor(i, 0, 0, 0); 
    }
  }
  showPixelColors();
}

void resetSweepingPlaneEffect() {
  effectStartTimeMs = millis();

  // get new random numbers
  prng_state = effectStartTimeMs;
  for (int i = effectStartTimeMs % 10; i <= 20; i++) {
    xorshift32();
  }

  // calculate new Z positions
  float x = ((prng_state >>  0) & 0xFFFF) * (2.0f / 65536.0f) - 1.0f;
  float y = ((prng_state >> 16) & 0xFFFF) * (2.0f / 65536.0f) - 1.0f;
  float inv_len = 1/sqrtf(x*x + y*y);
  float cos_theta = x * inv_len;
  float sin_theta = y * inv_len;
  xorshift32();
  x = ((prng_state >>  0) & 0xFFFF) * (2.0f / 65536.0f) - 1.0f;
  y = ((prng_state >> 16) & 0xFFFF) * (2.0f / 65536.0f) - 1.0f;
  inv_len = 1/sqrtf(x*x + y*y);
  float cos_alpha = x * inv_len;
  float sin_alpha = y * inv_len;
  
  if (effect_sweeping_plane_zposs) {
    delete[] effect_sweeping_plane_zposs;
  }
  effect_sweeping_plane_zposs = new float[numPixels];

  float min_z = -100;
  for (uint16_t i = 0; i < numPixels; ++i) {
    float x = ledPositions[i].x;
    float y = ledPositions[i].y;
    float z = ledPositions[i].z;
    effect_sweeping_plane_zposs[i] = sin_theta * (sin_alpha * x + cos_alpha * y) + cos_theta * z;
    if (effect_sweeping_plane_zposs[i] < min_z) {
      min_z = effect_sweeping_plane_zposs[i];
    }
  }

  // already offset z positions to start from 0 and upwards
  for (uint16_t i = 0; i < numPixels; ++i) {
    effect_sweeping_plane_zposs[i] += min_z;
  }

  effect_sweeping_plane_hue = (prng_state & 0xFFFF) / 65536.0f * 360.0f;
}

void updateSweepingPlaneEffect() {
  // elapsed time since effect started
  unsigned long elapsed = millis() - effectStartTimeMs;
  
  // calculate new color
  // https://cs.stackexchange.com/a/127918 the end of the answer contains the good stuff
  float h = effect_sweeping_plane_hue;
  float s = 0.40;
  float v = 0.20; 

  float max = v;
  float c = s*v;
  float min = max - c;  // equivalent to v * (1-s)
  float h_fraction = ((int)h % 60) / 60;
  float r, g, b;
  if (0 <= h && h < 60) {
    r = max;
    g = min;
    b = min + h_fraction * c;
  } else if (0 <= h && h < 60) {
    r = max;
    g = min + h_fraction * c;
    b = min;
  } else if (0 <= h && h < 60) {
    r = min + h_fraction * c;
    g = max;
    b = min;
  } else if (0 <= h && h < 60) {
    r = min;
    g = max;
    b = min + h_fraction * c;
  } else if (0 <= h && h < 60) {
    r = min;
    g = min + h_fraction * c;
    b = max;
  } else if (0 <= h && h < 60) {
    r = min + h_fraction * c;
    g = min;
    b = max;
  } else {
    r = 1;
    g = 1;
    b = 1;
  }

  float max_z = 100;
  for (int i = 0; i<numPixels; i++) {
    if (effect_sweeping_plane_zposs[i] > max_z) {
      max_z = effect_sweeping_plane_zposs[i];
    }
  }

  // sweep from minz to maxz with a constant speed -> effect needs to reset when the tree is done sweeping, not after a fixed time.
  // we do one unit per second, the tree is two units wide and deep
  float speed_in_units_per_ms = 0.001;
  float plane_z_position = elapsed * speed_in_units_per_ms;

  for (int i = 0; i<numPixels; i++) {
    float this_z = effect_sweeping_plane_zposs[i];
    if ((this_z - 0.1 < plane_z_position && plane_z_position < this_z + 0.1 ) && ledMask[i]) {
      setPixelColor(i, r * 255, g * 255, b * 255);
    } else {
      setPixelColor(i, 0, 0, 0);
    }
  }
  showPixelColors();
  


  // after plane is done, reset and choose new direction
  if (plane_z_position > max_z) {
    resetSweepingPlaneEffect();
  }
}

void resetSweepingPlaneXYZEffect() {
  effectStartTimeMs = millis();

  if (effect_sweeping_plane_zposs) {
    delete[] effect_sweeping_plane_zposs;
  }
  effect_sweeping_plane_zposs = new float[numPixels];

  float min_z = -100;
  for (uint16_t i = 0; i < numPixels; ++i) {
    switch effect {
      case EFFECT_SWEEPING_PLANE_X:
        effect_sweeping_plane_zposs[i] = ledPositions[i].x
        break;
      case EFFECT_SWEEPING_PLANE_Y:
        effect_sweeping_plane_zposs[i] = ledPositions[i].y
        break;
      case EFFECT_SWEEPING_PLANE_Z:
      default:
        effect_sweeping_plane_zposs[i] = ledPositions[i].z
        break;
    }
    if (effect_sweeping_plane_zposs[i] < min_z) {
      min_z = effect_sweeping_plane_zposs[i];
    }
  }

  // already offset z positions to start from 0 and upwards
  for (uint16_t i = 0; i < numPixels; ++i) {
    effect_sweeping_plane_zposs[i] += min_z;
  }
}

void updateSweepingPlaneXYZEffect() {
  // elapsed time since effect started
  unsigned long elapsed = millis() - effectStartTimeMs;
  
  float max_z = 100;
  for (int i = 0; i<numPixels; i++) {
    if (effect_sweeping_plane_zposs[i] > max_z) {
      max_z = effect_sweeping_plane_zposs[i];
    }
  }

  // sweep from minz to maxz with a constant speed -> effect needs to reset when the tree is done sweeping, not after a fixed time.
  // we do one unit per second, the tree is two units wide and deep
  float speed_in_units_per_ms = 0.001;
  float plane_z_position = elapsed * speed_in_units_per_ms;

  for (int i = 0; i<numPixels; i++) {
    float this_z = effect_sweeping_plane_zposs[i];
    if ((this_z - 0.1 < plane_z_position && plane_z_position < this_z + 0.1 ) && ledMask[i]) {
      setPixelColor(i, baseR, baseG, baseB);
    } else {
      setPixelColor(i, 0, 0, 0);
    }
  }
  showPixelColors();
  
  // after plane is done, reset and choose new direction
  if (plane_z_position > max_z) {
    resetSweepingPlaneXYZEffect();
  }
}

// -------------------- HTTP handlers -----------------------
void handleRoot() { server.send_P(200, "text/html", index_html); }
void handleStyle() { server.send_P(200, "text/css", style_css); }
void handleMainJs() { server.send_P(200, "application/javascript", main_js); }
void handleUiJs() { server.send_P(200, "application/javascript", ui_js); }
void handleMergeJs() { server.send_P(200, "application/javascript", merge_directions_js); }
void handleCaptureJs() { server.send_P(200, "application/javascript", capture_unidirectional_js); }
void handleEffectsJs() { server.send_P(200, "application/javascript", effects_js); }

// turn LEDs on or off, according to the dict sent from the frontend. Used for LED position determination and for turning on/off individual LEDs
void handleConfigureLEDs() {
  String body = getRequestBody();
  if (body.length() == 0) {
    server.send(400, "text/plain", "no body");
    return;
  }

  // parse JSON of format {"0": true, "1": false, ...}
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    server.send(400, "text/plain", String("JSON parse error: ") + err.c_str()); 
    return;
  }

  // turn all LEDs off
  for (uint16_t i = 0; i < numPixels; ++i) {
    setPixelColor(i, 0, 0, 0); 
  }

  // turn LEDs on if so instructed
  JsonObject obj = doc.as<JsonObject>();
  for (JsonPair kv : obj) {
    int idx = atoi(kv.key().c_str());
    bool val = kv.value().as<bool>();
    if (val) {
      setPixelColor(idx, baseR, baseG, baseB);
    }  // if val is false, turn it off. But that has already been done
  }
  showPixelColors();

  server.send(200, "text/plain", "success");
}

void handleSetLedPositions() {
  String body = getRequestBody();  // Get the body of the POST request
  if (body.length() == 0) {
    server.send(400, "text/plain", "No body in the request.");
    return;
  }

  // Parse the incoming JSON string
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    server.send(400, "text/plain", String("JSON parse error: ") + err.c_str());
    return;
  }

  // Save the entire JSON string directly to NV storage
  prefs.putString("led_positions", body);
  loadLedPositionsFromStorage();

  server.send(200, "text/plain", "LED positions successfully saved.");
}

void handleGetSavedLedPositions() {
  String savedJson = prefs.getString("led_positions", "");  // Retrieve the saved JSON
  if (savedJson.length() == 0) {
    server.send(204, "application/json", "");  // Return 204 (No Content) if no data
  } else {
    server.send(200, "application/json", savedJson);  // Return the saved JSON
  }
}

void handleSetNumLEDs() {
  String body = getRequestBody();
  if (body.length() == 0) {
    server.send(400, "text/plain", "no body");
    return;
  }

  // parse {"num":24}
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    server.send(400, "text/plain", String("JSON parse error: ") + err.c_str());
    return;
  }
  if (!doc["num"].is<int>()) {
    server.send(400,"text/plain","missing num");
    return;
  }
  int newNum = doc["num"].as<int>();

  if (newNum <= 0 || newNum > 8196) {
    server.send(400, "text/plain", "num out of range (1..8196)");
    return;
  }

  allocateStrip((uint16_t)newNum);

  prefs.putUInt("num_leds", numPixels);
  prefs.putString("led_state", ledMaskToJsonString());
  redrawPixels();
  server.send(200, "text/plain", "success");
}

void handleGetNumLEDs() {
  JsonDocument out;
  out["num"] = numPixels;
  String s;
  serializeJson(out, s);
  server.send(200, "application/json", s);
}

void handleMaskLED() {
  String body = getRequestBody();
  if (body.length() == 0) {
    server.send(400, "text/plain", "no body");
    return;
  }

  // parse {"num":24}
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    server.send(400, "text/plain", String("JSON parse error: ") + err.c_str());
    return;
  }
  if (!doc["num"].is<int>()) {
    server.send(400,"text/plain","missing num");
    return;
  }
  int led_index = doc["num"].as<int>();

  if (led_index <= 0 || led_index > numPixels) {
    server.send(400, "text/plain", "num out of range (1..8196)");
    return;
  }

  ledMask[led_index] = false;

  redrawPixels();
  server.send(200, "text/plain", "success");
}

void handleUnmaskLED() {
  String body = getRequestBody();
  if (body.length() == 0) {
    server.send(400, "text/plain", "no body");
    return;
  }

  // parse {"num":24}
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    server.send(400, "text/plain", String("JSON parse error: ") + err.c_str());
    return;
  }
  if (!doc["num"].is<int>()) {
    server.send(400,"text/plain","missing num");
    return;
  }
  int led_index = doc["num"].as<int>();

  if (led_index <= 0 || led_index > numPixels) {
    server.send(400, "text/plain", "num out of range (1..8196)");
    return;
  }

  ledMask[led_index] = true;

  redrawPixels();
  server.send(200, "text/plain", "success");
}

void handleUnmaskAll() {
  for (int i = 0; i < numPixels; i++) {
    ledMask[led_index] = true;
  }
  redrawPixels();
  server.send(200, "text/plain", "success");
}

void handleSetBaseColor() {
  String body = getRequestBody();
  if (body.length() == 0) {
    server.send(400, "text/plain", "no body");
    return;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    server.send(400, "text/plain", "JSON parse error");
    return;
  }

  // Update global variables
  baseR = doc["r"] | baseR;
  baseG = doc["g"] | baseG;
  baseB = doc["b"] | baseB;

  // Persist to NVS
  prefs.putUChar("bR", baseR);
  prefs.putUChar("bG", baseG);
  prefs.putUChar("bB", baseB);

  // Immediately update the strip
  redrawPixels();

  server.send(200, "text/plain", "color updated");
}

void handleStopEffects() {
  stopAllEffects();
  server.send(200, "text/plain", "effects stopped");
}

void handleStartBlinkEffect() {
  startEffect(EFFECT_BLINK);
  server.send(200, "text/plain", "blink effect started");
}

void handleAllOnEffect(){
  startEffect(EFFECT_ALL_ON);
  redrawPixels();
  server.send(200, "text/plain", "effect started");
}

void handleSweepingPlaneEffect(){
  startEffect(EFFECT_SWEEPING_PLANE);
  resetSweepingPlaneEffect();
  redrawPixels();
  server.send(200, "text/plain", "effect started");
}

void handleSweepingPlaneXEffect() {
  startEffect(EFFECT_SWEEPING_PLANE_X);
  resetSweepingPlaneXYZEffect();
  redrawPixels();
  server.send(200, "text/plain", "effect started");
}

void handleSweepingPlaneYEffect() {
  startEffect(EFFECT_SWEEPING_PLANE_Y);
  resetSweepingPlaneXYZEffect();
  redrawPixels();
  server.send(200, "text/plain", "effect started");
}

void handleSweepingPlaneZEffect() {
  startEffect(EFFECT_SWEEPING_PLANE_Z);
  resetSweepingPlaneXYZEffect();
  redrawPixels();
  server.send(200, "text/plain", "effect started");
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

// ------------------------- Setup & loop -------------------------
void setup() {
  pinMode(LED_BUILTIN_PIN, OUTPUT);
  digitalWrite(LED_BUILTIN_PIN, HIGH);
  
  Serial.begin(115200);
  // wait for userinput from serial connection
  // pressing ctrl+d in `screen /dev/ttyUSBx 115200` increases the number returned from Serial.available() by 1
  // workflow is flashing, then in the Terminal doing: `screen -S ESP /dev/ttyUSB0 115200  && screen -X -S ESP quit` and in screen, pressing ctrl+d to start, ctrl+ad to detach and quit
  // while (!(Serial.available())) {  
    // delay(100);
  // }

  prefs.begin("ledcfg", false);

  // Restore saved colors
  baseR = prefs.getUChar("bR", 50);
  baseG = prefs.getUChar("bG", 50);
  baseB = prefs.getUChar("bB", 50);

  // Restore number of LEDs if saved
  loadLedPositionsFromStorage();
  Serial.print("Currently configured pixels ('led_positions'): ");
  Serial.println(numPixels);

  uint32_t savedNum = prefs.getUInt("num_leds", STRIP_NUMPIXELS_DEFAULT);
  if (savedNum >= 1 && savedNum <= 8196) {
    numPixels = (uint16_t)savedNum;
  } else {
    numPixels = STRIP_NUMPIXELS_DEFAULT;
  }

  // allocate strip (pixels pointer and ledState)
  allocateStrip(numPixels);
  Serial.print("Currently configured pixels ('num_leds'): ");
  Serial.println(numPixels);


  // Restore the last running effect
  uint32_t savedEffect = prefs.getUInt("last_effect", (uint32_t)EFFECT_NONE);
  currentEffect = (EffectType)savedEffect;

  // Start AP
  const char* ssid = "LEDStrip";
  const char* password = "LEDStrip";
  WiFi.softAP(ssid, password);
  IPAddress apIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(apIP);

  // Start mDNS so device is reachable as http://leds.local
  if (!MDNS.begin("leds")) {
      Serial.println("Error: mDNS responder failed to start");
  } else {
      Serial.println("mDNS responder started: http://leds.local");
  }
  MDNS.addService("http", "tcp", 80);  // Advertise the HTTP service

  // Routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/static/style.css", HTTP_GET, handleStyle);
  server.on("/static/script/main.js", HTTP_GET, handleMainJs);
  server.on("/static/script/ui.js", HTTP_GET, handleUiJs);
  server.on("/static/script/merge_directions.js", HTTP_GET, handleMergeJs);
  server.on("/static/script/capture_unidirectional.js", HTTP_GET, handleCaptureJs);
  server.on("/static/script/effects.js", HTTP_GET, handleEffectsJs);

  server.on("/configure_leds", HTTP_POST, handleConfigureLEDs);
  server.on("/set_led_positions", HTTP_POST, handleSetLedPositions);
  server.on("/get_saved_led_positions", HTTP_GET, handleGetSavedLedPositions);

  server.on("/set_num_leds", HTTP_POST, handleSetNumLEDs);
  server.on("/get_num_leds", HTTP_GET, handleGetNumLEDs);

  server.on("/mask_led", HTTP_POST, handleMaskLED);
  server.on("/unmask_led", HTTP_POST, handleUnmaskLED);
  server.on("/unmask_all", HTTP_POST, handleUnmaskAll);
  
  server.on("/effects/stop", HTTP_POST, handleStopEffects);
  server.on("/effects/blink", HTTP_POST, handleStartBlinkEffect);
  server.on("/effects/planex", HTTP_POST, handleSweepingPlaneXEffect);
  server.on("/effects/planey", HTTP_POST, handleSweepingPlaneYEffect);
  server.on("/effects/planez", HTTP_POST, handleSweepingPlaneZEffect);
  server.on("/effects/sweepingplane", HTTP_POST, handleSweepingPlaneEffect);
  server.on("/effects/allon", HTTP_POST, handleAllOnEffect);
  server.on("/effects/basecolor", HTTP_POST, handleSetBaseColor);

  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started on port 80");
  
  // test functionality: light up all leds at once
  // first, make sure all are turned off
  digitalWrite(LED_BUILTIN_PIN, LOW);
  for (int i = 0; i<numPixels; i++) {
    setPixelColor(i, 0, 0, 0);
  }
  showPixelColors();
  delay(500);
  // then turn them on one by one
  digitalWrite(LED_BUILTIN_PIN, HIGH);
  for (int i = 0; i<numPixels; i++) {
    setPixelColor(i, baseR, baseG, baseB);
    showPixelColors();
    delay(5);  // 5ms * 500 LEDs = 2.5s 
  }
  delay(500);
  // then turn them all off again
  for (int i = 0; i<numPixels; i++) {
    setPixelColor(i, 0, 0, 0);
  }
  showPixelColors();
  digitalWrite(LED_BUILTIN_PIN, LOW);
}

void loop() {
  server.handleClient();

  switch (currentEffect) {
    case EFFECT_BLINK:
      updateBlinkEffect();
      break;
    case EFFECT_SWEEPING_PLANE:
      updateSweepingPlaneEffect();
      break;
    case EFFECT_SWEEPING_PLANE_X:
    case EFFECT_SWEEPING_PLANE_Y:
    case EFFECT_SWEEPING_PLANE_Z:
      updateSweepingPlaneXYZEffect();
      break;
    case EFFECT_ALL_ON:
      // do nothing
      break;
    case EFFECT_NONE:
      // do nothing
      break;
    default:
      break;
  }
}

