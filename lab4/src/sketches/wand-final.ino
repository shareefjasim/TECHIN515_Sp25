#include <Wand_inferencing.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <math.h> // for sin()

// === LED Pins ===
#define RED_PIN    2   // GPIO2
#define GREEN_PIN  3   // GPIO3
#define BLUE_PIN   4   // GPIO4

// === Button Pin ===
#define BUTTON_PIN 20  // D7 (GPIO20)

// === Constants for LED fading ===
#define FADE_STEPS 50
#define FADE_DELAY 20  // ms per step

// === MPU6050 ===
Adafruit_MPU6050 mpu;

// Sampling & inference variables
#define SAMPLE_RATE_MS 10
#define CAPTURE_DURATION_MS 1000
#define FEATURE_SIZE EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE

bool capturing = false;
unsigned long last_sample_time = 0;
unsigned long capture_start_time = 0;
int sample_count = 0;
float features[FEATURE_SIZE];

int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, features + offset, length * sizeof(float));
    return 0;
}

void setup() {
    Serial.begin(115200);

    // LED pins
    pinMode(RED_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);

    // Button pin
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // MPU6050 setup
    Serial.println("Initializing MPU6050...");
    if (!mpu.begin()) {
        Serial.println("Failed to find MPU6050 chip");
        while (1) delay(10);
    }

    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    Serial.println("MPU6050 initialized. Press button to start gesture recognition.");
}

void loop() {
    // Check button press (active LOW)
    if (digitalRead(BUTTON_PIN) == LOW && !capturing) {
        Serial.println("Button pressed! Starting gesture capture...");
        sample_count = 0;
        capturing = true;
        capture_start_time = millis();
        last_sample_time = millis();
        delay(200);  // debounce
    }

    // Capture data if we’re in capture mode
    if (capturing) {
        capture_accelerometer_data();
    }
}
void capture_accelerometer_data() {
    if (millis() - last_sample_time >= SAMPLE_RATE_MS) {
        last_sample_time = millis();

        sensors_event_t a, g, temp;
        mpu.getEvent(&a, &g, &temp);

        if (sample_count < FEATURE_SIZE / 3) {
            int idx = sample_count * 3;
            features[idx] = a.acceleration.x;
            features[idx + 1] = a.acceleration.y;
            features[idx + 2] = a.acceleration.z;
            sample_count++;
        }

        // Trigger inference once we have enough samples
        if (sample_count >= FEATURE_SIZE / 3) {
            capturing = false;
            Serial.println("Capture complete");
            run_inference();
        }
    }
}


void run_inference() {
    if (sample_count * 3 < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
        Serial.println("ERROR: Not enough data for inference");
        return;
    }

    ei_impulse_result_t result = { 0 };
    signal_t features_signal;
    features_signal.total_length = FEATURE_SIZE;
    features_signal.get_data = &raw_feature_get_data;

    EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false);
    if (res != EI_IMPULSE_OK) {
        Serial.print("Classifier failed: "); Serial.println(res);
        return;
    }

    // Highest probability gesture
    float max_val = 0;
    int max_idx = -1;
    for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        if (result.classification[i].value > max_val) {
            max_val = result.classification[i].value;
            max_idx = i;
        }
    }

    if (max_idx != -1) {
        String gesture = ei_classifier_inferencing_categories[max_idx];
        Serial.print("Prediction: "); Serial.print(gesture);
        Serial.print(" ("); Serial.print(max_val * 100); Serial.println("%)");

        if (gesture == "Z") {
            fadeLED(RED_PIN);
        } else if (gesture == "O") {
            fadeLED(BLUE_PIN);
        } else if (gesture == "V") {
            fadeLED(GREEN_PIN);
        }
    }
}

// Smooth fade in and out like a bell curve using sine wave
void fadeLED(int pin) {
    for (int i = 0; i <= FADE_STEPS; i++) {
        float bell = sin(PI * i / FADE_STEPS);  // sine from 0 to PI
        int value = (int)(bell * 255);
        analogWrite(pin, value);
        delay(FADE_DELAY);
    }

    delay(200);  // hold at peak

    for (int i = FADE_STEPS; i >= 0; i--) {
        float bell = sin(PI * i / FADE_STEPS);
        int value = (int)(bell * 255);
        analogWrite(pin, value);
        delay(FADE_DELAY);
    }

    analogWrite(pin, 0);  // make sure it’s off
}
