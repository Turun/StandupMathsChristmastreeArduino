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

export function setNumLeds(num) {
    const xhr = new XMLHttpRequest();
    xhr.open("POST", "set_num_leds", false);
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.send(JSON.stringify({ num }));
}

export function getNumLeds(callback) {
    const xhr = new XMLHttpRequest();
    xhr.open("GET", "get_num_leds", false);
    xhr.onload = () => {
      if (xhr.status === 200) {
        const data = JSON.parse(xhr.responseText);
        callback(data.num);
      }
    };
    xhr.onerror = () => console.error("Network error in get_num_leds call");
    xhr.send();
}

export function sweepingPlane() {
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "effects/sweepingplane", false);
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.send();
}

export function planeX() {
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "effects/planex", false);
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.send();
}

export function planeY() {
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "effects/planey", false);
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.send();
}

export function planeZ() {
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "effects/planez", false);
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.send();
}

)rawliteral";
