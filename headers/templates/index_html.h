#pragma once

// Auto-generated from: index.html
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>

<head>
  <title>Camera Capture App</title>
  <link rel="stylesheet" href="static/style.css">
</head>

<body>
  <div class="container">
    <div class="main-layout">
      <!-- Video canvas -->
      <div class="canvas-container">
        <video id="video" autoplay></video>
      </div>

      <!-- Controls -->
      <div class="controls-container">
        <button id="start-btn-x" class="action-btn">Capture X Coords</button>
        <button id="start-btn-y" class="action-btn">Capture Y Coords</button>
        <label for="led-select">Select LED:</label>
        <select id="led-select"></select>
        <button disabled id="overview-btn-x">Show Overview X</button>
        <button disabled id="overview-btn-y">Show Overview Y</button>
        <button id="transmit-btn" class="action-btn">Transmit 3D pos</button>
        <div class="btn-group">
          <button id="effect-blink-btn" class="blue-btn">Blink</button>
          <button id="effect-all-on-btn" class="blue-btn">All On</button>
          <div class="color-picker-wrapper">
            <input type="color" id="base-color-picker" value="#ffffff" title="Set LED Color">
          </div>
        </div>
        <button id="effect-sweeping-plane-btn">Sweeping Plane Effect</button>
        <button id="effect-stop-btn" class="action-btn">Stop Effects</button>
      </div>

      <!-- Diff canvas -->
      <div class="canvas-container">
        <canvas id="canvas-diff"></canvas>
      </div>
    </div>
  </div>
  <!-- <script src="static/script.js"></script> -->
  <script type="module" src="static/script/main.js"></script>
</body>

</html>
)rawliteral";
