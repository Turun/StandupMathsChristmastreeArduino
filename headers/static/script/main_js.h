#pragma once

// Auto-generated from: main.js
const char main_js[] PROGMEM = R"rawliteral(
// This file coordinates the other js files and is supposed to be the only
// one that has true procedural code in it. all other files should only provide
// functional methods which can be called without relying on global state.
//
// main.js -> tell ui to set up data, manages data,
// ui.js all the dirty code to interact with the ui
// capture_unidirectional.js the methods necessary to build the pipeline to capture pixel coordinates from one direction
// merge_directions.js given two sets of pixel coordinates, merge, normalize, transmit to server

import { setup_ui } from "./ui.js";

const num_leds = 500;  // TODO: make this configurable
const led_positions_raw_x = [];
const led_positions_raw_y = [];
const led_positions_normalized = [];

// Select HTML elements
const video = document.getElementById('video');
const diff_canvas = document.getElementById("canvas-diff");
const diff_context = diff_canvas.getContext("2d", { willReadFrequently: true });

// for saving intermediate results
const math_canvas = new OffscreenCanvas(100, 100);  // actual size will be set once we have a video feed
const math_ctx = math_canvas.getContext('2d', { willReadFrequently: true });

// on load this will run
setup_ui(
    num_leds,
    video,
    math_canvas,
    diff_canvas,
    diff_context,
    led_positions_raw_x,
    led_positions_raw_y,
    led_positions_normalized,
);


)rawliteral";
