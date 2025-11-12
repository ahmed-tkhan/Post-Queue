/**
 * @file AdvancedPostQueue.ino
 * @brief Advanced example showing queue management and error handling
 * 
 * This example demonstrates:
 * - Queue size management
 * - Handling queue full conditions
 * - Different SSL configurations
 * - Non-SSL POST requests
 * - Error handling and recovery
 */

#include <WiFi.h>
#include <PostQueue.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// API endpoints
const char* httpsEndpoint = "https://httpbin.org/post";
const char* httpEndpoint = "http://httpbin.org/post";

// Create PostQueue with larger queue
PostQueue postQueue(20);

// Track callback results
int successCount = 0;
int failureCount = 0;

void onPostComplete(bool success, int httpCode, const String& response) {
  if (success) {
    successCount++;
    Serial.println("✓ POST succeeded");
  } else {
    failureCount++;
    Serial.println("✗ POST failed");
  }
  
  Serial.printf("Stats - Success: %d, Failed: %d\n", successCount, failureCount);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n=== PostQueue Advanced Example ===");

  // Connect to WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nFailed to connect to WiFi!");
    Serial.println("Continuing anyway for demonstration...");
  } else {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }

  // Initialize PostQueue with custom settings
  Serial.println("\nInitializing PostQueue...");
  if (!postQueue.begin()) {
    Serial.println("Failed to initialize PostQueue!");
    return;
  }

  // Configure PostQueue
  postQueue.setTimeout(10000);           // 10 second timeout
  postQueue.setMaxRedirects(3);          // Follow up to 3 redirects
  postQueue.setSSLVerification(false);   // Skip SSL verification
  postQueue.setCallback(onPostComplete); // Set completion callback

  Serial.println("PostQueue initialized successfully!");

  // Demonstrate queue capacity handling
  demonstrateQueueCapacity();
  
  // Demonstrate SSL and non-SSL requests
  demonstrateSSLOptions();
  
  // Display initial statistics
  displayStatistics();
}

void loop() {
  // Monitor and display statistics every 10 seconds
  static unsigned long lastCheck = 0;
  unsigned long now = millis();
  
  if (now - lastCheck >= 10000) {
    lastCheck = now;
    displayStatistics();
    
    // Add batches of requests periodically
    static int batchCount = 0;
    if (batchCount < 3) {  // Only 3 batches for demo
      batchCount++;
      Serial.printf("\n=== Adding Batch #%d ===\n", batchCount);
      addBatchRequests(3);
    }
  }
  
  delay(100);
}

void demonstrateQueueCapacity() {
  Serial.println("\n=== Demonstrating Queue Capacity ===");
  
  // Fill the queue
  Serial.println("Filling queue with requests...");
  int queued = 0;
  
  for (int i = 0; i < 25; i++) {  // Try to add more than queue size
    StaticJsonDocument<200> doc;
    doc["batch"] = "capacity_test";
    doc["item"] = i;
    doc["timestamp"] = millis();
    
    if (postQueue.post(httpsEndpoint, doc, true)) {
      queued++;
    } else {
      Serial.printf("Queue full after %d items\n", queued);
      break;
    }
  }
  
  Serial.printf("Successfully queued %d items\n", queued);
  Serial.printf("Queue size: %d\n", postQueue.getQueueSize());
  Serial.printf("Queue full: %s\n", postQueue.isFull() ? "Yes" : "No");
}

void demonstrateSSLOptions() {
  Serial.println("\n=== Demonstrating SSL Options ===");
  
  // HTTPS request (SSL enabled)
  StaticJsonDocument<200> doc1;
  doc1["type"] = "https_request";
  doc1["ssl"] = true;
  
  if (postQueue.post(httpsEndpoint, doc1, true)) {
    Serial.println("✓ HTTPS request queued");
  }
  
  // HTTP request (SSL disabled)
  StaticJsonDocument<200> doc2;
  doc2["type"] = "http_request";
  doc2["ssl"] = false;
  
  if (postQueue.post(httpEndpoint, doc2, false)) {
    Serial.println("✓ HTTP request queued");
  }
  
  // HTTPS with custom headers
  StaticJsonDocument<200> doc3;
  doc3["type"] = "https_with_headers";
  doc3["ssl"] = true;
  
  const char* headers = "X-Custom-Header: CustomValue\nAuthorization: Bearer token123";
  
  if (postQueue.post(httpsEndpoint, doc3, true, headers)) {
    Serial.println("✓ HTTPS request with custom headers queued");
  }
}

void addBatchRequests(int count) {
  int queued = 0;
  
  for (int i = 0; i < count; i++) {
    if (postQueue.isFull()) {
      Serial.println("⚠ Queue is full, waiting...");
      delay(2000);  // Wait for queue to process
    }
    
    StaticJsonDocument<200> doc;
    doc["batch"] = "periodic";
    doc["item"] = i;
    doc["uptime"] = millis() / 1000;
    doc["free_heap"] = ESP.getFreeHeap();
    doc["wifi_rssi"] = WiFi.RSSI();
    
    if (postQueue.post(httpsEndpoint, doc, true)) {
      queued++;
      Serial.printf("  ✓ Item %d queued\n", i);
    } else {
      Serial.printf("  ✗ Failed to queue item %d\n", i);
    }
    
    delay(100);  // Small delay between requests
  }
  
  Serial.printf("Queued %d/%d items in batch\n", queued, count);
}

void displayStatistics() {
  Serial.println("\n=== Queue Statistics ===");
  
  uint32_t processed, successful, failed;
  postQueue.getStats(processed, successful, failed);
  
  Serial.printf("Queue size: %d\n", postQueue.getQueueSize());
  Serial.printf("Queue empty: %s\n", postQueue.isEmpty() ? "Yes" : "No");
  Serial.printf("Queue full: %s\n", postQueue.isFull() ? "Yes" : "No");
  Serial.printf("Total processed: %d\n", processed);
  Serial.printf("Successful: %d\n", successful);
  Serial.printf("Failed: %d\n", failed);
  
  if (processed > 0) {
    float successRate = (float)successful / processed * 100.0;
    Serial.printf("Success rate: %.1f%%\n", successRate);
  }
  
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
  Serial.println("=======================");
}
