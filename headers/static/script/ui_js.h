#pragma once

// Auto-generated from: ui.js
const char ui_js[] PROGMEM = R"rawliteral(
import {start_capturing} from "./capture_unidirectional.js";
import { merge_and_transmit } from "./merge_directions.js";
import {blink, allOn, sweepingPlane, stop, setBaseColor} from "./effects.js";

let current_led_index = 0;

// Ensures the function 'func' is called at most once every 'limit' milliseconds.
// Function calls that occur during the deadtime are silently dropped
function throttle(func, limit) {
    let inThrottle;
    return function() {
        const args = arguments;
        const context = this;
        if (!inThrottle) {
            func.apply(context, args);
            inThrottle = true;
            setTimeout(() => inThrottle = false, limit);
        }
    }
}


// visualize all LEDs as red markers with numbers
export function visualize_led_positions(
    diff_context,
    diff_canvas,
    led_positions_raw,
) {
    const ctx = diff_context;
    ctx.clearRect(0, 0, diff_canvas.width, diff_canvas.height); // clear previous drawings

    ctx.fillStyle = 'red';
    ctx.strokeStyle = 'red';
    ctx.font = '12px sans-serif';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';

    for (let i = 0; i < led_positions_raw.length; i++) {
        const [x, y] = led_positions_raw[i];
        if (x == null || y == null) continue;

        // draw small red circle
        ctx.fillStyle = 'red';
        ctx.beginPath();
        ctx.arc(x, y, 7, 0, 2 * Math.PI);
        ctx.fill();

        // draw LED number
        ctx.fillStyle = 'black';
        ctx.fillText(i, x, y);
    }
}


// Function to start the camera
function startCamera(
    video,
    diff_canvas,
    math_canvas,
) {
    // TODO: ideally we let the user pick. there is a navigator.mediaDevices.enumerateDevices(); method for that
    navigator.mediaDevices.getUserMedia({
        video: {
            facingMode: "environment"
        }
    }).then((stream) => {
            video.srcObject = stream;

            // Wait until metadata (including dimensions) is available
            video.addEventListener('loadedmetadata', () => {
                // also make the canvases the right size
                diff_canvas.width = video.videoWidth;
                diff_canvas.height = video.videoHeight;
                math_canvas.width = video.videoWidth;
                math_canvas.height = video.videoHeight;
                console.log("Set sizes to " + video.videoWidth + "×" + video.videoHeight);
            });
        })
        .catch((error) => {
            console.error("Error accessing the camera: ", error);
            alert("Could not access the camera. Please allow permissions and try again.");
        });
    if (typeof variable == 'undefined') {
        alert("can't access video on http sites. On firefox enable media.devices.insecure.enabled and media.getusermedia.insecure.enabled in about:config (chrome://geckoview/content/config.xhtml on mobile for some reason)");
    }
}


export function setup_ui(
    num_leds,
    video,
    math_canvas,
    diff_canvas,
    diff_context,
    led_positions_raw_x,
    led_positions_raw_y,
    led_positions_normalized,
) {
    window.addEventListener('load', () => startCamera(video, diff_canvas, math_canvas));

    const startButtonX = document.getElementById('start-btn-x');
    startButtonX.addEventListener('click', async () => {
        await start_capturing(
            num_leds,
            video,
            math_canvas,
            diff_canvas,
            diff_context,
            led_positions_raw_x,
        );
        visualize_led_positions(diff_context, diff_canvas, led_positions_raw_x);
        document.getElementById('overview-btn-x').disabled = false;
        
    });

    const startButtonY = document.getElementById('start-btn-y');
    startButtonY.addEventListener('click', async () => {
        await start_capturing(
            num_leds,
            video,
            math_canvas,
            diff_canvas,
            diff_context,
            led_positions_raw_y,
        );
        visualize_led_positions(diff_context, diff_canvas, led_positions_raw_y);
        document.getElementById('overview-btn-y').disabled = false;
    });

    const transmitButton = document.getElementById('transmit-btn');
    transmitButton.addEventListener('click', () => {
        merge_and_transmit(
            num_leds,
            led_positions_raw_x,
            led_positions_raw_y,
            led_positions_normalized
        );
    });

    const colorPicker = document.getElementById('base-color-picker');
    const throttledSetColor = throttle((hex) => {
        setBaseColor(hex);
    }, 100); 
    colorPicker.addEventListener('input', (event) => {
        const hexColor = event.target.value;
        throttledSetColor(hexColor);
    });
    const effectBlinkButton = document.getElementById('effect-blink-btn');
    effectBlinkButton.addEventListener('click', () => {
        setBaseColor(colorPicker.value);
        blink();
    });
    const effectAllOnButton = document.getElementById('effect-all-on-btn');
    effectAllOnButton.addEventListener('click', () => {
        setBaseColor(colorPicker.value);
        allOn();
    });
    const effectSweepingPlaneButton = document.getElementById('effect-sweeping-plane-btn');
    effectSweepingPlaneButton.addEventListener('click', () => {
        sweepingPlane();
    });
    const effectStopButton = document.getElementById('effect-stop-btn');
    effectStopButton.addEventListener('click', () => {
        stop();
    });

    // populate led select dropdown
    const select = document.getElementById('led-select');
    // keep dropdown in sync if the user manually changes it
    select.addEventListener('change', () => {
        current_led_index = parseInt(select.value);
    });
    select.innerHTML = '';
    for (let i = 0; i < num_leds; i++) {
        const option = document.createElement('option');
        option.value = i;
        option.textContent = 'LED ' + i;
        select.appendChild(option);
    }
    select.value = current_led_index;

    // button for showing X view
    const overviewBtnX = document.getElementById('overview-btn-x');
    overviewBtnX.addEventListener('click', () => {
        visualize_led_positions(diff_context, diff_canvas, led_positions_raw_x); // show all LEDs
    });
    // button for showing Y view
    const overviewBtnY = document.getElementById('overview-btn-y');
    overviewBtnY.addEventListener('click', () => {
        visualize_led_positions(diff_context, diff_canvas, led_positions_raw_y); // show all LEDs
    });
}



)rawliteral";
