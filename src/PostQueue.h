/**
 * @file PostQueue.h
 * @brief ESP32 Arduino library for queuing and sending HTTP POST requests with JSON payloads
 * @version 1.0.0
 * @date 2025-11-12
 * 
 * This library provides a thread-safe queue system for HTTP POST requests using FreeRTOS.
 * It handles SSL/TLS connections, follows redirects, and manages request retries.
 */

#ifndef POST_QUEUE_H
#define POST_QUEUE_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

/**
 * @brief Default maximum queue size to prevent memory issues
 */
#define DEFAULT_MAX_QUEUE_SIZE 10

/**
 * @brief Default timeout for HTTP requests in milliseconds
 */
#define DEFAULT_HTTP_TIMEOUT 10000

/**
 * @brief Default maximum redirects to follow
 */
#define DEFAULT_MAX_REDIRECTS 5

/**
 * @brief Default stack size for the worker task
 */
#define DEFAULT_TASK_STACK_SIZE 8192

/**
 * @brief Default priority for the worker task
 */
#define DEFAULT_TASK_PRIORITY 1

/**
 * @brief Structure to hold a POST request item
 */
struct PostItem {
    char* url;                  ///< Target URL for the POST request
    char* jsonPayload;          ///< JSON payload as string
    char* customHeaders;        ///< Optional custom headers (can be NULL)
    bool useSSL;                ///< Whether to use SSL/TLS
    uint32_t timestamp;         ///< Timestamp when the item was queued
};

/**
 * @brief Callback function type for POST completion
 * @param success Whether the POST request was successful
 * @param httpCode HTTP response code (0 if failed before getting response)
 * @param response Response body from server
 */
typedef void (*PostCallback)(bool success, int httpCode, const String& response);

/**
 * @brief Main PostQueue class for managing HTTP POST requests
 */
class PostQueue {
public:
    /**
     * @brief Construct a new PostQueue object
     * @param maxQueueSize Maximum number of items in the queue (default: 10)
     * @param taskStackSize Stack size for worker task (default: 8192)
     * @param taskPriority Priority for worker task (default: 1)
     */
    PostQueue(size_t maxQueueSize = DEFAULT_MAX_QUEUE_SIZE, 
              size_t taskStackSize = DEFAULT_TASK_STACK_SIZE,
              UBaseType_t taskPriority = DEFAULT_TASK_PRIORITY);

    /**
     * @brief Destroy the PostQueue object and cleanup resources
     */
    ~PostQueue();

    /**
     * @brief Initialize the queue and start the worker task
     * @return true if initialization successful, false otherwise
     */
    bool begin();

    /**
     * @brief Stop the worker task and cleanup
     */
    void end();

    /**
     * @brief Add a POST request to the queue
     * @param url Target URL
     * @param jsonPayload JSON string payload
     * @param useSSL Whether to use SSL/TLS (default: true)
     * @param customHeaders Optional custom headers (default: NULL)
     * @return true if successfully queued, false if queue is full
     */
    bool post(const char* url, const char* jsonPayload, bool useSSL = true, const char* customHeaders = NULL);

    /**
     * @brief Add a POST request to the queue using JsonDocument
     * @param url Target URL
     * @param jsonDoc ArduinoJson document
     * @param useSSL Whether to use SSL/TLS (default: true)
     * @param customHeaders Optional custom headers (default: NULL)
     * @return true if successfully queued, false if queue is full
     */
    bool post(const char* url, JsonDocument& jsonDoc, bool useSSL = true, const char* customHeaders = NULL);

    /**
     * @brief Get the current number of items in the queue
     * @return Number of items waiting to be processed
     */
    size_t getQueueSize();

    /**
     * @brief Check if the queue is empty
     * @return true if queue is empty, false otherwise
     */
    bool isEmpty();

    /**
     * @brief Check if the queue is full
     * @return true if queue is full, false otherwise
     */
    bool isFull();

    /**
     * @brief Clear all items from the queue
     */
    void clear();

    /**
     * @brief Set the HTTP timeout
     * @param timeout Timeout in milliseconds
     */
    void setTimeout(uint32_t timeout);

    /**
     * @brief Set maximum redirects to follow
     * @param maxRedirects Maximum number of redirects (0 to disable)
     */
    void setMaxRedirects(uint8_t maxRedirects);

    /**
     * @brief Set callback for POST completion
     * @param callback Function to call when POST completes
     */
    void setCallback(PostCallback callback);

    /**
     * @brief Set whether to verify SSL certificates
     * @param verify true to verify (default), false to skip verification
     */
    void setSSLVerification(bool verify);

    /**
     * @brief Get statistics about processed requests
     * @param totalProcessed Output: total requests processed
     * @param totalSuccessful Output: total successful requests
     * @param totalFailed Output: total failed requests
     */
    void getStats(uint32_t& totalProcessed, uint32_t& totalSuccessful, uint32_t& totalFailed);

private:
    QueueHandle_t _queue;           ///< FreeRTOS queue handle
    TaskHandle_t _taskHandle;       ///< Worker task handle
    size_t _maxQueueSize;           ///< Maximum queue size
    size_t _taskStackSize;          ///< Stack size for worker task
    UBaseType_t _taskPriority;      ///< Priority for worker task
    uint32_t _httpTimeout;          ///< HTTP request timeout
    uint8_t _maxRedirects;          ///< Maximum redirects to follow
    bool _verifySSL;                ///< Whether to verify SSL certificates
    PostCallback _callback;         ///< Callback for POST completion
    
    // Statistics
    uint32_t _totalProcessed;       ///< Total requests processed
    uint32_t _totalSuccessful;      ///< Total successful requests
    uint32_t _totalFailed;          ///< Total failed requests
    
    bool _running;                  ///< Whether the worker task is running

    /**
     * @brief Worker task function that processes the queue
     * @param parameter Pointer to the PostQueue instance
     */
    static void workerTask(void* parameter);

    /**
     * @brief Process a single POST request
     * @param item PostItem to process
     */
    void processPostItem(PostItem* item);

    /**
     * @brief Free memory allocated for a PostItem
     * @param item PostItem to free
     */
    void freePostItem(PostItem* item);

    /**
     * @brief Perform HTTP POST with redirect following
     * @param url Target URL
     * @param jsonPayload JSON payload
     * @param customHeaders Custom headers
     * @param useSSL Whether to use SSL
     * @param httpCode Output: HTTP response code
     * @param response Output: Response body
     * @return true if successful, false otherwise
     */
    bool performPost(const char* url, const char* jsonPayload, const char* customHeaders, 
                    bool useSSL, int& httpCode, String& response);
};

#endif // POST_QUEUE_H
