/**
 * @file PostQueue.cpp
 * @brief Implementation of the PostQueue library
 */

#include "PostQueue.h"

PostQueue::PostQueue(size_t maxQueueSize, size_t taskStackSize, UBaseType_t taskPriority)
    : _queue(NULL),
      _taskHandle(NULL),
      _maxQueueSize(maxQueueSize),
      _taskStackSize(taskStackSize),
      _taskPriority(taskPriority),
      _httpTimeout(DEFAULT_HTTP_TIMEOUT),
      _maxRedirects(DEFAULT_MAX_REDIRECTS),
      _verifySSL(false),
      _callback(NULL),
      _totalProcessed(0),
      _totalSuccessful(0),
      _totalFailed(0),
      _running(false) {
}

PostQueue::~PostQueue() {
    end();
}

bool PostQueue::begin() {
    if (_running) {
        return true; // Already running
    }

    // Create FreeRTOS queue
    _queue = xQueueCreate(_maxQueueSize, sizeof(PostItem*));
    if (_queue == NULL) {
        Serial.println("PostQueue: Failed to create queue");
        return false;
    }

    // Create worker task
    BaseType_t result = xTaskCreate(
        workerTask,
        "PostQueueWorker",
        _taskStackSize,
        this,
        _taskPriority,
        &_taskHandle
    );

    if (result != pdPASS) {
        Serial.println("PostQueue: Failed to create worker task");
        vQueueDelete(_queue);
        _queue = NULL;
        return false;
    }

    _running = true;
    Serial.println("PostQueue: Initialized successfully");
    return true;
}

void PostQueue::end() {
    if (!_running) {
        return;
    }

    _running = false;

    // Clear the queue
    clear();

    // Delete the task
    if (_taskHandle != NULL) {
        vTaskDelete(_taskHandle);
        _taskHandle = NULL;
    }

    // Delete the queue
    if (_queue != NULL) {
        vQueueDelete(_queue);
        _queue = NULL;
    }

    Serial.println("PostQueue: Stopped");
}

bool PostQueue::post(const char* url, const char* jsonPayload, bool useSSL, const char* customHeaders) {
    if (!_running || _queue == NULL) {
        Serial.println("PostQueue: Not initialized");
        return false;
    }

    // Check if queue is full
    if (uxQueueSpacesAvailable(_queue) == 0) {
        Serial.println("PostQueue: Queue is full");
        return false;
    }

    // Allocate and populate PostItem
    PostItem* item = new PostItem();
    if (item == NULL) {
        Serial.println("PostQueue: Failed to allocate PostItem");
        return false;
    }

    item->url = strdup(url);
    item->jsonPayload = strdup(jsonPayload);
    item->customHeaders = customHeaders ? strdup(customHeaders) : NULL;
    item->useSSL = useSSL;
    item->timestamp = millis();

    // Check if allocations succeeded
    if (item->url == NULL || item->jsonPayload == NULL) {
        Serial.println("PostQueue: Failed to allocate memory for item data");
        freePostItem(item);
        return false;
    }

    // Add to queue
    if (xQueueSend(_queue, &item, 0) != pdTRUE) {
        Serial.println("PostQueue: Failed to add item to queue");
        freePostItem(item);
        return false;
    }

    return true;
}

bool PostQueue::post(const char* url, JsonDocument& jsonDoc, bool useSSL, const char* customHeaders) {
    String jsonString;
    serializeJson(jsonDoc, jsonString);
    return post(url, jsonString.c_str(), useSSL, customHeaders);
}

size_t PostQueue::getQueueSize() {
    if (_queue == NULL) {
        return 0;
    }
    return uxQueueMessagesWaiting(_queue);
}

bool PostQueue::isEmpty() {
    return getQueueSize() == 0;
}

bool PostQueue::isFull() {
    if (_queue == NULL) {
        return false;
    }
    return uxQueueSpacesAvailable(_queue) == 0;
}

void PostQueue::clear() {
    if (_queue == NULL) {
        return;
    }

    PostItem* item;
    while (xQueueReceive(_queue, &item, 0) == pdTRUE) {
        freePostItem(item);
    }
}

void PostQueue::setTimeout(uint32_t timeout) {
    _httpTimeout = timeout;
}

void PostQueue::setMaxRedirects(uint8_t maxRedirects) {
    _maxRedirects = maxRedirects;
}

void PostQueue::setCallback(PostCallback callback) {
    _callback = callback;
}

