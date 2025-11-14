#pragma once

// Auto-generated from: merge_directions.js
const char merge_directions_js[] PROGMEM = R"rawliteral(
//given the two 2D position information, merge them into one 3D position information array and return it.
function merge_led_positions(num_leds, led_positions_raw_x, led_positions_raw_y) {
    let led_positions_raw = []
    for (const i of Array(num_leds).keys()) {
        let [xx, xz] = led_positions_raw_x[i];
        let [yy, yz] = led_positions_raw_y[i];
        // we could do something smarter with the z coordinates here. For example check for consistency between x and y captures.
        // For now we just take the average and flip it, so that the tip of the tree points up
        led_positions_raw[i] = [xx, yy, -(xz + yz) / 2];
    }
    return led_positions_raw;
}

// after we have determined the position of the leds in terms of pixels, we need to rescale them so that they are in a 1x1 box
function normalize_led_positions(num_leds, led_positions_raw, led_positions_normalized){
    let min_x = Number.POSITIVE_INFINITY;
    let min_y = Number.POSITIVE_INFINITY;
    let min_z = Number.POSITIVE_INFINITY;
    let max_x = Number.NEGATIVE_INFINITY;
    let max_y = Number.NEGATIVE_INFINITY;
    let max_z = Number.NEGATIVE_INFINITY;
    for (const p of led_positions_raw) {
        if (p[0] < min_x){
            min_x = p[0];
        }
        if (p[0] > max_x){
            max_x = p[0];
        }
        if (p[1] < min_y){
            min_y = p[1];
        }
        if (p[1] > max_y){
            max_y = p[1];
        }
        if (p[2] < min_z){
            min_z = p[2];
        }
        if (p[2] > max_z){
            max_z = p[2];
        }
    }

    // there is only one normalization constant (span), which has to work for x, y and z.
    // it is chosen such that the positions are confined on a 2x2 square centered at the coordinate origin.
    // TODO: we could center the average led position to be 0,0 in the xy plane. At the
    //   moment the tree touches -1 in both x and y direction and +1 in one of the two directions. This is kinda weird.
    let span_x = max_x - min_x;
    let span_y = max_y - min_y;
    let span = Math.max(span_x, span_y);
    for (const i of Array(num_leds).keys()) {
        const [raw_x, raw_y, raw_z] = led_positions_raw[i];
        let norm_x = (raw_x - min_x) / span;
        norm_x = norm_x * 2 - 1;  // move from space [0, 1] to space [-1, 1]
        let norm_y = (raw_y - min_y) / span;
        norm_y = norm_y * 2 - 1;  // move from space [0, 1] to space [-1, 1]
        let norm_z = (raw_z - min_z) / span;  // will be left in space [0, ...]
        led_positions_normalized[i] = [norm_x, norm_y, norm_z];
    }
    console.log("normalized pixel positions to the following values");
    console.log(led_positions_normalized);
}

// after we have determined the led positions we can tell the server our results.
// This is required for fast animations, we simply cannot control leds from the
// client device with a few ms of network delay in the way.
function transmit_led_positions(num_leds, led_positions_normalized){
    const data = {};
    for (const i of Array(num_leds).keys()) {
        data[i] = led_positions_normalized[i];
    }
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "set_led_positions", false);
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.send(JSON.stringify(data));
}


export function merge_and_transmit(num_leds, led_positions_raw_x, led_positions_raw_y, led_positions_normalized){
    console.log("merge led positions...");
    const led_positions_raw = merge_led_positions(num_leds, led_positions_raw_x, led_positions_raw_y);
    console.log("normalize led positions...");
    normalize_led_positions(num_leds, led_positions_raw, led_positions_normalized);
    console.log("transmit led positions...");
    transmit_led_positions(num_leds, led_positions_normalized);
}

)rawliteral";
