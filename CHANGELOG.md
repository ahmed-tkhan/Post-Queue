# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2025-11-12

### Added
- Initial release of PostQueue library
- Thread-safe FreeRTOS queue implementation for HTTP POST requests
- SSL/TLS support with configurable certificate verification
- Automatic HTTP redirect following (up to 5 redirects by default)
- JSON payload support for both String and ArduinoJson documents
- Custom HTTP headers support
- Configurable queue size to prevent memory exhaustion
- Request statistics tracking (total, successful, failed)
- Optional callback functions for POST completion notification
- Non-blocking background task processing
- Automatic memory management and cleanup
- Comprehensive API with getter methods for queue state
- Support for both HTTPS and HTTP endpoints

### Examples
- BasicPostQueue - Introduction to library usage with simple examples
- AdvancedPostQueue - Demonstrates queue management and error handling
- IoTSensorData - Real-world IoT device simulation with periodic sensor readings

### Documentation
- Comprehensive README.md with API reference
- Detailed installation instructions for Arduino IDE and PlatformIO
- Usage examples and best practices
- Troubleshooting guide
- Publishing guide (PUBLISHING.md)

### Metadata
- Arduino Library Manager compatibility (library.properties)
- PlatformIO Library Registry compatibility (library.json)
- Arduino IDE syntax highlighting (keywords.txt)
- MIT License

### Dependencies
- ArduinoJson (^6.21.0)
- WiFiClientSecure (built-in ESP32)
- HTTPClient (built-in ESP32)
- FreeRTOS (built-in ESP32)

### Platform Support
- ESP32 (fully supported)
- Requires Arduino framework or ESP-IDF

---

## Future Roadmap

### [1.1.0] - Planned
- [ ] Add retry mechanism with exponential backoff
- [ ] Support for custom CA certificates
- [ ] Add request priority levels
- [ ] Implement request cancellation
- [ ] Add progress callback for large payloads

### [1.2.0] - Planned
- [ ] Support for other HTTP methods (PUT, PATCH, DELETE)
- [ ] Add batch POST support
- [ ] Persistent storage for queue (SPIFFS/LittleFS)
- [ ] Network reconnection handling
- [ ] OTA update integration

### [2.0.0] - Future
- [ ] ESP8266 support (limited FreeRTOS features)
- [ ] Multi-endpoint support
- [ ] Request scheduling and rate limiting
- [ ] Built-in authentication handlers (OAuth, API keys)
- [ ] Response caching

---

## Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Make your changes with tests
4. Submit a pull request

## Versioning

This project uses [Semantic Versioning](https://semver.org/):
- **MAJOR** version for incompatible API changes
- **MINOR** version for backwards-compatible functionality additions
- **PATCH** version for backwards-compatible bug fixes