void PostQueue::setSSLVerification(bool verify) {
    _verifySSL = verify;
}

void PostQueue::getStats(uint32_t& totalProcessed, uint32_t& totalSuccessful, uint32_t& totalFailed) {
    totalProcessed = _totalProcessed;
    totalSuccessful = _totalSuccessful;
    totalFailed = _totalFailed;
}

void PostQueue::workerTask(void* parameter) {
    PostQueue* queue = static_cast<PostQueue*>(parameter);
    PostItem* item;

    Serial.println("PostQueue: Worker task started");

    while (queue->_running) {
        // Wait for item in queue (with timeout to check _running flag periodically)
        if (xQueueReceive(queue->_queue, &item, pdMS_TO_TICKS(100)) == pdTRUE) {
            Serial.println("PostQueue: Processing item");
            queue->processPostItem(item);
            queue->freePostItem(item);
        }
        
        // Small delay to prevent tight loop
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    Serial.println("PostQueue: Worker task stopped");
    vTaskDelete(NULL);
}

void PostQueue::processPostItem(PostItem* item) {
    if (item == NULL) {
        return;
    }

    _totalProcessed++;

    int httpCode = 0;
    String response = "";
    
    Serial.print("PostQueue: Sending POST to ");
    Serial.println(item->url);

    bool success = performPost(item->url, item->jsonPayload, item->customHeaders, 
                               item->useSSL, httpCode, response);

    if (success) {
        _totalSuccessful++;
        Serial.print("PostQueue: POST successful, HTTP code: ");
        Serial.println(httpCode);
    } else {
        _totalFailed++;
        Serial.print("PostQueue: POST failed, HTTP code: ");
        Serial.println(httpCode);
    }

    // Call callback if set
    if (_callback != NULL) {
        _callback(success, httpCode, response);
    }
}

bool PostQueue::performPost(const char* url, const char* jsonPayload, const char* customHeaders,
                           bool useSSL, int& httpCode, String& response) {
    HTTPClient http;
    WiFiClient client;
    WiFiClientSecure secureClient;

    // Configure SSL client if using SSL
    if (useSSL) {
        if (!_verifySSL) {
            secureClient.setInsecure(); // Skip SSL verification
        }
        http.begin(secureClient, url);
    } else {
        http.begin(client, url);
    }

    // Set timeout
    http.setTimeout(_httpTimeout);

    // Follow redirects
    http.setFollowRedirects(_maxRedirects > 0 ? HTTPC_FORCE_FOLLOW_REDIRECTS : HTTPC_DISABLE_FOLLOW_REDIRECTS);
    http.setRedirectLimit(_maxRedirects);

    // Set headers
    http.addHeader("Content-Type", "application/json");
    
    // Add custom headers if provided
    if (customHeaders != NULL && strlen(customHeaders) > 0) {
        // Parse custom headers (format: "Header1: Value1\nHeader2: Value2")
        String headers = String(customHeaders);
        int startPos = 0;
        int endPos = headers.indexOf('\n');
        
        while (endPos >= 0 || startPos < headers.length()) {
            String headerLine = (endPos >= 0) ? headers.substring(startPos, endPos) : headers.substring(startPos);
            headerLine.trim();
            
            if (headerLine.length() > 0) {
                int colonPos = headerLine.indexOf(':');
                if (colonPos > 0) {
                    String headerName = headerLine.substring(0, colonPos);
                    String headerValue = headerLine.substring(colonPos + 1);
                    headerName.trim();
                    headerValue.trim();
                    http.addHeader(headerName, headerValue);
                }
            }
            
            if (endPos < 0) break;
            startPos = endPos + 1;
            endPos = headers.indexOf('\n', startPos);
        }
    }

    // Perform POST request
    httpCode = http.POST(jsonPayload);

    // Check response
    bool success = false;
    if (httpCode > 0) {
        if (httpCode >= 200 && httpCode < 300) {
            success = true;
            response = http.getString();
        } else {
            response = http.getString();
        }
    } else {
        Serial.print("PostQueue: HTTP error: ");
        Serial.println(http.errorToString(httpCode).c_str());
    }

    http.end();
    return success;
}

void PostQueue::freePostItem(PostItem* item) {
    if (item == NULL) {
        return;
    }

    if (item->url != NULL) {
        free(item->url);
    }
    if (item->jsonPayload != NULL) {
        free(item->jsonPayload);
    }
    if (item->customHeaders != NULL) {
        free(item->customHeaders);
    }
    delete item;
}
