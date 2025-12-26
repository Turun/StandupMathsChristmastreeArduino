/*
  ESP32 LEDStrip AP + Webserver + Adafruit NeoPixel control
  - AP: SSID "LEDStrip", PW "LEDStrip"
  - HTTP server on port 8080
  - Serves files via included PROGMEM headers (see instructions)
  - Endpoints:
      GET  /                          -> index.html
      GET  /static/style.css
      GET  /static/script/main.js
      GET  /static/script/ui.js
      GET  /static/script/merge_directions.js
      GET  /static/script/capture_unidirectional.js
      POST /configure_leds            -> JSON dict string->bool, e.g. {"0":true,"1":false}
      POST /set_led_positions         -> JSON positions saved to NV storage
      GET  /get_saved_led_positions   -> returns saved JSON positions
      POST /set_num_leds              -> JSON {"num":24} to change LED count (saved to NV)
      GET  /get_num_leds              -> returns {"num": <current>}
*/

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <Preferences.h>
#include <Arduino.h>

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
  // future:
  // EFFECT_PULSE,
  // EFFECT_CHASE,
};
EffectType currentEffect = EFFECT_NONE;
unsigned long effectStartTimeMs = 0;

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

  numPixels = newNum;
  
  // Allocate new state
  ledMask = new bool[numPixels];
  for (uint16_t i = 0; i < numPixels; ++i) {
    ledMask[i] = false;
  }

  // Allocate new positions array
  ledPositions = new Point[numPixels];
  
  // Instantiate new NeoPixel
  pixels = new Adafruit_NeoPixel(numPixels, STRIP_PIN_DEFAULT, NEO_GRB + NEO_KHZ800);
  pixels->begin();
  pixels->clear();
  pixels->show();
}

void setPixelColor(uint16_t index, uint8_t r, uint8_t g, uint8_t b) {
  if (!pixels) return;
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

void stopAllEffects() {
  currentEffect = EFFECT_NONE;
  effectStartTimeMs = 0;

  // Save state to NVS
  prefs.putUInt("last_effect", (uint32_t)EFFECT_NONE);

  // reset LEDs to a known state
  for (uint16_t i = 0; i < numPixels; ++i) {
    setPixelColor(i, baseR, baseG, baseB);  // TODO: either we make this set all pixels off, or we add a button to turn all pixels off. At the moment, this is identical to pressing the all on button
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
  JsonObject obj = doc.as<JsonObject>();
  for (JsonPair kv : obj) {
    int idx = atoi(kv.key().c_str());
    bool val = kv.value().as<bool>();
    if (idx >= 0 && idx < numPixels) {
      ledMask[idx] = val;  // TODO: might be worth thinking about a split of this method for disabling single LEDs and for determining the LED positions
    }
  }

  redrawPixels();
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

  uint32_t savedNum = prefs.getUInt("num_leds", 0);
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
  
  server.on("/effects/stop", HTTP_POST, handleStopEffects);
  server.on("/effects/blink", HTTP_POST, handleStartBlinkEffect);
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
    delay(5);  // if we have 1000 LEDs, this is the time it will take to light them all up in seconds
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

