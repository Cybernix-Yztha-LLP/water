#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "esp_wifi.h"  // Needed for esp_wifi_set_channel()

// WiFi credentials
const char* ssid = "Motorola";
const char* password = "123456789";

// Receiver MAC Address (Replace with the actual MAC of your receiver ESP32)
uint8_t receiverMacAddress[] = { 0xFC, 0xB4, 0x67, 0xF5, 0x18, 0xB8 };

// Flow sensor pins and related variables
const int flowSensorPins[6] = {23, 22, 32, 33,26, 25};
volatile int pulseCounts[6] = {0};
unsigned long oldTime = 0;

// Other sensor pins and parameters
const int pressureSensorPin = 27;
const int trigPin = 14;
const int echoPin = 12;
const float tankHeight = 24.0;

// pH sensor pin (assumed to be connected to analog pin 39)
const int phSensorPin = 13;

// Data structure for sensor data to be sent via ESP‑NOW
typedef struct sensor_data_struct {
  float flowRates[6];
  float pressure;
  float waterLevel;
  float pH;  // Added for pH sensor
} sensor_data_struct;

sensor_data_struct sensorData;

// ESP‑NOW peer info structure
esp_now_peer_info_t peerInfo;

// Interrupt Service Routines for flow sensors
void IRAM_ATTR pulseCounter0() { pulseCounts[0]++; }
void IRAM_ATTR pulseCounter1() { pulseCounts[1]++; }
void IRAM_ATTR pulseCounter2() { pulseCounts[2]++; }
void IRAM_ATTR pulseCounter3() { pulseCounts[3]++; }
void IRAM_ATTR pulseCounter4() { pulseCounts[4]++; }
void IRAM_ATTR pulseCounter5() { pulseCounts[5]++; }

// Callback function executed when data is sent via ESP‑NOW
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Last Packet Send Status: ");
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("Delivery Success");
  } else {
    Serial.println("Delivery Fail");
  }
}

void setup() {
  Serial.begin(115200);

  // Initialize sensor pins for flow sensors
  for (int i = 0; i < 6; i++) {
    pinMode(flowSensorPins[i], INPUT);
  }
  
  // Initialize ultrasonic sensor pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  // pH sensor pin does not require a special pinMode for analog input
  
  // Attach interrupts for flow sensors
  attachInterrupt(digitalPinToInterrupt(flowSensorPins[0]), pulseCounter0, FALLING);
  attachInterrupt(digitalPinToInterrupt(flowSensorPins[1]), pulseCounter1, FALLING);
  attachInterrupt(digitalPinToInterrupt(flowSensorPins[2]), pulseCounter2, FALLING);
  attachInterrupt(digitalPinToInterrupt(flowSensorPins[3]), pulseCounter3, FALLING);
  attachInterrupt(digitalPinToInterrupt(flowSensorPins[4]), pulseCounter4, FALLING);
  attachInterrupt(digitalPinToInterrupt(flowSensorPins[5]), pulseCounter5, FALLING);

  // Set device as a WiFi station and connect to your network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // Get the current WiFi channel
  uint8_t channel = WiFi.channel();
  Serial.print("WiFi Channel: ");
  Serial.println(channel);
  
  // Initialize ESP‑NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP‑NOW");
    return;
  }
  
  // Set WiFi channel explicitly (optional, but ensures both devices use the same channel)
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  
  // Register the send callback
  esp_now_register_send_cb(OnDataSent);
  
  // Register the receiver as a peer
  memcpy(peerInfo.peer_addr, receiverMacAddress, 6);
  peerInfo.channel = channel;  // Use the channel we got from WiFi
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  
  Serial.println("ESP‑NOW Transmitter initialized");
}

void loop() {
  // Process sensor readings and send data every second
  if (millis() - oldTime > 1000) {
    // Calculate flow rates for each sensor (conversion: pulses/7.5 = L/min)
    for (int i = 0; i < 6; i++) {
      sensorData.flowRates[i] = pulseCounts[i] / 7.5;
      pulseCounts[i] = 0;  // Reset pulse count after reading
    }
    
    // Read pressure sensor (convert ADC value to voltage)
    sensorData.pressure = analogRead(pressureSensorPin) * (3.3 / 4095.0);
    
    // Measure water level using ultrasonic sensor
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    long duration = pulseIn(echoPin, HIGH);
    sensorData.waterLevel = tankHeight - (duration * 0.034 / 2);
    
    // Read pH sensor and convert voltage to pH (calibration required)
    float phVoltage = analogRead(phSensorPin) * (5 / 4095.0);
    sensorData.pH = 7 + ((2.5 - phVoltage) / 5);
    
    // Send the sensor data via ESP‑NOW
    esp_err_t result = esp_now_send(receiverMacAddress, (uint8_t *)&sensorData, sizeof(sensorData));
    if (result == ESP_OK) {
      Serial.println("Sensor data sent successfully");
    } else {
      Serial.println("Error sending sensor data");
    }
    
  oldTime =millis();
}
}