#pragma once

// Auto-generated from: capture_unidirectional.js
const char capture_unidirectional_js[] PROGMEM = R"rawliteral(
import { visualize_led_positions } from "./ui.js";


// const cycle_base = 9;
// const num_leds_log_cycle_base = Math.log(num_leds) / Math.log(cycle_base);
// const num_cycles = cycle_base * Math.ceil(num_leds_log_cycle_base);
// // returns true if the led should turn on for this cycle, false if not.
// function is_led_on(led_index, cycle) {
//     let major_cycle = Math.floor(cycle / cycle_base);
//     let minor_cycle = cycle % cycle_base;

//     // shift right, so that the digit to check is the left most if presented in base cycle_base
//     let number = Math.floor(led_index / Math.pow(cycle_base, major_cycle));
//     // check on that right most digit now
//     return number % cycle_base === minor_cycle;
// }


// returns true if the led should turn on for this cycle, false if not.
export function is_led_on(led_index, cycle) {
    // turn on leds one by one.
    return led_index === cycle;
}

// for many led detection cycles we have much more off images than on images.
// Thus we need to weigh them so that unaffected regions of the image come
// out as zero after adding all on and all weighted off images.
function off_weight(led_index, num_cycles) {
    let count_on = 0;
    let count_off = 0;
    for (const cycle of Array(num_cycles).keys()) {
        if (is_led_on(led_index, cycle)) {
            count_on += 1;
        } else {
            count_off += 1;
        }
    }
    return count_on / count_off;
}

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


// tells the server which leds to light up for configuration.
// argument is a dictionary of <led_index>: <bool on> values.
// blocks.
function configure_leds(dict) {
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "configure_leds", false);
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.send(JSON.stringify(dict));
}

// main code block, tell the pi what to do, take a picture and then move on.
function capture_lock_in_data(num_leds, num_cycles, video, contexts){
    for (const shift of Array(num_cycles).keys()) {
        var data = {};
        for (const led_index of Array(num_leds).keys()) {
            data[led_index] = is_led_on(led_index, shift);
        }
        configure_leds(data);
        contexts[shift].drawImage(video, 0, 0, video.videoWidth, video.videoHeight);
    }
}

// convert all images to greyscale
function convert_to_greyscale(contexts){
    for (const ctx of contexts) {
        let image_data = ctx.getImageData(0, 0, ctx.canvas.width, ctx.canvas.height);
        for (let i = 0; i < image_data.data.length; i += 4) {
            let r = image_data.data[i];
            let g = image_data.data[i+1];
            let b = image_data.data[i+2];
            let _a = image_data.data[i+3];
            let brightness = Math.sqrt(0.241 * r * r + 0.691 * g * g  + 0.068 * b * b);  // weighted sum of color values
            image_data.data[i] = brightness;
            image_data.data[i+1] = brightness;
            image_data.data[i+2] = brightness;
        }
        ctx.putImageData(image_data, 0, 0);
    }
}

// blur all images with adjustable kernel size
function blur_images(contexts, half_kernel_size = 4) {
    if (half_kernel_size < 1) half_kernel_size = 1;

    for (const ctx of contexts) {
        const width = ctx.canvas.width;
        const height = ctx.canvas.height;

        const src = ctx.getImageData(0, 0, width, height);
        const dst = ctx.createImageData(width, height);

        const srcData = src.data;
        const dstData = dst.data;

        // loop through every pixel
        for (let y = 0; y < height; y++) {
            for (let x = 0; x < width; x++) {
                let r = 0, g = 0, b = 0, a = 0;
                let count = 0;

                // average over the kernel neighborhood
                for (let ky = -half_kernel_size; ky <= half_kernel_size; ky++) {
                    for (let kx = -half_kernel_size; kx <= half_kernel_size; kx++) {
                        const px = x + kx;
                        const py = y + ky;

                        if (px >= 0 && px < width && py >= 0 && py < height) {
                            const idx = (py * width + px) * 4;
                            r += srcData[idx];
                            g += srcData[idx + 1];
                            b += srcData[idx + 2];
                            a += srcData[idx + 3];
                            count++;
                        }
                    }
                }

                const i = (y * width + x) * 4;
                dstData[i] = r / count;
                dstData[i + 1] = g / count;
                dstData[i + 2] = b / count;
                dstData[i + 3] = a / count;
            }
        }

        ctx.putImageData(dst, 0, 0);
    }
}

// for every values in base, add the value of the addition image (in the red channel, presumed to be converted to greyscale before)
function add(base, addition) {
    for (let i = 0; i < base.length; i += 1) {
        base[i] += addition.data[i * 4];
    }
}

// for every values in base, subtract the value of the subtraction image (in the red channel, presumed to be converted to greyscale before)
//
// The subtracted image is weighted, to ensure parts of the image that are unaffected by the led come out to approximately 0, not -1000 or something.
function sub(base, subtraction, weight) {
    for (let i = 0; i < base.length; i += 1) {
        base[i] -= subtraction.data[i * 4] * weight;
    }
}

export function compute_lock_in_image_data_array(led_index, num_cycles, math_canvas, contexts) {
    let image_data = new Float32Array(math_canvas.width * math_canvas.height);
    for (const shift of Array(num_cycles).keys()){
        const ctx = contexts[shift];
        const img = ctx.getImageData(0, 0, math_canvas.width, math_canvas.height);

        if (is_led_on(led_index, shift)){
            add(image_data, img);
        } else {
            let this_off_weight = off_weight(led_index, num_cycles);
            sub(image_data, img, this_off_weight);
        }
    }
    return image_data;
}

// for every led, add all images where it was turned on and subtract all images where it was turned off
function analyze_lock_in_data(num_leds, num_cycles, math_canvas, contexts, led_positions_raw) {
    for (const led_index of Array(num_leds).keys()) {
        console.log("analyzing data for LED " + led_index);
        const image_data = compute_lock_in_image_data_array(
            led_index, num_cycles, math_canvas, contexts
        );
        
        // now we have done the lock in amplification. The brightest pixel should now be the location of the LED.
        // (smarter detection algs may be possible)
        let max_brightness = 0;
        let x = 0;
        let y = 0;
        for (let i = 0; i < image_data.length; i += 1) {
            let brightness = image_data[i];
            if (brightness > max_brightness) {
                max_brightness = brightness;
                x = i % math_canvas.width;
                y = Math.floor(i / math_canvas.width);
            }
        }
        led_positions_raw[led_index] = [x, y];
    }
    console.log("got the following LED positions in pixel coordinates:");
    console.log(led_positions_raw);
}


export function start_capturing(
    num_leds,
    num_cycles,
    video,
    contexts,
    math_canvas,
    diff_canvas,
    diff_context,
    led_positions_raw,
){
    console.log("starting...");
    console.log("generating lock in data...");
    capture_lock_in_data(num_leds, num_cycles, video, contexts);
    console.log("converting images to greyscale...");
    convert_to_greyscale(contexts);
    console.log("bluring images...");
    blur_images(contexts);
    console.log("analyzing lock in data...");
    analyze_lock_in_data(num_leds, num_cycles, math_canvas, contexts, led_positions_raw);
    console.log("visualizing LED positions...");
    visualize_led_positions(
        diff_context,
        diff_canvas,
        led_positions_raw,
    );
    console.log("done!!");
}


)rawliteral";
