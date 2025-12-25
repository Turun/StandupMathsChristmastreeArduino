#pragma once

// Auto-generated from: ui.js
const char ui_js[] PROGMEM = R"rawliteral(
import {start_capturing, is_led_on, compute_lock_in_image_data_array} from "./capture_unidirectional.js";
import { merge_and_transmit } from "./merge_directions.js";
import {blink, allOn, stop, setBaseColor} from "./effects.js";

let current_led_index = 0;

function test_is_led_on(max_led_index, max_cycle)  {
    for (let cycle = 0; cycle <= max_cycle; cycle += 1) {
        let out = "";
        for (let led_index = 0; led_index <= max_led_index; led_index += 1) {
            if (is_led_on(led_index, cycle)) {
                out += "x";
            } else {
                out += "_"
            }
        }
        console.log(out);
    }
}

/**
 * Ensures the function 'func' is called at most once every 'limit' milliseconds.
 */
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

// populate the LED dropdown after analyzing
function populate_led_select(num_leds) {
    const select = document.getElementById('led-select');
    select.innerHTML = '';
    for (let i = 0; i < num_leds; i++) {
        const option = document.createElement('option');
        option.value = i;
        option.textContent = 'LED ' + i;
        select.appendChild(option);
    }
    select.value = current_led_index;
}

function setup_buttons(
    num_leds,
    num_cycles,
    math_canvas,
    diff_canvas,
    diff_context,
    contexts_x,
    contexts_y,
    led_positions_raw_x,
    led_positions_raw_y,
) {
    const select = document.getElementById('led-select');
    const prevBtnX = document.getElementById('prev-led-btn-x');
    const visualizeBtnX = document.getElementById('visualize-btn-x');
    const nextBtnX = document.getElementById('next-led-btn-x');
    const overviewBtnX = document.getElementById('overview-btn-x');
    const prevBtnY = document.getElementById('prev-led-btn-y');
    const visualizeBtnY = document.getElementById('visualize-btn-y');
    const nextBtnY = document.getElementById('next-led-btn-y');
    const overviewBtnY = document.getElementById('overview-btn-y');

    // keep dropdown in sync if the user manually changes it
    select.addEventListener('change', () => {
        current_led_index = parseInt(select.value);
    });

    // buttons for showing X view
    prevBtnX.addEventListener('click', () => {
        current_led_index = (current_led_index - 1 + num_leds) % num_leds;
        select.value = current_led_index;
        visualize_single_led(num_leds, num_cycles, current_led_index, math_canvas, diff_context, contexts_x, led_positions_raw_x);
    });
    visualizeBtnX.addEventListener('click', () => {
        current_led_index = parseInt(select.value);
        visualize_single_led(num_leds, num_cycles, current_led_index, math_canvas, diff_context, contexts_x, led_positions_raw_x);
    });
    nextBtnX.addEventListener('click', () => {
        current_led_index = (current_led_index + 1) % num_leds;
        select.value = current_led_index;
        visualize_single_led(num_leds, num_cycles, current_led_index, math_canvas, diff_context, contexts_x, led_positions_raw_x);
    });
    overviewBtnX.addEventListener('click', () => {
        visualize_led_positions(diff_context, diff_canvas, led_positions_raw_x); // show all LEDs
    });

    // buttons for showing Y view
    prevBtnY.addEventListener('click', () => {
        current_led_index = (current_led_index - 1 + num_leds) % num_leds;
        select.value = current_led_index;
        visualize_single_led(num_leds, num_cycles, current_led_index, math_canvas, diff_context, contexts_y, led_positions_raw_y);
    });
    visualizeBtnY.addEventListener('click', () => {
        current_led_index = parseInt(select.value);
        visualize_single_led(num_leds, num_cycles, current_led_index, math_canvas, diff_context, contexts_y, led_positions_raw_y);
    });
    nextBtnY.addEventListener('click', () => {
        current_led_index = (current_led_index + 1) % num_leds;
        select.value = current_led_index;
        visualize_single_led(num_leds, num_cycles, current_led_index, math_canvas, diff_context, contexts_y, led_positions_raw_y);
    });
    overviewBtnY.addEventListener('click', () => {
        visualize_led_positions(diff_context, diff_canvas, led_positions_raw_y); // show all LEDs
    });
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
    ctx.textAlign = 'left';
    ctx.textBaseline = 'top';

    for (let i = 0; i < led_positions_raw.length; i++) {
        const [x, y] = led_positions_raw[i];
        if (x == null || y == null) continue;

        // draw small red circle
        ctx.fillStyle = 'red';
        ctx.beginPath();
        ctx.arc(x, y, 5, 0, 2 * Math.PI);
        ctx.fill();

        // draw LED number
        ctx.fillStyle = 'black';
        ctx.fillText(i, x + 6, y - 6);
    }
}

