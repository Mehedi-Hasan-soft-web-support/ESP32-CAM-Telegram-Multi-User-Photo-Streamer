An ESP32-CAM project that captures photos at configurable intervals and sends them as JPEG images to multiple Telegram users via a bot. Utilizes PSRAM for optimized image quality and supports live flashing LED during capture. Ideal for DIY remote monitoring and surveillance with real-time multi-user notifications through Telegram.

Features:

Sends captured photos to multiple Telegram chat IDs

Adjustable capture interval (default 5 seconds)

High-quality JPEG image capture optimized for PSRAM-enabled boards

LED flash activation during photo capture for better lighting

Simple multipart/form-data POST requests to Telegram Bot API

WiFi connectivity with automatic reconnection

