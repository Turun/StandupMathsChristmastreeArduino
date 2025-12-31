#pragma once

// Auto-generated from: ui.js
const char ui_js[] PROGMEM = R"rawliteral(
import {start_capturing} from "./capture_unidirectional.js";
import { merge_and_transmit, centerLEDBetweenNeighbors} from "./merge_directions.js";
import {blink, allOn, sweepingPlane, stop, setBaseColor, maskLed, unmaskLed, unmaskAll, planeX, planeY, planeZ, concentricColor, configure_leds, setNumLeds} from "./effects.js";

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
    if (diff_context == undefined) {
        return;
    }
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

    // and the currently selected LED in blue
    if (led_positions_raw[current_led_index] != undefined) {
        const [x, y] = led_positions_raw[current_led_index];
        ctx.fillStyle = 'blue';
        ctx.beginPath();
        ctx.arc(x, y, 7, 0, 2 * Math.PI);
        ctx.fill();
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
                console.log(`Set sizes to ${video.videoWidth}×${video.videoHeight}`);
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

// get the pixel coordinates where the user clicked in the video. But like, in video source coordinates.
function getVideoCoords(e, video) {
    const rect = video.getBoundingClientRect();

    const videoAspect = video.videoWidth / video.videoHeight;
    const elementAspect = rect.width / rect.height;

    let drawWidth, drawHeight, offsetX, offsetY;
    if (elementAspect > videoAspect) {
        drawHeight = rect.height;
        drawWidth = drawHeight * videoAspect;
        offsetX = (rect.width - drawWidth) / 2;
        offsetY = 0;
    } else {
        drawWidth = rect.width;
        drawHeight = drawWidth / videoAspect;
        offsetX = 0;
        offsetY = (rect.height - drawHeight) / 2;
    }

    const x = e.clientX - rect.left - offsetX;
    const y = e.clientY - rect.top - offsetY;

    if (x < 0 || y < 0 || x > drawWidth || y > drawHeight) {
        return null; // clicked in black bars
    }

    return {
        x: (x / drawWidth) * video.videoWidth,
        y: (y / drawHeight) * video.videoHeight
    };
}

// Helper functions to check which tab is active (for your main.js)
function getActiveTab() {
  const activePanel = document.querySelector('.tab-panel.active');
  return activePanel ? activePanel.id : null;
}

function isTabXActive() {
  return getActiveTab() === 'x-coords';
}

function isTabYActive() {
  return getActiveTab() === 'y-coords';
}

function current_led_changed() {
    stop();
    configure_leds({[current_led_index]: true});
    if (isTabXActive()) {    
        visualize_led_positions(diff_context, diff_canvas, led_positions_raw_x);
    } else if (isTabYActive()) {
        visualize_led_positions(diff_context, diff_canvas, led_positions_raw_y);
    }
}

// Log a message to the log panel
export function my_log(message) {
  const logContent = document.getElementById('log-content');
  const entry = document.createElement('div');
  entry.className = 'log-entry';
  
  // Get current time
  const now = new Date();
  const timeStr = now.toLocaleTimeString('en-US', { 
    hour12: false, 
    hour: '2-digit', 
    minute: '2-digit', 
    second: '2-digit' 
  });
  
  entry.innerHTML = `<span class="log-time">[${timeStr}]</span>${message}`;
  logContent.appendChild(entry);
  
  // Auto-scroll to bottom
  logContent.scrollTop = logContent.scrollHeight;
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
    video.addEventListener('click', (e) => {
        // get the pixel that was clicked
        const coords = getVideoCoords(e, video);
        if (!coords) return;

        if (isTabXActive()) {
            led_positions_raw_x[current_led_index] = [coords.x, coords.y];
            visualize_led_positions(diff_context, diff_canvas, led_positions_raw_x);
        } else if (isTabYActive()) {
            led_positions_raw_y[current_led_index] = [coords.x, coords.y];
            visualize_led_positions(diff_context, diff_canvas, led_positions_raw_y);
        }
    });
    const navTabX = document.getElementById('nav-tab-x');
    navTabX.addEventListener('click', async () => {
        visualize_led_positions(diff_context, diff_canvas, led_positions_raw_x);
    });
    const navTabY = document.getElementById('nav-tab-y');
    navTabY.addEventListener('click', async () => {
        visualize_led_positions(diff_context, diff_canvas, led_positions_raw_y);
    });

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

    
    const numLedsInput = document.getElementById('num-leds-input');
    const setNumLedsBtn = document.getElementById('set-num-leds-btn'); 
    setNumLedsBtn.addEventListener('click', () => {
        num_leds = parseInt(numLedsInput.value);
        setNumLeds(num_leds);
        allOn();
    });
    numLedsInput.value = num_leds;  // init the value to what we currently think the right value is. fetched from the backend

    // populate led select dropdown
    const select = document.getElementById('led-select');
    // keep dropdown in sync if the user manually changes it
    select.addEventListener('change', () => {
        current_led_index = parseInt(select.value);
        current_led_changed();
    });
    select.innerHTML = '';
    for (let i = 0; i < num_leds; i++) {
        const option = document.createElement('option');
        option.value = i;
        option.textContent = 'LED ' + i;
        select.appendChild(option);
    }
    select.value = current_led_index;
    const decrementButton = document.getElementById('btn-dec');
    decrementButton.addEventListener('click', async () => {
        current_led_index = (current_led_index + num_leds - 1) % num_leds;
        select.value = current_led_index;
        current_led_changed();
    });
    const incrementButton = document.getElementById('btn-inc');
    incrementButton.addEventListener('click', async () => {
        current_led_index = (current_led_index + 1) % num_leds;
        select.value = current_led_index;
        current_led_changed();
    });
    const centerLEDPositionXButton = document.getElementById('center-led-position-in-x-btn');
    centerLEDPositionXButton.addEventListener('click', async () => {
        centerLEDBetweenNeighbors(current_led_index, num_leds, led_positions_raw_x);
        visualize_led_positions(diff_context, diff_canvas, led_positions_raw_x);
    });
    const centerLEDPositionYButton = document.getElementById('center-led-position-in-y-btn');
    centerLEDPositionYButton.addEventListener('click', async () => {
        centerLEDBetweenNeighbors(current_led_index, num_leds,  led_positions_raw_y);
        visualize_led_positions(diff_context, diff_canvas, led_positions_raw_y);
    });

    const maskLEDButton = document.getElementById('mask-led-btn');
    maskLEDButton.addEventListener('click', async () => {
        maskLed(current_led_index);
    });
    const unmaskLEDButton = document.getElementById('unmask-led-btn');
    unmaskLEDButton.addEventListener('click', async () => {
        unmaskLed(current_led_index);
    });
    const unmaskAllButton = document.getElementById('unmask-all-btn');
    unmaskAllButton.addEventListener('click', async () => {
        unmaskAll();
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
    const effectAllOnButton2 = document.getElementById('effect-all-on-btn2');
    effectAllOnButton2.addEventListener('click', () => {
        setBaseColor(colorPicker.value);
        allOn();
    });
    const effectSweepingPlaneButton = document.getElementById('effect-sweeping-plane-btn');
    effectSweepingPlaneButton.addEventListener('click', () => {
        sweepingPlane();
    });
    const effectSweepingPlaneXButton = document.getElementById('effect-sweeping-plane-x-btn');
    effectSweepingPlaneXButton.addEventListener('click', () => {
        planeX();
    });
    const effectSweepingPlaneYButton = document.getElementById('effect-sweeping-plane-y-btn');
    effectSweepingPlaneYButton.addEventListener('click', () => {
        planeY();
    });
    const effectSweepingPlaneZButton = document.getElementById('effect-sweeping-plane-z-btn');
    effectSweepingPlaneZButton.addEventListener('click', () => {
        planeZ();
    });
    const effectConcentricColorButton = document.getElementById('effect-concentric-color-btn');
    effectConcentricColorButton.addEventListener('click', () => {
        concentricColor();
    });
    const effectStopButton = document.getElementById('effect-stop-btn');
    effectStopButton.addEventListener('click', () => {
        stop();
    });
}



)rawliteral";
