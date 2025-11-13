/**
 * @file IoTSensorData.ino
 * @brief Real-world IoT example - collecting and posting sensor data
 * 
 * This example simulates an IoT device that:
 * - Reads multiple sensor values periodically
 * - Queues sensor data as JSON
 * - Handles network interruptions gracefully
 * - Monitors queue health
 */

#include <WiFi.h>
#include <PostQueue.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// API endpoint for sensor data
const char* sensorApiUrl = "https://your-api.example.com/api/sensors/data";

// Device configuration
const char* deviceId = "ESP32-SENSOR-001";
const char* location = "Office";

// Create PostQueue with sufficient size for periodic data
PostQueue sensorQueue(15);

// Sensor reading interval (10 seconds)
const unsigned long SENSOR_INTERVAL = 10000;
unsigned long lastSensorRead = 0;

// Statistics interval (30 seconds)
const unsigned long STATS_INTERVAL = 30000;
unsigned long lastStatsDisplay = 0;

// Simulated sensor values
float temperature = 22.0;
float humidity = 50.0;
float pressure = 1013.25;

void onDataSent(bool success, int httpCode, const String& response) {
  if (success) {
    Serial.println("✓ Sensor data sent successfully");
  } else {
    Serial.printf("✗ Failed to send sensor data (HTTP %d)\n", httpCode);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n=== IoT Sensor Data Queue Example ===");

  // Initialize WiFi
  connectWiFi();

  // Initialize PostQueue
  Serial.println("Initializing PostQueue...");
  if (!sensorQueue.begin()) {
    Serial.println("Failed to initialize PostQueue!");
    return;
  }

  // Configure PostQueue for IoT usage
  sensorQueue.setTimeout(20000);           // 20 second timeout for slow networks
  sensorQueue.setMaxRedirects(3);          // Allow some redirects
  sensorQueue.setSSLVerification(false);   // Skip SSL verification for testing
  sensorQueue.setCallback(onDataSent);     // Set callback

  Serial.println("PostQueue ready for sensor data!");
  Serial.println("Starting sensor readings...\n");
}

void loop() {
  unsigned long currentTime = millis();

  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠ WiFi disconnected! Reconnecting...");
    connectWiFi();
  }

  // Read and queue sensor data periodically
  if (currentTime - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = currentTime;
    readAndQueueSensorData();
  }

  // Display statistics periodically
  if (currentTime - lastStatsDisplay >= STATS_INTERVAL) {
    lastStatsDisplay = currentTime;
    displayQueueStatistics();
  }

  delay(100);
}

void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("\n✗ Failed to connect to WiFi");
  }
}

void readAndQueueSensorData() {
  // Simulate reading sensors (replace with actual sensor code)
  temperature += random(-10, 10) / 10.0;  // Vary by ±1°C
  humidity += random(-20, 20) / 10.0;      // Vary by ±2%
  pressure += random(-5, 5) / 10.0;        // Vary by ±0.5 hPa

  // Keep values in realistic ranges
  temperature = constrain(temperature, 15.0, 35.0);
  humidity = constrain(humidity, 30.0, 80.0);
  pressure = constrain(pressure, 980.0, 1040.0);

  Serial.println("\n--- Reading Sensors ---");
  Serial.printf("Temperature: %.1f°C\n", temperature);
  Serial.printf("Humidity: %.1f%%\n", humidity);
  Serial.printf("Pressure: %.2f hPa\n", pressure);

  // Create JSON payload
  StaticJsonDocument<400> doc;
  
  // Device information
  doc["device_id"] = deviceId;
  doc["location"] = location;
  doc["timestamp"] = currentTime();
  
  // Sensor readings
  JsonObject sensors = doc.createNestedObject("sensors");
  sensors["temperature"]["value"] = temperature;
  sensors["temperature"]["unit"] = "celsius";
  sensors["humidity"]["value"] = humidity;
  sensors["humidity"]["unit"] = "percent";
  sensors["pressure"]["value"] = pressure;
  sensors["pressure"]["unit"] = "hPa";
  
  // Device health
  JsonObject health = doc.createNestedObject("health");
  health["uptime"] = millis() / 1000;
  health["free_heap"] = ESP.getFreeHeap();
  health["wifi_rssi"] = WiFi.RSSI();
  health["wifi_quality"] = getWiFiQuality();

  // Optional: Add custom headers with API key
  const char* headers = "X-API-Key: your-api-key-here\nX-Device-Type: ESP32";

  // Queue the sensor data
  if (sensorQueue.post(sensorApiUrl, doc, true, headers)) {
    Serial.println("✓ Sensor data queued");
    Serial.printf("Queue size: %d items\n", sensorQueue.getQueueSize());
  } else {
    Serial.println("✗ Failed to queue sensor data (queue full?)");
    
    // If queue is full, try to clear old data or wait
    if (sensorQueue.isFull()) {
      Serial.println("⚠ Queue is full! Consider increasing queue size or check network.");
    }
  }
}

void displayQueueStatistics() {
  Serial.println("\n=== Queue Statistics ===");
  
  uint32_t processed, successful, failed;
  sensorQueue.getStats(processed, successful, failed);
  
  Serial.printf("Queue size: %d items\n", sensorQueue.getQueueSize());
  Serial.printf("Total processed: %d\n", processed);
  Serial.printf("Successful: %d\n", successful);
  Serial.printf("Failed: %d\n", failed);
  
  if (processed > 0) {
    float successRate = (float)successful / processed * 100.0;
    Serial.printf("Success rate: %.1f%%\n", successRate);
    
    if (successRate < 80.0) {
      Serial.println("⚠ Low success rate! Check network and API endpoint.");
    }
  }
  
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("WiFi RSSI: %d dBm\n", WiFi.RSSI());
  Serial.println("========================\n");
}

String currentTime() {
  // In a real application, use NTP to get actual time
  // For this example, just return uptime
  unsigned long uptime = millis() / 1000;
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "uptime_%lu", uptime);
  return String(buffer);
}

int getWiFiQuality() {
  int rssi = WiFi.RSSI();
  int quality = 0;
  
  if (rssi <= -100) {
    quality = 0;
  } else if (rssi >= -50) {
    quality = 100;
  } else {
    quality = 2 * (rssi + 100);
  }
  
  return quality;
}
