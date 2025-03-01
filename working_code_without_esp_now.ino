// working code without esp now 
/*
 #include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "custom_model.h"

const char* ssid = "Motorola";
const char* password = "123456789";
WebServer server(80);

const int pressureSensorPin = 27;
const int trigPin = 14;
const int echoPin = 12;
const float tankHeight = 25.0;

// Flow sensors for 3 pipes (2 sensors per pipe)
const int flowSensorPins[6] = {34, 35, 32, 33, 26, 25};
volatile int pulseCounts[6] = {0};
unsigned long oldTime = 0;
float flowRates[6] = {0.0};
float pressure = 0.0;
float waterLevel = 0.0;
float leak_prediction[3] = {0.0};

#define WINDOW_SIZE 10
float pastFlowRates[3][WINDOW_SIZE] = {{0}};
int bufferIndex[3] = {0};
bool bufferFilled[3] = {false};

void IRAM_ATTR pulseCounter0() { pulseCounts[0]++; }
void IRAM_ATTR pulseCounter1() { pulseCounts[1]++; }
void IRAM_ATTR pulseCounter2() { pulseCounts[2]++; }
void IRAM_ATTR pulseCounter3() { pulseCounts[3]++; }
void IRAM_ATTR pulseCounter4() { pulseCounts[4]++; }
void IRAM_ATTR pulseCounter5() { pulseCounts[5]++; }

float forecastDemand(int pipe) {
    float sum = 0;
    int count = bufferFilled[pipe] ? WINDOW_SIZE : bufferIndex[pipe];
    for (int i = 0; i < count; i++) {
        sum += pastFlowRates[pipe][i];
    }
    return count > 0 ? sum / count : 0;
}

void sendSensorData() {
    String json = "{";
    for (int i = 0; i < 6; i++) {
        json += "\"flow" + String(i+1) + "\":" + String(flowRates[i], 2) + ",";
    }
    json += "\"pressure\":" + String(pressure, 2) + ",";
    json += "\"waterLevel\":" + String(waterLevel, 2) + ",";
    for (int i = 0; i < 3; i++) {
        json += "\"leak_pipe" + String(i+1) + "\":" + String(leak_prediction[i] > 0.5 ? "true" : "false") + ",";
        json += "\"forecast_pipe" + String(i+1) + "\":" + String(forecastDemand(i), 2);
        if (i < 2) {
            json += ",";
        }
    }
    json += "}";

    // Debugging output to Serial Monitor
    Serial.println("Sending Data: " + json);

    server.send(200, "application/json", json);
}


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
                    
                    // Update pressure and water level
                    document.getElementById('pressure').innerText = data.pressure;
                    document.getElementById('waterLevel').innerText = data.waterLevel;

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

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }

    for (int i = 0; i < 6; i++) {
        pinMode(flowSensorPins[i], INPUT);
    }
    pinMode(pressureSensorPin, INPUT);
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(flowSensorPins[0]), pulseCounter0, FALLING);
    attachInterrupt(digitalPinToInterrupt(flowSensorPins[1]), pulseCounter1, FALLING);
    attachInterrupt(digitalPinToInterrupt(flowSensorPins[2]), pulseCounter2, FALLING);
    attachInterrupt(digitalPinToInterrupt(flowSensorPins[3]), pulseCounter3, FALLING);
    attachInterrupt(digitalPinToInterrupt(flowSensorPins[4]), pulseCounter4, FALLING);
    attachInterrupt(digitalPinToInterrupt(flowSensorPins[5]), pulseCounter5, FALLING);

    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    server.on("/", sendWebPage);
    server.on("/data", sendSensorData);
    server.begin();
    
    // Initialize leak prediction array
    for (int i = 0; i < 3; i++) {
        leak_prediction[i] = 0.0;
    }
    
    // Update buffer for flow rate history
    for (int pipe = 0; pipe < 3; pipe++) {
        // Calculate average flow rate for this pipe (using both sensors)
        float avgFlow = (flowRates[pipe*2] + flowRates[pipe*2+1]) / 2.0;
        
        // Store in circular buffer
        pastFlowRates[pipe][bufferIndex[pipe]] = avgFlow;
        bufferIndex[pipe] = (bufferIndex[pipe] + 1) % WINDOW_SIZE;
        if (bufferIndex[pipe] == 0) {
            bufferFilled[pipe] = true;
        }
    }
}

void loop() {
    server.handleClient();
    
    if (millis() - oldTime > 1000) { // Calculate every second
        for (int i = 0; i < 6; i++) {
            flowRates[i] = (pulseCounts[i] / 7.5);  // Convert pulses to L/min
            pulseCounts[i] = 0;  // Reset pulse count
        }
        
        pressure = analogRead(pressureSensorPin) * (3.3 / 4095.0);  // Convert to voltage
        
        // Measure water level using ultrasonic sensor
        digitalWrite(trigPin, LOW);
        delayMicroseconds(2);
        digitalWrite(trigPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(trigPin, LOW);
        
        long duration = pulseIn(echoPin, HIGH);
        waterLevel = tankHeight - (duration * 0.034 / 2);  // Calculate water level in cm
        
        // Update leak predictions (simplified example - in real use would call ML model)
        // Here we're just checking if input flow significantly differs from output flow
        for (int pipe = 0; pipe < 3; pipe++) {
            float input = flowRates[pipe*2];
            float output = flowRates[pipe*2+1];
            float difference = abs(input - output);
            
            // Detect leak if difference is more than 10% of input flow and above 0.5 L/min threshold
            if (difference > 0.5 && difference > (input * 0.1)) {
                leak_prediction[pipe] = 1.0;
            } else {
                leak_prediction[pipe] = 0.0;
            }
        }
        
        // Update buffer for flow rate history
        for (int pipe = 0; pipe < 3; pipe++) {
            // Calculate average flow rate for this pipe (using both sensors)
            float avgFlow = (flowRates[pipe*2] + flowRates[pipe*2+1]) / 2.0;
            
            // Store in circular buffer
            pastFlowRates[pipe][bufferIndex[pipe]] = avgFlow;
            bufferIndex[pipe] = (bufferIndex[pipe] + 1) % WINDOW_SIZE;
            if (bufferIndex[pipe] == 0) {
                bufferFilled[pipe] = true;
            }
        }
        
        oldTime = millis();
    }
} 
 */
