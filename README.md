# 💧 Water Leak Detection System

This repository provides an end-to-end solution for detecting water leaks using sensor data, machine learning, and embedded systems. It covers model training, conversion to TensorFlow Lite, and integration with ESP‑NOW-based Arduino sketches for real‑time monitoring and forecasting.

---

## 📋 Table of Contents

- [Overview](#overview)
- [Repository Structure](#repository-structure)
- [Requirements](#requirements)
- [Setup and Usage](#setup-and-usage)
  - [1. Model Training & Conversion](#1-model-training--conversion)
  - [2. Embedded Deployment](#2-embedded-deployment)
- [Code Details](#code-details)
- [Contributing](#contributing)
- [License](#license)

---

## 🔍 Overview

This project is designed to:
- **Analyze sensor data** from water flow, pressure, water level, and pH sensors.
- **Train a logistic regression model** in Python to detect leaks.
- **Convert the trained model** into a TensorFlow Lite format.
- **Embed the model into microcontroller firmware** by converting it to a C header file.
- **Deploy on ESP32-based devices** using two Arduino sketches:
  - **Receiver (`reciver.ino`)**: Receives sensor data via ESP‑NOW, performs leak detection and demand forecasting, and displays results on an LCD as well as via a web dashboard.
  - **Transmitter (`transmitter.ino`)**: Reads sensor data from various sensors and sends the data to the receiver via ESP‑NOW.

---

## 🗂 Repository Structure

```plaintext
├── modelgen.py                     # Python script for data processing, model training, & TFLite conversion
├── convert_to_header.py     # Python script to convert the TFLite model into a C header file (model_data.h)
├── custom_model.h           # C++ header implementing the logistic regression leak detection model
├── model_data.h             # C header containing the TFLite model as a C array
├── sensor_data.csv          # CSV file with sensor readings and leak labels
├── reciver.ino              # Arduino sketch for receiving data, leak detection, & web/LCD display
├── transmitter.ino          # Arduino sketch for reading sensor data & transmitting via ESP‑NOW
└── README.md                # This README file
```

## 🔧 Requirements

### Software
- Python 3.x
- Required libraries:
  - numpy
  - pandas
  - scikit-learn
  - tensorflow
- Arduino IDE / PlatformIO for compiling the ESP32 sketches.

### Hardware
- ESP32-based microcontrollers (or compatible boards) for both receiver and transmitter.
- Sensors:
  - 6 Flow sensors
  - Pressure sensor
  - Ultrasonic sensor for water level measurement
  - pH sensor
- LCD display (I2C, e.g., LiquidCrystal_PCF8574)

## 🚀 Setup and Usage

### 1. Model Training & Conversion

#### Prepare Your Data
Ensure `sensor_data.csv` is placed in the repository root. It should contain the following columns:
- Flow1, Flow2, Pressure (and other required sensor values)
- Leak (target variable)

#### Train the Model
Run the Python script to load data, train the logistic regression model, and convert it to a TensorFlow Lite model:

```bash
python modelgen.py
```

This will generate `logistic_regression_model.tflite`.

#### Convert the Model to a C Header
Convert the TFLite model into a C array by running:

```bash
python convert_to_header.py
```

This creates the file `model_data.h` for embedded deployment.

### 2. Embedded Deployment

#### Receiver (reciver.ino) 📡
**Features:**
- Receives sensor data via ESP‑NOW.
- Performs leak detection on three pipes by comparing paired flow sensor readings.
- Uses a circular buffer to forecast water demand based on historical flow data.
- Hosts a web server (using Chart.js) to display real‑time sensor readings, leak status, and forecasts.
- Uses an I2C LCD display to cycle through various data (connection info, leak status, pressure, water level, and pH).

**Setup:**
- Update the WiFi credentials (SSID & password) as needed.
- Verify the LCD I2C address (e.g., 0x27).
- Compile and upload the sketch to your ESP32 using the Arduino IDE.

#### Transmitter (transmitter.ino) 📤
**Features:**
- Reads sensor data from:
  - 6 flow sensors (using interrupts to count pulses; conversion factor: pulses/7.5 = L/min)
  - Pressure sensor (via ADC)
  - Ultrasonic sensor for water level measurement (using pulse duration)
  - pH sensor (via analog input; calibration required)
- Sends the processed data via ESP‑NOW to the receiver.

**Setup:**
- Update WiFi credentials and the receiver's MAC address.
- Attach flow sensor interrupts correctly.
- Compile and upload the sketch to your ESP32 using the Arduino IDE.

## 📖 Code Details

**Python Scripts:**
- `modelgen.py`: Handles data loading, feature scaling, model training, and TFLite conversion.
- `convert_to_header.py`: Converts the TFLite model into a C header file (`model_data.h`).

**C/C++ Headers:**
- `custom_model.h`: Contains a C++ class `LeakDetectionModel` that implements the logistic regression model with pre-calculated means, standard deviations, weights, and bias values.

**Arduino Sketches:**
- `reciver.ino`: Configures WiFi, ESP‑NOW reception, a web server, and an LCD display to present sensor data and leak predictions.
- `transmitter.ino`: Configures sensor inputs, processes sensor readings, and sends data via ESP‑NOW.

## 🤝 Contributing

Contributions are very welcome! To contribute:

1. Fork the repository.
2. Create a new branch for your feature or bug fix.
3. Commit your changes with clear messages.
4. Open a pull request for review.

## 📜 License

This project is licensed under the MIT License. See the LICENSE file for details.

