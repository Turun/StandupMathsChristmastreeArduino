#pragma once

// Auto-generated from: effects.js
const char effects_js[] PROGMEM = R"rawliteral(
export function blink() {
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "effects/blink", false);
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.send();
}

export function allOn() {
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "effects/allon", false);
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.send();
}

export function stop() {
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "effects/stop", false);
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.send();
}

export function setBaseColor(hex) {
    // Convert hex (#rrggbb) to RGB integers
    const r = parseInt(hex.slice(1, 3), 16);
    const g = parseInt(hex.slice(3, 5), 16);
    const b = parseInt(hex.slice(5, 7), 16);

    const xhr = new XMLHttpRequest();
    xhr.open("POST", "effects/basecolor", false);
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.send(JSON.stringify({ r, g, b }));
}
)rawliteral";
