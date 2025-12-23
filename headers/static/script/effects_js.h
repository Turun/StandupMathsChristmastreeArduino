#pragma once

// Auto-generated from: effects.js
const char effects_js[] PROGMEM = R"rawliteral(
export function blink() {
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "efffects/blink", false);
    xhr.send();
}

export function planes() {
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "efffects/planes", false);
    xhr.send();
}

export function stop() {
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "efffects/stop", false);
    xhr.send();
}

)rawliteral";
