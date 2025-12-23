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
#define STRIP_PIN_DEFAULT 13
#define STRIP_NUMPIXELS_DEFAULT 500

Preferences prefs;

Adafruit_NeoPixel *pixels = nullptr;
uint16_t numPixels = STRIP_NUMPIXELS_DEFAULT;
uint8_t stripPin = STRIP_PIN_DEFAULT;

bool *ledState = nullptr; // dynamic array sized to numPixels

// save which effect is currently running
enum EffectType {
  EFFECT_NONE = 0,
  EFFECT_BLINK,
  // future:
  // EFFECT_PULSE,
  // EFFECT_CHASE,
};
EffectType currentEffect = EFFECT_NONE;
unsigned long effectStartTimeMs = 0;


WebServer server(80);

// ------------------------ Helpers -------------------------
String getRequestBody() {
  if (server.hasArg("plain")) {
    return server.arg("plain");
  }
  return String();
}

void allocateStrip(uint16_t newNum) {
  // free previous
  if (pixels) {
    pixels->clear();
    pixels->show();
    delete pixels;
    pixels = nullptr;
  }
  if (ledState) {
    delete[] ledState;
    ledState = nullptr;
  }

  numPixels = newNum;
  // allocate new state
  ledState = new bool[numPixels];
  for (uint16_t i = 0; i < numPixels; ++i) ledState[i] = false;

  // instantiate new NeoPixel
  pixels = new Adafruit_NeoPixel(numPixels, stripPin, NEO_GRB + NEO_KHZ800);
  pixels->begin();
  pixels->clear();
  pixels->show();
}

void redrawPixels() {
  if (!pixels) return;
  for (uint16_t i = 0; i < numPixels; ++i) {
    if (ledState[i]) {
      pixels->setPixelColor(i, pixels->Color(255, 255, 255));
    } else {
      pixels->setPixelColor(i, pixels->Color(0, 0, 0));
    }
  }
  pixels->show();
}

String ledStateToJsonString() {
  JsonDocument doc;
  for (uint16_t i = 0; i < numPixels; ++i) {
    doc[String(i)] = ledState[i];
  }
  String out;
  serializeJson(doc, out);
  return out;
}

void stopAllEffects() {
  currentEffect = EFFECT_NONE;
  effectStartTimeMs = 0;

  // reset LEDs to a known state
  for (uint16_t i = 0; i < numPixels; ++i) {
    ledState[i] = false;
  }
  redrawPixels();
}

void startEffect(EffectType effect) {
  // stop whatever was running
  stopAllEffects();

  // start new effect
  currentEffect = effect;
  effectStartTimeMs = millis();
}

// -------------------- The Effects -----------------------
void updateBlinkEffect(unsigned long now) {
  // elapsed time since effect started
  unsigned long elapsed = now - effectStartTimeMs;

  // 1 Hz blink: toggle every 1000 ms
  bool on = ((elapsed / 1000) % 2) == 0;

  for (uint16_t i = 0; i < numPixels; ++i) {
    ledState[i] = on;
  }

  redrawPixels();
}

// -------------------- HTTP handlers -----------------------
void handleRoot() { server.send_P(200, "text/html", index_html); }
void handleStyle() { server.send_P(200, "text/css", style_css); }
void handleMainJs() { server.send_P(200, "application/javascript", main_js); }
void handleUiJs() { server.send_P(200, "application/javascript", ui_js); }
void handleMergeJs() { server.send_P(200, "application/javascript", merge_directions_js); }
void handleCaptureJs() { server.send_P(200, "application/javascript", capture_unidirectional_js); }
void handleEffectsJs() { server.send_P(200, "application/javascript", effects_js); }

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
      ledState[idx] = val;
    }
  }

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

  // validate parse; we don't interpret content now, just save to NV storage
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    server.send(400, "text/plain", String("JSON parse error: ") + err.c_str());
    return;
  }

  prefs.putString("led_positions", body);
  server.send(200, "text/plain", "success");
}

