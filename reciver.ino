#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_now.h>
#include <LiquidCrystal_PCF8574.h>  // Include the LCD library
#include "custom_model.h"       // Your custom model (if needed)

// WiFi credentials for web server
const char* ssid = "Motorola";
const char* password = "123456789";
WebServer server(80);

// Initialize the I2C LCD (change address if necessary, e.g., 0x27 or 0x3F)
LiquidCrystal_PCF8574 lcd(0x27);

// Tank parameters (tankHeight is not dynamic but you can use waterLevel for measured value)
const float tankHeight = 25.0;

// Data structure to receive sensor data via ESP‑NOW
typedef struct sensor_data_struct {
    float flowRates[6];
    float pressure;
    float waterLevel;
    float pH;  // Added for pH sensor
} sensor_data_struct;

// Global instance for incoming sensor data
sensor_data_struct sensorData;

// Variables for leak detection and forecasting
float leak_prediction[3] = {0.0};

#define WINDOW_SIZE 10
float pastFlowRates[3][WINDOW_SIZE] = {{0}};
int bufferIndex[3] = {0};
bool bufferFilled[3] = {false};

// Function to forecast demand based on historical data (not used for LCD)
float forecastDemand(int pipe) {
    float sum = 0;
    int count = bufferFilled[pipe] ? WINDOW_SIZE : bufferIndex[pipe];
    for (int i = 0; i < count; i++) {
        sum += pastFlowRates[pipe][i];
    }
    return count > 0 ? sum / count : 0;
}

// Callback function executed when data is received via ESP‑NOW
void OnDataRecv(const esp_now_recv_info *recv_info, const uint8_t *incomingData, int len) {
    // Copy the incoming data into sensorData
    memcpy(&sensorData, incomingData, sizeof(sensorData));
    Serial.println("Data received via ESP‑NOW!");

    // Update leak predictions for each pipe using paired flow sensor data
    for (int pipe = 0; pipe < 3; pipe++) {
        float input = sensorData.flowRates[pipe * 2];
        float output = sensorData.flowRates[pipe * 2 + 1];
        float difference = abs(input - output);
        // If the difference is significant, mark a leak (threshold: 10% and >0.5 L/min)
        if (difference > 0.5 && difference > (input * 0.1)) {
            leak_prediction[pipe] = 1.0;
        } else {
            leak_prediction[pipe] = 0.0;
        }
    }

    // Update the circular buffer for flow rate history
    for (int pipe = 0; pipe < 3; pipe++) {
        // Average the two sensor readings for this pipe
        float avgFlow = (sensorData.flowRates[pipe * 2] + sensorData.flowRates[pipe * 2 + 1]) / 2.0;
        pastFlowRates[pipe][bufferIndex[pipe]] = avgFlow;
        bufferIndex[pipe] = (bufferIndex[pipe] + 1) % WINDOW_SIZE;
        if (bufferIndex[pipe] == 0) {
            bufferFilled[pipe] = true;
        }
    }
}

// Function to send sensor data as JSON to the web client
void sendSensorData() {
    String json = "{";
    for (int i = 0; i < 6; i++) {
        json += "\"flow" + String(i + 1) + "\":" + String(sensorData.flowRates[i], 2) + ",";
    }
    json += "\"pressure\":" + String(sensorData.pressure, 2) + ",";
    json += "\"waterLevel\":" + String(sensorData.waterLevel, 2) + ",";
    json += "\"pH\":" + String(sensorData.pH, 2) + ",";
    for (int i = 0; i < 3; i++) {
        json += "\"leak_pipe" + String(i + 1) + "\":" + String(leak_prediction[i] > 0.5 ? "true" : "false") + ",";
        json += "\"forecast_pipe" + String(i + 1) + "\":" + String(forecastDemand(i), 2);
        if (i < 2) {
            json += ",";
        }
    }
    json += "}";
    Serial.println("Sending Data: " + json);
    server.send(200, "application/json", json);
}

