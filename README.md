# PostQueue

[![Arduino](https://img.shields.io/badge/Arduino-Compatible-blue.svg)](https://www.arduino.cc/)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange.svg)](https://platformio.org/)
[![ESP32](https://img.shields.io/badge/ESP32-Compatible-green.svg)](https://www.espressif.com/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A powerful and robust ESP32 Arduino library for queuing and sending HTTP POST requests with JSON payloads using FreeRTOS. This library provides a thread-safe queue system that handles SSL/TLS connections, follows HTTP redirects, and manages requests efficiently to prevent memory issues.

## Features

- ✅ **Thread-Safe Queue**: Uses FreeRTOS queue for reliable request management
- ✅ **SSL/TLS Support**: Secure HTTPS connections with configurable certificate verification
- ✅ **Automatic Redirects**: Follows HTTP redirects up to a configurable limit
- ✅ **JSON Support**: Native support for ArduinoJson library
- ✅ **Custom Headers**: Add custom HTTP headers to requests
- ✅ **Configurable Queue Size**: Prevent memory issues with size-limited queue
- ✅ **Request Statistics**: Track successful and failed requests
- ✅ **Callbacks**: Optional callbacks for request completion
- ✅ **Non-Blocking**: Background task processes queue without blocking main code
- ✅ **Memory Safe**: Automatic memory management and cleanup

## Installation

### Arduino IDE

1. Download the latest release from GitHub
2. In Arduino IDE, go to **Sketch → Include Library → Add .ZIP Library**
3. Select the downloaded ZIP file
4. Restart Arduino IDE

Alternatively, use the Library Manager:
1. Open **Tools → Manage Libraries**
2. Search for "PostQueue"
3. Click Install

### PlatformIO

Add to your `platformio.ini`:

```ini
lib_deps =
    PostQueue
    bblanchon/ArduinoJson@^6.21.0
```

Or install via CLI:
```bash
pio lib install "PostQueue"
```

## Dependencies

- **ArduinoJson** (v6.21.0 or higher) - For JSON handling
- **WiFiClientSecure** - Built-in ESP32 library for SSL/TLS
- **HTTPClient** - Built-in ESP32 library for HTTP requests
- **FreeRTOS** - Built-in ESP32 library for task management

## Quick Start

```cpp
#include <WiFi.h>
#include <PostQueue.h>
#include <ArduinoJson.h>

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
const char* apiUrl = "https://api.example.com/data";

PostQueue postQueue(10);  // Queue size of 10

void setup() {
  Serial.begin(115200);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  
  // Initialize PostQueue
  postQueue.begin();
  postQueue.setTimeout(15000);
  postQueue.setSSLVerification(false);  // For testing
  
  // Create and queue a POST request
  StaticJsonDocument<200> doc;
  doc["sensor"] = "temperature";
  doc["value"] = 25.5;
  
  postQueue.post(apiUrl, doc, true);
}

void loop() {
  // Your code here - queue processes in background
  delay(1000);
}
```

## API Reference

### Constructor

```cpp
PostQueue(size_t maxQueueSize = 10, 
          size_t taskStackSize = 8192,
          UBaseType_t taskPriority = 1)
```

Creates a new PostQueue instance.

**Parameters:**
- `maxQueueSize` - Maximum number of items in queue (default: 10)
- `taskStackSize` - Stack size for worker task (default: 8192)
- `taskPriority` - FreeRTOS task priority (default: 1)

### Methods

#### `bool begin()`
Initialize the queue and start the worker task.

**Returns:** `true` if successful, `false` otherwise

#### `void end()`
Stop the worker task and cleanup all resources.

#### `bool post(const char* url, const char* jsonPayload, bool useSSL = true, const char* customHeaders = NULL)`
Add a POST request to the queue using a JSON string.

**Parameters:**
- `url` - Target URL
- `jsonPayload` - JSON string payload
- `useSSL` - Use HTTPS (default: true)
- `customHeaders` - Optional custom headers (format: "Header1: Value1\nHeader2: Value2")

**Returns:** `true` if queued successfully, `false` if queue is full

#### `bool post(const char* url, JsonDocument& jsonDoc, bool useSSL = true, const char* customHeaders = NULL)`
Add a POST request to the queue using an ArduinoJson document.

**Parameters:**
- `url` - Target URL
- `jsonDoc` - ArduinoJson document
- `useSSL` - Use HTTPS (default: true)
- `customHeaders` - Optional custom headers

**Returns:** `true` if queued successfully, `false` if queue is full

#### `size_t getQueueSize()`
Get the current number of items in the queue.

**Returns:** Number of pending items

#### `bool isEmpty()`
Check if the queue is empty.

**Returns:** `true` if empty, `false` otherwise

#### `bool isFull()`
Check if the queue is full.

**Returns:** `true` if full, `false` otherwise

#### `void clear()`
Remove all items from the queue.

#### `void setTimeout(uint32_t timeout)`
Set HTTP request timeout in milliseconds (default: 10000).

#### `void setMaxRedirects(uint8_t maxRedirects)`
Set maximum number of redirects to follow (default: 5, 0 to disable).

#### `void setCallback(PostCallback callback)`
Set callback function for request completion.

**Callback signature:**
```cpp
void callback(bool success, int httpCode, const String& response)
```

#### `void setSSLVerification(bool verify)`
Enable or disable SSL certificate verification (default: false for development).

#### `void getStats(uint32_t& totalProcessed, uint32_t& totalSuccessful, uint32_t& totalFailed)`
Get statistics about processed requests.

## Examples

### Basic Usage

```cpp
#include <PostQueue.h>

PostQueue queue(5);

void setup() {
  queue.begin();
  
  // Simple POST with JSON string
  queue.post("https://api.example.com/data", 
             "{\"temp\":25.5}", 
             true);
}
```

### Using ArduinoJson

```cpp
StaticJsonDocument<200> doc;
doc["sensor"] = "humidity";
doc["value"] = 65.2;
doc["unit"] = "percent";

postQueue.post("https://api.example.com/data", doc, true);
```

### Custom Headers

```cpp
const char* headers = "X-API-Key: your-key\nAuthorization: Bearer token123";

postQueue.post("https://api.example.com/data", 
               "{\"data\":\"value\"}", 
               true, 
               headers);
```

### With Callback

```cpp
void onComplete(bool success, int httpCode, const String& response) {
  if (success) {
    Serial.println("POST successful!");
    Serial.println(response);
  } else {
    Serial.printf("POST failed with code: %d\n", httpCode);
  }
}

void setup() {
  postQueue.begin();
  postQueue.setCallback(onComplete);
  
  postQueue.post("https://api.example.com/data", "{\"test\":true}", true);
}
```

### Queue Management

```cpp
// Check queue status
if (postQueue.isFull()) {
  Serial.println("Queue is full, waiting...");
  delay(1000);
}

// Add request if space available
if (!postQueue.isFull()) {
  postQueue.post(url, jsonData, true);
}

// Get statistics
uint32_t processed, successful, failed;
postQueue.getStats(processed, successful, failed);
Serial.printf("Success rate: %.1f%%\n", 
              (float)successful / processed * 100.0);
```

## Best Practices

1. **Queue Size**: Choose a queue size based on your available memory and posting frequency
2. **SSL Verification**: Enable SSL verification in production for security
3. **Error Handling**: Always check the return value of `post()` to handle full queue conditions
4. **Memory Management**: The library handles memory cleanup automatically
5. **WiFi Connection**: Ensure WiFi is connected before initializing PostQueue
6. **Timeout**: Adjust timeout based on your network conditions and API response time

## Troubleshooting

### Queue is always full
- Increase queue size in constructor
- Check if API endpoint is responding
- Verify network connectivity

### SSL/TLS errors
- Try `setSSLVerification(false)` for testing
- Ensure correct URL (https:// vs http://)
- Check if ESP32 has enough memory

### Requests failing
- Check WiFi connection
- Verify API endpoint URL
- Increase timeout with `setTimeout()`
- Check API server logs

### Memory issues
- Reduce queue size
- Reduce task stack size
- Clear queue periodically with `clear()`

## Platform Support

- **ESP32** - Fully supported
- **ESP8266** - Not supported (lacks FreeRTOS features)
- **Other platforms** - Not supported

## License

MIT License - See LICENSE file for details

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues.

## Author

Ahmed T Khan

## Version History

- **1.0.0** (2025-11-12)
  - Initial release
  - FreeRTOS queue implementation
  - SSL/TLS support
  - Redirect following
  - Custom headers support
  - Statistics tracking
  - Callback support
