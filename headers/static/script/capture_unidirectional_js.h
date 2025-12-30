#pragma once

// Auto-generated from: capture_unidirectional.js
const char capture_unidirectional_js[] PROGMEM = R"rawliteral(
import { visualize_led_positions } from "./ui.js";
import {allOn, stop} from "./effects.js";

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
    video,
    math_canvas,
    diff_canvas,
    diff_context,
    led_positions_raw,
) {
    const ctx = math_canvas.getContext('2d', { willReadFrequently: true });
    const width = math_canvas.width;
    const height = math_canvas.height;

    console.log(`Starting detection for ${num_leds} LEDs...`);
    stop();

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

        // normalize image array
        let minVal = Number.POSITIVE_INFINITY;
        let maxVal = Number.NEGATIVE_INFINITY;
        for (let i = 0; i < blurredDiff.length; i++) {
            if (blurredDiff[i] < minVal) minVal = blurredDiff[i];
            if (blurredDiff[i] > maxVal) maxVal = blurredDiff[i];
        }
        const range = maxVal - minVal || 1; // avoid division by zero

        // draw to diff_canvas
        const canvas_image = diff_context.createImageData(math_canvas.width, math_canvas.height);
        for (let i = 0; i < blurredDiff.length; i++) {
            const normalized = ((blurredDiff[i] - minVal) / range) * 255;
            canvas_image.data[4 * i] = normalized;
            canvas_image.data[4 * i + 1] = normalized;
            canvas_image.data[4 * i + 2] = normalized;
            canvas_image.data[4 * i + 3] = 255; // fully opaque
        }
        diff_context.putImageData(canvas_image, 0, 0);

        // overlay red marker for the LED position
        if (bestX != null && bestY != null) {
            const armLength = 10;   // length of each arm
            const gap = 1;          // half of the 3px center gap
            diff_context.strokeStyle = "red";
            diff_context.lineWidth = 1;

            diff_context.beginPath();
            // horizontal left
            diff_context.moveTo(bestX - armLength, bestY);
            diff_context.lineTo(bestX - gap, bestY);
            // horizontal right
            diff_context.moveTo(bestX + gap, bestY);
            diff_context.lineTo(bestX + armLength, bestY);
            // vertical top
            diff_context.moveTo(bestX, bestY - armLength);
            diff_context.lineTo(bestX, bestY - gap);
            // vertical bottom
            diff_context.moveTo(bestX, bestY + gap);
            diff_context.lineTo(bestX, bestY + armLength);
            diff_context.stroke();
        }

        console.log(`LED ${i} found at [${bestX}, ${bestY}]`);
    }
    console.log("Capture complete.", led_positions_raw);
    allOn();
}

)rawliteral";
