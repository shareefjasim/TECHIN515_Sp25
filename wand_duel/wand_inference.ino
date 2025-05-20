/* Edge Impulse ingestion SDK
 * Copyright (c) 2022 EdgeImpulse Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

/* Includes ---------------------------------------------------------------- */
#include <test_wand_inferencing.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// WiFi credentials 
const char* ssid = "UW MPSK";
const char* password = "5Gp5iN(!^7";

// Server details 
const char* serverUrl = "http://192.168.1.159:8080/predict";

// Student identifier - set this to the student's UWNetID
const char* studentId = "REPLACE_WITH_UWNetID";

// MPU6050 sensor
Adafruit_MPU6050 mpu;

// Sampling and capture variables
#define SAMPLE_RATE_MS 10  // 100Hz sampling rate (10ms between samples)
#define CAPTURE_DURATION_MS 1000  // 1 second capture
#define FEATURE_SIZE EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE  // Size of the feature array

// Capture state variables
bool capturing = false;
unsigned long last_sample_time = 0;
unsigned long capture_start_time = 0;
int sample_count = 0;

// Feature array to store accelerometer data
float features[FEATURE_SIZE];

/**
 * @brief      Copy raw feature data in out_ptr
 *
 * @param[in]  offset   The offset
 * @param[in]  length   The length
 * @param      out_ptr  The out pointer
 *
 * @return     0
 */
int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, features + offset, length * sizeof(float));
    return 0;
}

/**
 * Setup WiFi connection
 */
void setupWiFi() {
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    
    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void setup()
{
    // Initialize serial
    Serial.begin(115200);
    
    while (!Serial);
    Serial.println("Edge Impulse Inferencing Demo");
    
    // Initialize MPU6050
    Serial.println("Initializing MPU6050...");
    if (!mpu.begin()) {
        Serial.println("Failed to find MPU6050 chip");
        while (1) {
            delay(10);
        }
    }
    
    // Configure MPU6050 - match settings with gesture_capture.ino
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    
    Serial.println("MPU6050 initialized successfully");
    
    // Setup WiFi
    setupWiFi();
    
    Serial.println("Send 'o' to start gesture capture");
}

/**
 * @brief      Capture accelerometer data for inference
 */
void capture_accelerometer_data() {
    if (millis() - last_sample_time >= SAMPLE_RATE_MS) {
        last_sample_time = millis();
        
        // Get accelerometer data
        sensors_event_t a, g, temp;
        mpu.getEvent(&a, &g, &temp);
        
        // Store data in features array (x, y, z, x, y, z, ...)
        if (sample_count < FEATURE_SIZE / 3) {
            int idx = sample_count * 3;
            features[idx] = a.acceleration.x;
            features[idx + 1] = a.acceleration.y;
            features[idx + 2] = a.acceleration.z;
            sample_count++;
        }
        
        // Check if capture duration has elapsed
        if (millis() - capture_start_time >= CAPTURE_DURATION_MS) {
            capturing = false;
            Serial.println("Capture complete");
            
            // Run inference on captured data
            run_inference();
        }
    }
}

/**
 * @brief      Send result to server
 */
void sendGestureToServer(const char* gesture, float confidence) {
    // Create JSON payload
    String jsonPayload = "{";
    jsonPayload += "\"student_id\":";
    jsonPayload += "\"";
    jsonPayload += studentId;
    jsonPayload += "\",";
    jsonPayload += "\"gesture\":";
    jsonPayload += "\"";
    jsonPayload += gesture;
    jsonPayload += "\",";
    jsonPayload += "\"confidence\":";
    jsonPayload += confidence;
    jsonPayload += "}";
    
    Serial.println("\n--- Sending Prediction to Server ---");
    Serial.println("URL: " + String(serverUrl));
    Serial.println("Payload: " + jsonPayload);
    
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");
    
    // Send POST request
    int httpResponseCode = http.POST(jsonPayload);
    
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    
    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println("Server response: " + response);
    } else {
        Serial.printf("Error sending POST: %s\n", http.errorToString(httpResponseCode).c_str());
    }
    
    http.end();
    Serial.println("--- End of Request ---\n");
}

/**
 * @brief      Run inference on the captured data
 */
void run_inference() {
    // Check if we have enough data
    if (sample_count * 3 < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
        Serial.println("ERROR: Not enough data for inference");
        return;
    }
    
    ei_impulse_result_t result = { 0 };

    // Create signal from features array
    signal_t features_signal;
    features_signal.total_length = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
    features_signal.get_data = &raw_feature_get_data;

    // Run the classifier
    EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false /* debug */);
    if (res != EI_IMPULSE_OK) {
        Serial.print("ERR: Failed to run classifier (");
        Serial.print(res);
        Serial.println(")");
        return;
    }

    // Print and send inference result
    print_inference_result(result);
}

void print_inference_result(ei_impulse_result_t result) {
    // Find the prediction with highest confidence
    float max_value = 0;
    int max_index = -1;
    
    for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        if (result.classification[i].value > max_value) {
            max_value = result.classification[i].value;
            max_index = i;
        }
    }
    
    // Only print the prediction with highest confidence
    if (max_index != -1) {
        String gesture = String(ei_classifier_inferencing_categories[max_index]);
        float confidence = max_value * 100;
        
        // Print to serial
        Serial.print("Prediction: ");
        Serial.print(gesture);
        Serial.print(" (");
        Serial.print(confidence);
        Serial.println("%)");
        
        // Send to server
        sendGestureToServer(gesture.c_str(), confidence);
    }
}

void loop()
{
    // Check for serial commands
    if (Serial.available() > 0) {
        char cmd = Serial.read();
        if (cmd == 'o') {
            // Start capturing data
            Serial.println("Starting gesture capture...");
            sample_count = 0;
            capturing = true;
            capture_start_time = millis();
            last_sample_time = millis();
        }
    }
    
    // Capture data if in capturing mode
    if (capturing) {
        capture_accelerometer_data();
    }
}