// Function to serve the web page 
void sendWebPage() {
    String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
        <title>ESP32 Leak Detection</title>
        <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
        <style>
            body { font-family: Arial, sans-serif; background-color: #f4f4f4; transition: 0.3s; margin: 0; padding: 0; }
            .container { display: flex; flex-direction: column; align-items: center; padding: 10px; }
            .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 15px; width: 100%; max-width: 1200px; margin-bottom: 15px; }
            .card { background: white; padding: 15px; border-radius: 10px; box-shadow: 2px 2px 10px rgba(0,0,0,0.1); text-align: center; }
            .dark-mode { background-color: #121212; color: white; }
            .dark-mode .card { background: #1e1e1e; color: white; }
            .toggle { position: fixed; top: 10px; right: 10px; cursor: pointer; padding: 10px; background: black; color: white; border-radius: 5px; }
            canvas { width: 100% !important; height: 150px !important; }
            .leak-alert { font-weight: bold; font-size: 1.1em; }
            .leak-true { color: red; animation: blink 1s infinite; }
            .leak-false { color: green; }
            .forecast-card { grid-column: span 3; }
            .forecast-container { display: flex; justify-content: space-around; margin-top: 10px; }
            .forecast-item { text-align: center; padding: 10px; border-radius: 5px; background-color: #e9f5ff; }
            .dark-mode .forecast-item { background-color: #2a3b47; }
            @keyframes blink {
                0% { opacity: 1; }
                50% { opacity: 0.5; }
                100% { opacity: 1; }
            }
        </style>
    </head>
    <body>
        <div class="toggle" onclick="toggleDarkMode()">🌙 Dark Mode</div>
        <h1 style="text-align: center;">ESP32 Leak Detection & Forecasting</h1>
        <div class="container">
            <div class="grid">
                <div class="card">
                    <h2>Leak Status</h2>
                    <p><span class="leak-alert" id="leak_pipe1">No Leak</span> in Pipe 1</p>
                    <p><span class="leak-alert" id="leak_pipe2">No Leak</span> in Pipe 2</p>
                    <p><span class="leak-alert" id="leak_pipe3">No Leak</span> in Pipe 3</p>
                </div>
                <div class="card">
                    <h2>Other Sensor Data</h2>
                    <p>Pressure: <span id="pressure">0</span> bar</p>
                    <p>Water Level: <span id="waterLevel">0</span> cm</p>
                    <p>pH: <span id="pH">0</span></p>
                </div>
                <div class="card">
                    <h2>Sensor Readings (Grouped by Pipe)</h2>
                    <p id="pipe1_data">Flow1: 0 L/min, Flow2: 0 L/min</p>
                    <p id="pipe2_data">Flow3: 0 L/min, Flow4: 0 L/min</p>
                    <p id="pipe3_data">Flow5: 0 L/min, Flow6: 0 L/min</p>
                </div>
            </div>
            
            <!-- New Demand Forecasting Card -->
            <div class="grid">
                <div class="card forecast-card">
                    <h2>Demand Forecasting</h2>
                    <p>Predicted water demand based on historical flow patterns</p>
                    <div class="forecast-container">
                        <div class="forecast-item">
                            <h3>Pipe 1</h3>
                            <div id="forecast_pipe1">0.00 L/min</div>
                        </div>
                        <div class="forecast-item">
                            <h3>Pipe 2</h3>
                            <div id="forecast_pipe2">0.00 L/min</div>
                        </div>
                        <div class="forecast-item">
                            <h3>Pipe 3</h3>
                            <div id="forecast_pipe3">0.00 L/min</div>
                        </div>
                    </div>
                </div>
            </div>
            
            <div class="grid">
                <div class="card">
                    <h3>Flow Sensor 1</h3>
                    <canvas id="chart1"></canvas>
                    <div id="flow1_display">0 L/min</div>
                </div>
                <div class="card">
                    <h3>Flow Sensor 2</h3>
                    <canvas id="chart2"></canvas>
                    <div id="flow2_display">0 L/min</div>
                </div>
                <div class="card">
                    <h3>Flow Sensor 3</h3>
                    <canvas id="chart3"></canvas>
                    <div id="flow3_display">0 L/min</div>
                </div>
                <div class="card">
                    <h3>Flow Sensor 4</h3>
                    <canvas id="chart4"></canvas>
                    <div id="flow4_display">0 L/min</div>
                </div>
                <div class="card">
                    <h3>Flow Sensor 5</h3>
                    <canvas id="chart5"></canvas>
                    <div id="flow5_display">0 L/min</div>
                </div>
                <div class="card">
                    <h3>Flow Sensor 6</h3>
                    <canvas id="chart6"></canvas>
                    <div id="flow6_display">0 L/min</div>
                </div>
            </div>
        </div>

        <script>
            let flowCharts = [];

            function fetchData() {
                fetch('/data')
                .then(response => response.json())
                .then(data => {
                    console.log("Received Data:", data);
                    
                    // Update pressure, water level, and pH
                    document.getElementById('pressure').innerText = data.pressure;
                    document.getElementById('waterLevel').innerText = data.waterLevel;
                    document.getElementById('pH').innerText = data.pH;
                    
                    // Update flow rates and charts
                    for (let i = 1; i <= 6; i++) {
                        const flowValue = data["flow" + i];
                        document.getElementById('flow' + i + '_display').innerText = flowValue + " L/min";
                        
                        // Update chart
                        flowCharts[i - 1].data.labels.push(new Date().toLocaleTimeString());
                        flowCharts[i - 1].data.datasets[0].data.push(flowValue);
                        if (flowCharts[i - 1].data.labels.length > 10) {
                            flowCharts[i - 1].data.labels.shift();
                            flowCharts[i - 1].data.datasets[0].data.shift();
                        }
                        flowCharts[i - 1].update();
                    }

                    // Update leak status
                    for (let i = 1; i <= 3; i++) {
                        const leakElement = document.getElementById('leak_pipe' + i);
                        const isLeak = data["leak_pipe" + i] === true || data["leak_pipe" + i] === "true";
                        
                        leakElement.innerText = isLeak ? "LEAK DETECTED!" : "No Leak";
                        leakElement.className = "leak-alert " + (isLeak ? "leak-true" : "leak-false");
                    }

                    // Update forecast data
                    for (let i = 1; i <= 3; i++) {
                        if (data["forecast_pipe" + i] !== undefined) {
                            document.getElementById('forecast_pipe' + i).innerText = 
                                parseFloat(data["forecast_pipe" + i]).toFixed(2) + " L/min";
                        }
                    }

                    // Update real-time sensor readings grouped by pipe
                    document.getElementById('pipe1_data').innerText = "Flow1: " + data.flow1 + " L/min, Flow2: " + data.flow2 + " L/min";
                    document.getElementById('pipe2_data').innerText = "Flow3: " + data.flow3 + " L/min, Flow4: " + data.flow4 + " L/min";
                    document.getElementById('pipe3_data').innerText = "Flow5: " + data.flow5 + " L/min, Flow6: " + data.flow6 + " L/min";
                })
                .catch(error => console.error('Error:', error));
            }

            function toggleDarkMode() {
                document.body.classList.toggle("dark-mode");
            }

            // Initialize charts and fetch initial data when the page is loaded
            window.onload = function() {
                for (let i = 1; i <= 6; i++) {
                    let ctx = document.getElementById('chart' + i).getContext('2d');
                    flowCharts.push(new Chart(ctx, {
                        type: 'line',
                        data: {
                            labels: [],
                            datasets: [{ 
                                label: "Flow " + i, 
                                data: [], 
                                borderColor: "blue", 
                                fill: false 
                            }]
                        },
                        options: { 
                            scales: { 
                                x: { display: false }, 
                                y: { min: 0 } 
                            } 
                        }
                    }));
                }
                
                // Initial data fetch
                fetchData();
                
                // Set up interval for continuous updates
                setInterval(fetchData, 1000);
            };
        </script>
    </body>
    </html>
    )rawliteral";
    server.send(200, "text/html", html);
}

// Variables for LCD display update timing
unsigned long lastLCDUpdate = 0;
int displayCycle = 0;  // cycles from 0 to 5 (6 display sets)

// Function to update the LCD display based on the current cycle
void updateLCD() {
    lcd.clear();
    char line1[17];
    char line2[17];
    
    switch(displayCycle) {
        case 0:
            // Display connection info: device connected and IP address
            snprintf(line1, 17, "Connected");
            snprintf(line2, 17, "%s", WiFi.localIP().toString().c_str());
            break;
        case 1:
            // Pipe 1: Leak status and Flow sensors 1 & 2
            snprintf(line1, 17, "P1: %s", (leak_prediction[0] > 0.5 ? "Leak" : "No Leak"));
            snprintf(line2, 17, "F1:%.1f F2:%.1f", sensorData.flowRates[0], sensorData.flowRates[1]);
            break;
        case 2:
            // Pipe 2: Leak status and Flow sensors 3 & 4
            snprintf(line1, 17, "P2: %s", (leak_prediction[1] > 0.5 ? "Leak" : "No Leak"));
            snprintf(line2, 17, "F3:%.1f F4:%.1f", sensorData.flowRates[2], sensorData.flowRates[3]);
            break;
        case 3:
            // Pipe 3: Leak status and Flow sensors 5 & 6
            snprintf(line1, 17, "P3: %s", (leak_prediction[2] > 0.5 ? "Leak" : "No Leak"));
            snprintf(line2, 17, "F5:%.1f F6:%.1f", sensorData.flowRates[4], sensorData.flowRates[5]);
            break;
        case 4:
            // Pressure and Water level
            snprintf(line1, 17, "Prs:%.1f", sensorData.pressure);
            snprintf(line2, 17, "Lvl:%.1fcm", sensorData.waterLevel);
            break;
        case 5:
            // pH value display
            snprintf(line1, 17, "pH: %.2f", sensorData.pH);
            snprintf(line2, 17, ""); // leave second line blank
            break;
        default:
            snprintf(line1, 17, " ");
            snprintf(line2, 17, " ");
            break;
    }
    
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);
}

void setup() {
    Serial.begin(115200);
    Wire.begin(21, 22);  // Use GPIO 21 (SDA) and GPIO 22 (SCL) for I2C on ESP32
    lcd.begin(16, 2);     // Initialize LCD (16 columns, 2 rows)
    lcd.setBacklight(1);  // Turn on the LCD backlight

    
    // Set device as WiFi station/AP for ESP‑NOW
    WiFi.mode(WIFI_AP_STA);
    
    // Initialize ESP‑NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP‑NOW");
        return;
    }
    esp_now_register_recv_cb(OnDataRecv);
    
    // Connect to WiFi for the web server
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    // Initialize web server routes
    server.on("/", sendWebPage);
    server.on("/data", sendSensorData);
    server.begin();
    
    // Initialize leak prediction and sensor data structure
    for (int i = 0; i < 3; i++) {
        leak_prediction[i] = 0.0;
    }
    memset(&sensorData, 0, sizeof(sensorData));
    
    
}

void loop() {
    server.handleClient();
    
    // Update the LCD display every 2 seconds
    if (millis() - lastLCDUpdate >= 2000) {
        updateLCD();
        displayCycle = (displayCycle + 1) % 6;  // Cycle through 6 display sets
        lastLCDUpdate = millis();
    }
}