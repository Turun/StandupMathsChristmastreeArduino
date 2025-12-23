#pragma once

// Auto-generated from: effects.js
const char effects_js[] PROGMEM = R"rawliteral(
export function blink() {
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "effects/blink", false);
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.send();
}

export function planes() {
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "effects/planes", false);
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.send();
}

export function stop() {
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "effects/stop", false);
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.send();
}

)rawliteral";
