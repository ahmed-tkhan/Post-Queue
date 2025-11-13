/**
 * @file BasicPostQueue.ino
 * @brief Basic example of using the PostQueue library
 * 
 * This example demonstrates:
 * - Connecting to WiFi
 * - Initializing PostQueue
 * - Adding POST requests to the queue
 * - Using both JSON strings and JsonDocument
 * - Monitoring queue statistics
 */

#include <WiFi.h>
#include <PostQueue.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// API endpoint - replace with your actual endpoint
const char* apiEndpoint = "https://httpbin.org/post";

// Create PostQueue instance with max queue size of 5
PostQueue postQueue(5);

// Callback function for POST completion
void onPostComplete(bool success, int httpCode, const String& response) {
  Serial.println("=== POST Request Completed ===");
  Serial.print("Success: ");
  Serial.println(success ? "Yes" : "No");
  Serial.print("HTTP Code: ");
  Serial.println(httpCode);
  Serial.print("Response: ");
  Serial.println(response);
  Serial.println("=============================");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n=== PostQueue Basic Example ===");

  // Connect to WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Initialize PostQueue
  Serial.println("\nInitializing PostQueue...");
  if (!postQueue.begin()) {
    Serial.println("Failed to initialize PostQueue!");
    return;
  }

  // Configure PostQueue
  postQueue.setTimeout(15000);           // 15 second timeout
  postQueue.setMaxRedirects(5);          // Follow up to 5 redirects
  postQueue.setSSLVerification(false);   // Skip SSL verification (for testing)
  postQueue.setCallback(onPostComplete); // Set completion callback

  Serial.println("PostQueue initialized successfully!");

  // Example 1: Post using JSON string
  Serial.println("\n--- Example 1: JSON String ---");
  const char* jsonString = "{\"sensor\":\"temperature\",\"value\":25.5,\"unit\":\"celsius\"}";
  if (postQueue.post(apiEndpoint, jsonString, true)) {
    Serial.println("✓ JSON string queued successfully");
  } else {
    Serial.println("✗ Failed to queue JSON string");
  }

  // Example 2: Post using JsonDocument
  Serial.println("\n--- Example 2: JsonDocument ---");
  StaticJsonDocument<200> doc;
  doc["sensor"] = "humidity";
  doc["value"] = 65.2;
  doc["unit"] = "percent";
  doc["location"] = "office";
  
  if (postQueue.post(apiEndpoint, doc, true)) {
    Serial.println("✓ JsonDocument queued successfully");
  } else {
    Serial.println("✗ Failed to queue JsonDocument");
  }

  // Example 3: Post with custom headers
  Serial.println("\n--- Example 3: Custom Headers ---");
  StaticJsonDocument<200> doc2;
  doc2["event"] = "motion_detected";
  doc2["timestamp"] = millis();
  doc2["zone"] = "entrance";
  
  const char* customHeaders = "X-API-Key: your-api-key-here\nX-Device-ID: ESP32-001";
  
  if (postQueue.post(apiEndpoint, doc2, true, customHeaders)) {
    Serial.println("✓ Request with custom headers queued successfully");
  } else {
    Serial.println("✗ Failed to queue request with custom headers");
  }

  // Display queue status
  Serial.println("\n--- Queue Status ---");
  Serial.print("Items in queue: ");
  Serial.println(postQueue.getQueueSize());
  Serial.print("Queue full: ");
  Serial.println(postQueue.isFull() ? "Yes" : "No");
}

void loop() {
  // Monitor queue status every 5 seconds
  static unsigned long lastCheck = 0;
  unsigned long now = millis();
  
  if (now - lastCheck >= 5000) {
    lastCheck = now;
    
    Serial.println("\n--- Queue Statistics ---");
    Serial.print("Items in queue: ");
    Serial.println(postQueue.getQueueSize());
    
    uint32_t processed, successful, failed;
    postQueue.getStats(processed, successful, failed);
    
    Serial.print("Total processed: ");
    Serial.println(processed);
    Serial.print("Successful: ");
    Serial.println(successful);
    Serial.print("Failed: ");
    Serial.println(failed);
    Serial.println("----------------------");
    
    // Add another item every 30 seconds for demonstration
    static unsigned long lastPost = 0;
    if (now - lastPost >= 30000) {
      lastPost = now;
      
      Serial.println("\n--- Adding periodic POST ---");
      StaticJsonDocument<200> doc;
      doc["type"] = "periodic_update";
      doc["uptime"] = millis() / 1000;
      doc["free_heap"] = ESP.getFreeHeap();
      
      if (postQueue.post(apiEndpoint, doc, true)) {
        Serial.println("✓ Periodic update queued");
      } else {
        Serial.println("✗ Queue is full, skipping periodic update");
      }
    }
  }
  
  delay(100);
}
