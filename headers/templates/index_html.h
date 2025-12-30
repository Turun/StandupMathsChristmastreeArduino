#pragma once

// Auto-generated from: index.html
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>

<head>
  <title>Camera Capture App</title>
  <style>
    body,
    html {
      margin: 0;
      padding: 0;
      width: 100%;
      height: 100%;
      font-family: Arial, sans-serif;
      background-color: #f4f4f9;
      display: flex;
      justify-content: center;
      align-items: center;
    }

    .container {
      display: flex;
      flex-direction: column;
      width: 100%;
      height: 100%;
      padding: 20px;
      box-sizing: border-box;
      text-align: center;
    }

    /* Layout for canvases and controls */
    .main-layout {
      display: flex;
      flex: 1 1 auto;
      justify-content: space-between;
      align-items: stretch;
      gap: 20px;
      height: 100%;
    }

    /* Canvas containers */
    .canvas-container {
      flex: 1 1 0;
      display: flex;
      justify-content: center;
    }

    video,
    canvas {
      max-width: 100%;
      max-height: 100%;
      width: auto;
      height: auto;
      border-radius: 8px;
      box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);
      object-fit: contain;
    }

    /* Controls container */
    .controls-container {
      display: flex;
      flex-direction: column;
      min-width: 220px;
      max-width: 250px;
      background-color: rgba(255, 255, 255, 0.9);
      border-radius: 8px;
      box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
      overflow: hidden;
    }

    /* Tab navigation */
    .tab-nav {
      display: flex;
      background-color: #e0e0e0;
      border-bottom: 2px solid #ccc;
    }

    .tab-btn {
      flex: 1;
      padding: 12px 8px;
      font-size: 14px;
      font-weight: 600;
      border: none;
      background-color: #e0e0e0;
      color: #555;
      cursor: pointer;
      transition: all 0.3s;
      border-right: 1px solid #ccc;
    }

    .tab-btn:last-child {
      border-right: none;
    }

    .tab-btn:hover {
      background-color: #d0d0d0;
    }

    .tab-btn.active {
      background-color: #fff;
      color: #333;
      border-bottom: 2px solid #4CAF50;
      margin-bottom: -2px;
    }

    /* Tab content area */
    .tab-content {
      flex: 1;
      overflow-y: auto;
      padding: 15px;
    }

    .tab-panel {
      display: none;
      flex-direction: column;
      gap: 10px;
      align-items: stretch;
    }

    .tab-panel.active {
      display: flex;
    }

    /* Common controls section */
    .common-controls {
      display: flex;
      flex-direction: column;
      gap: 10px;
      padding-bottom: 15px;
      border-bottom: 2px solid #e0e0e0;
      margin-bottom: 10px;
    }

    .common-controls.hidden {
      display: none;
    }

    /* Buttons */
    .controls-container button,
    .tab-panel button {
      padding: 10px 20px;
      font-size: 16px;
      border: none;
      border-radius: 5px;
      cursor: pointer;
      width: 100%;
    }

    .action-btn {
      background-color: #e74c3c;
      color: white;
    }

    .action-btn:hover {
      background-color: #c0392b;
    }

    .blue-btn {
      background-color: #3498db;
      color: white;
    }

    .blue-btn:hover {
      background-color: #2980b9;
    }

    .controls-container button:not(.action-btn):not(.blue-btn):not(.tab-btn),
    .tab-panel button:not(.action-btn):not(.blue-btn) {
      background-color: #4CAF50;
      color: white;
    }

    .controls-container button:not(.action-btn):not(.blue-btn):not(.tab-btn):hover,
    .tab-panel button:not(.action-btn):not(.blue-btn):hover {
      background-color: #45a049;
    }

    .btn-group {
      display: flex;
      gap: 5px;
      align-items: stretch;
    }

    .color-picker-wrapper {
      display: flex;
      align-items: center;
      background-color: #4CAF50;
      border-radius: 5px;
      padding: 0 5px;
      cursor: pointer;
    }

    #base-color-picker {
      width: 40px;
      height: 30px;
      border: none;
      background: none;
      cursor: pointer;
      padding: 0;
    }

    .color-picker-wrapper:hover {
      background-color: #45a049;
    }

    select {
      padding: 5px 10px;
      font-size: 16px;
      flex: 1;
    }

    label {
      font-size: 14px;
      font-weight: 600;
      color: #555;
      margin-top: 5px;
    }

    /* Responsive: stack canvases on narrow screens */
    @media (max-width: 1400px) {
      .main-layout {
        flex-direction: column;
        gap: 10px;
      }

      .controls-container {
        max-width: 100%;
        width: 100%;
      }

      video,
      canvas {
        max-height: 40vh;
      }
    }
  </style>