// visualize the lock-in difference image for a single LED
function visualize_single_led(
    num_leds,
    num_cycles,
    led_index,
    math_canvas,
    diff_context,
    contexts,
    led_positions_raw,
) {
    if (led_index == null || led_index < 0 || led_index >= num_leds) return;

    // recompute lock-in image for that LED
    const image_data_array = compute_lock_in_image_data_array(
        led_index,
        num_cycles,
        math_canvas,
        contexts,
    );

    // normalize image array
    let minVal = Number.POSITIVE_INFINITY;
    let maxVal = Number.NEGATIVE_INFINITY;
    for (let i = 0; i < image_data_array.length; i++) {
        if (image_data_array[i] < minVal) minVal = image_data_array[i];
        if (image_data_array[i] > maxVal) maxVal = image_data_array[i];
    }
    const range = maxVal - minVal || 1; // avoid division by zero

    // draw to diff_canvas
    const canvas_image = diff_context.createImageData(math_canvas.width, math_canvas.height);
    for (let i = 0; i < image_data_array.length; i++) {
        const normalized = ((image_data_array[i] - minVal) / range) * 255;
        canvas_image.data[4 * i] = normalized;
        canvas_image.data[4 * i + 1] = normalized;
        canvas_image.data[4 * i + 2] = normalized;
        canvas_image.data[4 * i + 3] = 255; // fully opaque
    }
    diff_context.putImageData(canvas_image, 0, 0);

    // overlay red marker for the LED position
    const [x, y] = led_positions_raw[led_index];
    if (x != null && y != null) {
        const armLength = 10;   // length of each arm
        const gap = 1;          // half of the 3px center gap
        diff_context.strokeStyle = "red";
        diff_context.lineWidth = 1;

        diff_context.beginPath();
        // horizontal left
        diff_context.moveTo(x - armLength, y);
        diff_context.lineTo(x - gap, y);
        // horizontal right
        diff_context.moveTo(x + gap, y);
        diff_context.lineTo(x + armLength, y);
        // vertical top
        diff_context.moveTo(x, y - armLength);
        diff_context.lineTo(x, y - gap);
        // vertical bottom
        diff_context.moveTo(x, y + gap);
        diff_context.lineTo(x, y + armLength);
        diff_context.stroke();
    }
}

// Function to start the camera
function startCamera(
    video,
    diff_canvas,
    math_canvas,
    canvases,
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

                for (const canvas of canvases) {
                    canvas.width = video.videoWidth;
                    canvas.height = video.videoHeight;
                }

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
    num_cycles,
    video,
    canvases_x,
    canvases_y,
    contexts_x,
    contexts_y,
    math_canvas,
    diff_canvas,
    diff_context,
    led_positions_raw_x,
    led_positions_raw_y,
    led_positions_normalized,
) {
    window.addEventListener('load', () => startCamera(video, diff_canvas, math_canvas, canvases_x.concat(canvases_y)));

    const startButtonX = document.getElementById('start-btn-x');
    startButtonX.addEventListener('click', async () => {
        await start_capturing(
            num_leds,
            num_cycles,
            video,
            contexts_x,
            math_canvas,
            diff_canvas,
            diff_context,
            led_positions_raw_x,
        );
        visualize_led_positions(diff_context, diff_canvas, led_positions_raw_x);
        document.getElementById('prev-led-btn-x').disabled = false;
        document.getElementById('visualize-btn-x').disabled = false;
        document.getElementById('next-led-btn-x').disabled = false;
        document.getElementById('overview-btn-x').disabled = false;
        
    });

    const startButtonY = document.getElementById('start-btn-y');
    startButtonY.addEventListener('click', async () => {
        await start_capturing(
            num_leds,
            num_cycles,
            video,
            contexts_y,
            math_canvas,
            diff_canvas,
            diff_context,
            led_positions_raw_y,
        );
        visualize_led_positions(diff_context, diff_canvas, led_positions_raw_y);
        document.getElementById('prev-led-btn-y').disabled = false;
        document.getElementById('visualize-btn-y').disabled = false;
        document.getElementById('next-led-btn-y').disabled = false;
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

    const effectBlinkButton = document.getElementById('effect-blink-btn');
    effectBlinkButton.addEventListener('click', () => {
         blink();
    });
    const effectAllOnButton = document.getElementById('effect-all-on-btn');
    effectAllOnButton.addEventListener('click', () => {
         allOn();
    });
    const effectStopButton = document.getElementById('effect-stop-btn');
    effectStopButton.addEventListener('click', () => {
         stop();
    });
    const colorPicker = document.getElementById('base-color-picker');
    const throttledSetColor = throttle((hex) => {
        setBaseColor(hex);
    }, 100); 
    colorPicker.addEventListener('input', (event) => {
        const hexColor = event.target.value;
        throttledSetColor(hexColor);
    });

    populate_led_select(num_leds);
    setup_buttons(
        num_leds,
        num_cycles,
        math_canvas,
        diff_canvas,
        diff_context,
        contexts_x,
        contexts_y,
        led_positions_raw_x,
        led_positions_raw_y,
    );
}



)rawliteral";
