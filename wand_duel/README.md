# Wand Duel Competition

This project enables a magical wand duel competition between two ESP32 devices. Each ESP32 runs gesture recognition from lab 5 to detect wand movements, and the results are sent to a central server that determines the winner.

## Learning Objectives

Through the wand compeption, students should 
- Get familiar with sending data from edge (ESP32) to server;
- Validate the magic wand built in previous lab.

## Setup Instructions

Please follow the steps below to enable your magic wand to talk to the server. An example **yet incomplete** sketch is provided for your reference.

### Your Tasks as ESP32 Clients

1. Install the required libraries in Arduino IDE:
   - ArduinoJson (by Benoit Blanchon)
   - Adafruit MPU6050 (by Adafruit)
   - Adafruit Unified Sensor (by Adafruit)
   - Your Edge Impulse library for gesture recognition
   - You should have the following includes
    ```cpp
    #include <test_wand_inferencing.h>
    #include <Adafruit_MPU6050.h>
    #include <Adafruit_Sensor.h>
    #include <Wire.h>
    #include <WiFi.h>
    #include <HTTPClient.h>
    #include <ArduinoJson.h>
    ```
    **Note**: Please replace `<test_wand_inferencing.h>` with appropriate names from your Edge Impulse downloads

2. Find our the MAC address of your ESP32. You can either follow the [tutorial](https://randomnerdtutorials.com/get-change-esp32-esp8266-mac-address-arduino/#:~:text=rate%20of%20115200.-,Press%20the%20on%2Dboard%20RESET%20or%20EN%20button.,printed%20in%20the%20Serial%20Monitor.) here or by reading the seriel monitor of Arduino IDE after uploading your code to dev board.

3. Register your ESP32 with UW MPSK and add the following code into your magic wand sketch:
    ```cpp
    // WiFi credentials 
    const char* ssid = "UW MPSK";
    const char* password = ; // use your password here
    // Server details 
    const char* serverUrl = ; // Fill in with server URL

    // Student identifier - set this to your UWNetID
    const char* studentId = "REPLACE_WITH_UWNetID";
    ```

4. Add the following stab to your sketch for WiFi configuration:
   ```cpp
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
   ```

5. To enable your magic wand to talk to the server, add the stab below to your magic wand:
    ```cpp
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
    ```

    The ESP32 client (your wand) uses this function to communicate recognized gestures to the central server. This function is a key part of the duel system, as it allows each wand to report its detected spell (gesture) and confidence score for each round. Specifically, 
    - This function constructs a JSON payload with the student ID, gesture, and confidence. 
    - It then sends this data via an HTTP POST request to the server’s `/predict` endpoint.
    - Prints the server’s response (or any error) to the Serial Monitor for debugging.

    This function enables real-time communication between the wand and the server, allowing the server to process duel logic, update health/mana, and display results on the web app.
    
    Without this function, the server would not know which gestures were performed or their confidence, and the duel could not proceed.

6. Upload the sketch to your magic wand.

**Motivating Discussion**: What are the pros and cons for sending raw data to server vs sending prediction and confidence to server?

## Duel Rules

### House Talents
Each house grants a unique talent to its members during duels:

- **Gryffindor – Lion's Heart:** Start with 1 extra HP (6 HP instead of 5).
- **Hufflepuff – Healing Efficiency:** Cast healing spells with 1.5 mana instead of 2.
- **Ravenclaw – Shield Mastery:** Cast shield spells with 1.5 mana instead of 2.
- **Slytherin – Enhanced Fire Bolt:** Fire Bolt deals 1.5x damage.

### Spell Cards
During a duel, you can cast the following spells by performing the corresponding gesture:

- **Fire Bolt (`Z`):** Launches a bolt of fire that deals 1 damage to your opponent. *Mana Cost: 1* (Slytherin: 1.5x damage)
- **Reflect Shield (`O`):** Creates a magical barrier that reflects Fire Bolt with double damage. *Mana Cost: 2* (Ravenclaw: 1.5 mana)
- **Healing Light (`V`):** Channels healing energy to restore 1 HP to yourself. *Mana Cost: 2* (Hufflepuff: 1.5 mana)

House talents are applied automatically based on your chosen house. Use your spells and talents strategically to win the duel!

## Troubleshooting

- If the ESP32 can't connect to WiFi, check your credentials
- If you're not seeing results in the web app, check the Serial Monitor for error messages