</head>

<body>
  <div class="container">
    <div class="main-layout">
      <!-- Video canvas -->
      <div class="canvas-container">
        <video id="video" autoplay></video>
      </div>

      <!-- Controls with Tabs -->
      <div class="controls-container">
        <!-- Tab Navigation -->
        <div class="tab-nav">
          <button class="tab-btn active" data-tab="x-coords" id="nav-tab-x">X</button>
          <button class="tab-btn" data-tab="y-coords" id="nav-tab-y">Y</button>
          <button class="tab-btn" data-tab="effects" id="nav-tab-effects">Effects</button>
        </div>

        <!-- Tab Content -->
        <div class="tab-content">
          <!-- Common Controls (shown in X and Y tabs only) -->
          <div class="common-controls" id="common-controls">
            <label for="led-select">Select LED:</label>
            <div class="btn-group">
              <button id="btn-dec">&lt;</button>
              <select id="led-select"></select>
              <button id="btn-inc">&gt;</button>
            </div>
            <button id="effect-all-on-btn">All On</button>
            <div class="btn-group">
              <button id="mask-led-btn">Mask</button>
              <button id="unmask-led-btn">Unmask</button>
            </div>
            <button id="unmask-all-btn" class="action-btn">Unmask All</button>
            <button id="transmit-btn" class="action-btn">Transmit 3D pos</button>
          </div>

          <!-- X Coordinates Tab -->
          <div class="tab-panel active" id="x-coords">
            <button id="start-btn-x">Capture X Coords</button>
            <button id="center-led-position-in-x-btn" class="action-btn">Fix Position in X</button>
          </div>

          <!-- Y Coordinates Tab -->
          <div class="tab-panel" id="y-coords">
            <button id="start-btn-y">Capture Y Coords</button>
            <button id="center-led-position-in-y-btn" class="action-btn">Fix Position in Y</button>
          </div>

          <!-- Effects Tab -->
          <div class="tab-panel" id="effects">
            <div class="btn-group">
              <button id="effect-blink-btn" class="blue-btn">Blink</button>
              <button id="effect-all-on-btn2" class="blue-btn">All On</button>
              <div class="color-picker-wrapper">
                <input type="color" id="base-color-picker" value="#ffffff" title="Set LED Color">
              </div>
            </div>
            <button id="effect-sweeping-plane-btn">Sweeping Plane Effect</button>
            <button id="effect-sweeping-plane-x-btn">Sweeping Plane X</button>
            <button id="effect-sweeping-plane-y-btn">Sweeping Plane Y</button>
            <button id="effect-sweeping-plane-z-btn">Sweeping Plane Z</button>
            <button id="effect-concentric-color-btn">Concentric Color</button>
            <button id="effect-stop-btn" class="action-btn">Stop Effects</button>
          </div>
        </div>
      </div>

      <!-- Diff canvas -->
      <div class="canvas-container">
        <canvas id="canvas-diff"></canvas>
      </div>
    </div>
  </div>

  <script>
    // Tab switching functionality
    const commonControls = document.getElementById('common-controls');

    document.querySelectorAll('.tab-btn').forEach(button => {
      button.addEventListener('click', () => {
        const tabName = button.getAttribute('data-tab');

        // Remove active class from all buttons and panels
        document.querySelectorAll('.tab-btn').forEach(btn => btn.classList.remove('active'));
        document.querySelectorAll('.tab-panel').forEach(panel => panel.classList.remove('active'));

        // Add active class to clicked button and corresponding panel
        button.classList.add('active');
        document.getElementById(tabName).classList.add('active');

        // Show/hide common controls based on active tab
        if (tabName === 'effects') {
          commonControls.classList.add('hidden');
        } else {
          commonControls.classList.remove('hidden');
        }
      });
    });
  </script>

  <script type="module" src="static/script/main.js"></script>
</body>

</html>
)rawliteral";
