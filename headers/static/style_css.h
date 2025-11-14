#pragma once

// Auto-generated from: style.css
const char style_css[] PROGMEM = R"rawliteral(
body, html {
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

video, canvas {
    max-width: 100%;
    max-height: 100%;
    width: auto;
    height: auto;
    border-radius: 8px;
    box-shadow: 0 4px 8px rgba(0,0,0,0.2);
    object-fit: contain;
}

/* Controls container sticky at bottom */
.controls-container {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 10px;
    min-width: 180px;
    position: sticky;
    bottom: 10px;
    background-color: rgba(255,255,255,0.9);
    padding: 10px;
    border-radius: 8px;
    box-shadow: 0 4px 8px rgba(0,0,0,0.1);
    z-index: 10;
}

/* Buttons */
.controls-container button {
    padding: 10px 20px;
    font-size: 16px;
    border: none;
    border-radius: 5px;
    cursor: pointer;
}

.action-btn {
    background-color: #e74c3c;
    color: white;
}

.action-btn:hover {
    background-color: #c0392b;
}

.controls-container button:not(.action-btn) {
    background-color: #4CAF50;
    color: white;
}

.controls-container button:not(.action-btn):hover {
    background-color: #45a049;
}

.btn-group {
    display: flex;
    gap: 5px;
}

select {
    padding: 5px 10px;
    font-size: 16px;
}

/* Responsive: stack canvases on narrow screens */
@media (max-width: 1400px) {
    .main-layout {
        flex-direction: column;
        gap: 10px;
    }

    .controls-container {
        position: sticky;
        bottom: 0;
        width: 100%;
    }

    video, canvas {
        max-height: 40vh;
    }
}

)rawliteral";
