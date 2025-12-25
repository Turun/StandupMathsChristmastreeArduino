#pragma once

// Auto-generated from: capture_unidirectional.js
const char capture_unidirectional_js[] PROGMEM = R"rawliteral(
import { visualize_led_positions } from "./ui.js";

/**
 * Helper to wait for the next camera frame.
 */
function waitForNextCameraFrame(video) {
    return new Promise(resolve => {
        if (typeof video.requestVideoFrameCallback === 'function') {
            video.requestVideoFrameCallback((now, metadata) => {
                resolve(metadata ?? { timestamp: now });
            });
        } else {
            requestAnimationFrame(() => {
                requestAnimationFrame(() => {
                    resolve({ timestamp: performance.now() });
                });
            });
        }
    });
}

/**
 * Configures the LED state on the server.
 */
function configure_leds(dict) {
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "configure_leds", false); 
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.send(JSON.stringify(dict));
}

/**
 * Converts RGBA ImageData to a 1-channel Float32Array of brightness.
 * Uses your original HSP formula: sqrt(0.241*R^2 + 0.691*G^2 + 0.068*B^2)
 */
function get_brightness_map(imageData) {
    const data = imageData.data;
    const width = imageData.width;
    const height = imageData.height;
    const brightnessMap = new Float32Array(width * height);

    for (let i = 0; i < data.length; i += 4) {
        const r = data[i];
        const g = data[i + 1];
        const b = data[i + 2];
        // HSP formula for true perceived brightness
        brightnessMap[i / 4] = Math.sqrt(0.241 * r * r + 0.691 * g * g + 0.068 * b * b);
    }
    return brightnessMap;
}

/**
 * Optimized Box Blur for a single-channel Float32Array.
 */
function blur_array(srcData, width, height, half_kernel_size = 4) {
    const dstData = new Float32Array(width * height);

    for (let y = 0; y < height; y++) {
        for (let x = 0; x < width; x++) {
            let sum = 0, count = 0;
            for (let ky = -half_kernel_size; ky <= half_kernel_size; ky++) {
                for (let kx = -half_kernel_size; kx <= half_kernel_size; kx++) {
                    const px = x + kx;
                    const py = y + ky;
                    if (px >= 0 && px < width && py >= 0 && py < height) {
                        sum += srcData[py * width + px];
                        count++;
                    }
                }
            }
            dstData[y * width + x] = sum / count;
        }
    }
    return dstData;
}

/**
 * Main execution: Processes LED by LED to save memory.
 */
export async function start_capturing(
    num_leds,
    num_cycles,
    video,
    contexts,
    math_canvas,
    diff_canvas,
    diff_context,
    led_positions_raw,
) {
    const ctx = math_canvas.getContext('2d', { willReadFrequently: true });
    const width = math_canvas.width;
    const height = math_canvas.height;

    console.log(`Starting detection for ${num_leds} LEDs...`);

    for (let i = 0; i < num_leds; i++) {
        // 1. Capture "ON" state
        configure_leds({ [i]: true });
        await waitForNextCameraFrame(video);
        ctx.drawImage(video, 0, 0, width, height);
        const brightMapOn = get_brightness_map(ctx.getImageData(0, 0, width, height));

        // 2. Capture "OFF" state
        configure_leds({ [i]: false });
        await waitForNextCameraFrame(video);
        ctx.drawImage(video, 0, 0, width, height);
        const brightMapOff = get_brightness_map(ctx.getImageData(0, 0, width, height));

        // 3. Subtract to isolate the LED
        const diffData = new Float32Array(width * height);
        for (let j = 0; j < diffData.length; j++) {
            diffData[j] = brightMapOn[j] - brightMapOff[j];
        }

        // 4. Blur the difference map once to reduce noise
        const blurredDiff = blur_array(diffData, width, height);

        // 5. Locate the peak
        let maxBrightness = -Infinity;
        let bestX = 0;
        let bestY = 0;

        for (let j = 0; j < blurredDiff.length; j++) {
            if (blurredDiff[j] > maxBrightness) {
                maxBrightness = blurredDiff[j];
                bestX = j % width;
                bestY = Math.floor(j / width);
            }
        }

        led_positions_raw[i] = [bestX, bestY];
        // Update UI
        visualize_led_positions(diff_context, diff_canvas, led_positions_raw);
        console.log(`LED ${i} found at [${bestX}, ${bestY}]`);
    }
    console.log("Capture complete.", led_positions_raw);
}
)rawliteral";