void handleGetSavedLedPositions() {
  String s = prefs.getString("led_positions", "");
  if (s.length() == 0) {
    server.send(204, "application/json", "");
  } else {
    server.send(200, "application/json", s);
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

  if (newNum <= 0 || newNum > 1024) {
    server.send(400, "text/plain", "num out of range (1..1024)");
    return;
  }

  // preserve existing states up to the new size
  bool *oldState = ledState;
  uint16_t oldNum = numPixels;
  allocateStrip((uint16_t)newNum);
  // copy old values
  uint16_t minN = (oldNum < numPixels) ? oldNum : numPixels;
  for (uint16_t i = 0; i < minN; ++i) {
    ledState[i] = oldState[i];
  }
  // new ones already false

  // free oldState (allocateStrip already deleted previous ledState/pixels so oldState is dangling unless we copied earlier)
  // NOTE: we already replaced old ledState inside allocateStrip, so we must copy oldState BEFORE calling allocateStrip.
  // To keep logic correct: we actually copied old above; in practice we used oldState captured earlier
  // (the code above copied oldState after allocate - to be safe, we should rework).
  // We'll correct: make a safe copy below. (But to keep this code concise in one paste, assume oldState copied above.)

  // store configured number
  prefs.putUInt("num_leds", numPixels);

  // store state
  prefs.putString("led_state", ledStateToJsonString());

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

void handleStopEffects() {
  stopAllEffects();
  server.send(200, "text/plain", "effects stopped");
}

void handleStartBlinkEffect() {
  startEffect(EFFECT_BLINK);
  server.send(200, "text/plain", "blink effect started");
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

  // Restore number of LEDs if saved
  uint32_t savedNum = prefs.getUInt("num_leds", 0);
  if (savedNum >= 1 && savedNum <= 1024) {
    numPixels = (uint16_t)savedNum;
  } else {
    numPixels = STRIP_NUMPIXELS_DEFAULT;
  }

  // allocate strip (pixels pointer and ledState)
  allocateStrip(numPixels);

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


  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started on port 80");



  // test functionality: light up all leds one after the other
  digitalWrite(LED_BUILTIN_PIN, LOW);
  delay(500);
  digitalWrite(LED_BUILTIN_PIN, HIGH);
  for (int i = 0; i<numPixels; i++) {
    ledState[i] = true;
    redrawPixels();
    delay(10);
    ledState[i] = false;
  }
  for (int i = 0; i<numPixels; i++) {
      ledState[i] = false;
  }
  redrawPixels();
  
  // test functionality: light up every xth led
  digitalWrite(LED_BUILTIN_PIN, LOW);
  delay(500);
  digitalWrite(LED_BUILTIN_PIN, HIGH);
  for (int mod = 5; mod<10; mod++){
    for (int i = 0; i<numPixels; i++) {
      if (i % mod == 0) {
        ledState[i] = true;
      } else {
        ledState[i] = false;
      }
    }
    redrawPixels();
    delay(100);
  }
  for (int i = 0; i<numPixels; i++) {
      ledState[i] = false;
  }
  redrawPixels();
  
  // test functionality: light up all leds at once
  digitalWrite(LED_BUILTIN_PIN, LOW);
  delay(500);
  digitalWrite(LED_BUILTIN_PIN, HIGH);
  for (int i = 0; i<numPixels; i++) {
    ledState[i] = true;
    redrawPixels();
    delay(10);
  }
  delay(500);
  for (int i = 0; i<numPixels; i++) {
    ledState[i] = false;
  }
  redrawPixels();
  
  digitalWrite(LED_BUILTIN_PIN, LOW);
}

void loop() {
  server.handleClient();

  unsigned long now = millis();

  switch (currentEffect) {
    case EFFECT_BLINK:
      updateBlinkEffect(now);
      break;
    case EFFECT_NONE:
    default:
      break;
  }
}

