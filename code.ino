#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

//
// WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
//            Ensure ESP32 Wrover Module or other board with PSRAM is selected
//

// ===================
// Select camera model - Uncomment the one you're using
// ===================
#define CAMERA_MODEL_AI_THINKER // Has PSRAM

// ===================
// Camera Pin Definitions (inline)
// ===================
#if defined(CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define LED_GPIO_NUM       4  // Flash LED
#endif

// ===========================
// Enter your WiFi credentials
// ===========================
const char* ssid = "Me";
const char* password = "mehedi113";

// ===========================
// Telegram Bot Configuration
// ===========================
const String BOT_TOKEN = "8447024742:AAFDLc_v3_sRZXjaYnONttL_8i66YwSZdbo";

// Multiple Chat IDs - Add more users here
const String CHAT_IDS[] = {
  "6032672417",  // User 1
  "5093700812"   // User 2
};
const int TOTAL_USERS = 2; // Update this number when adding more users

const String TELEGRAM_URL = "https://api.telegram.org/bot8447024742:AAFDLc_v3_sRZXjaYnONttL_8i66YwSZdbo/sendPhoto";

// ===========================
// Timing Configuration
// ===========================
const unsigned long CAPTURE_INTERVAL = 5000; // 5 seconds between captures
unsigned long lastCaptureTime = 0;

// ===========================
// Function Declarations
// ===========================
bool sendPhotoToTelegram(camera_fb_t* fb, String chatId);
void captureAndSendPhoto();
void setupLedFlash(int pin);

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  Serial.println("ESP32-CAM Telegram Multi-User Live Stream Starting...");

  // Camera configuration
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  
  // Optimized settings for Telegram streaming
  config.frame_size = FRAMESIZE_SVGA;  // 800x600 - good balance of quality/size
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 15;  // Lower number = higher quality, but larger file
  config.fb_count = 1;
  
  // Optimize for PSRAM if available
  if(psramFound()){
    config.jpeg_quality = 12;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
    Serial.println("PSRAM found - using optimized settings");
  } else {
    config.frame_size = FRAMESIZE_VGA;  // Smaller if no PSRAM
    config.fb_location = CAMERA_FB_IN_DRAM;
    Serial.println("No PSRAM - using conservative settings");
  }

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }
  Serial.println("Camera initialized successfully");

  // Get camera sensor and apply settings
  sensor_t * s = esp_camera_sensor_get();
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);
    s->set_brightness(s, 1);
    s->set_saturation(s, -2);
  }

  // Setup LED Flash if available
#ifdef LED_GPIO_NUM
  setupLedFlash(LED_GPIO_NUM);
#endif

  // Connect to WiFi
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  Serial.println("Starting Telegram multi-user live stream...");
  Serial.printf("Sending photos to %d users every %lu seconds\n", TOTAL_USERS, CAPTURE_INTERVAL/1000);
  
  // Print all chat IDs
  for(int i = 0; i < TOTAL_USERS; i++) {
    Serial.printf("User %d: %s\n", i+1, CHAT_IDS[i].c_str());
  }
}

void loop() {
  unsigned long currentTime = millis();
  
  // Check if it's time to capture and send a photo
  if (currentTime - lastCaptureTime >= CAPTURE_INTERVAL) {
    captureAndSendPhoto();
    lastCaptureTime = currentTime;
  }
  
  // Small delay to prevent watchdog issues
  delay(100);
}

void captureAndSendPhoto() {
  Serial.println("Capturing photo...");
  
  // Flash LED on during capture (if available)
#ifdef LED_GPIO_NUM
  digitalWrite(LED_GPIO_NUM, HIGH);
  delay(100);
#endif
  
  // Capture photo
  camera_fb_t * fb = esp_camera_fb_get();
  
  // Turn off flash
#ifdef LED_GPIO_NUM
  digitalWrite(LED_GPIO_NUM, LOW);
#endif
  
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  
  Serial.printf("Photo captured: %dx%d, size: %d bytes\n", 
                fb->width, fb->height, fb->len);
  
  // Send to all users
  int successCount = 0;
  for(int i = 0; i < TOTAL_USERS; i++) {
    Serial.printf("Sending to user %d (ID: %s)...\n", i+1, CHAT_IDS[i].c_str());
    
    bool success = sendPhotoToTelegram(fb, CHAT_IDS[i]);
    
    if (success) {
      Serial.printf("✓ Sent to user %d successfully!\n", i+1);
      successCount++;
    } else {
      Serial.printf("✗ Failed to send to user %d\n", i+1);
    }
    
    // Small delay between sends to avoid rate limiting
    if(i < TOTAL_USERS - 1) {
      delay(1000); // 1 second delay between users
    }
  }
  
  Serial.printf("Photo sent to %d/%d users successfully!\n", successCount, TOTAL_USERS);
  
  // Return the frame buffer back to the driver for reuse
  esp_camera_fb_return(fb);
}

bool sendPhotoToTelegram(camera_fb_t* fb, String chatId) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
    return false;
  }
  
  WiFiClientSecure client;
  client.setInsecure();
  
  if (!client.connect("api.telegram.org", 443)) {
    Serial.println("Connection to Telegram failed");
    return false;
  }
  
  String boundary = "----ESP32CAMBoundary1234567890";
  String contentType = "multipart/form-data; boundary=" + boundary;
  
  // Build the POST body
  String bodyStart = "";
  bodyStart += "--" + boundary + "\r\n";
  bodyStart += "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n";
  bodyStart += chatId + "\r\n";
  bodyStart += "--" + boundary + "\r\n";
  bodyStart += "Content-Disposition: form-data; name=\"caption\"\r\n\r\n";
  bodyStart += "Live feed from ESP32-CAM - " + String(millis()/1000) + "s\r\n";
  bodyStart += "--" + boundary + "\r\n";
  bodyStart += "Content-Disposition: form-data; name=\"photo\"; filename=\"esp32cam.jpg\"\r\n";
  bodyStart += "Content-Type: image/jpeg\r\n\r\n";
  
  String bodyEnd = "\r\n--" + boundary + "--\r\n";
  
  int contentLength = bodyStart.length() + fb->len + bodyEnd.length();
  
  // Send HTTP headers
  client.println("POST /bot" + BOT_TOKEN + "/sendPhoto HTTP/1.1");
  client.println("Host: api.telegram.org");
  client.println("Content-Type: " + contentType);
  client.println("Content-Length: " + String(contentLength));
  client.println("Connection: close");
  client.println();
  
  // Send the body start
  client.print(bodyStart);
  
  // Send image data in chunks
  const size_t chunkSize = 1024;
  size_t totalSent = 0;
  
  while (totalSent < fb->len) {
    size_t remainingBytes = fb->len - totalSent;
    size_t currentChunk = (remainingBytes < chunkSize) ? remainingBytes : chunkSize;
    
    size_t sent = client.write(fb->buf + totalSent, currentChunk);
    totalSent += sent;
    
    if (sent == 0) {
      Serial.println("Failed to send image data");
      client.stop();
      return false;
    }
    
    // Small delay and yield to prevent watchdog
    delay(1);
    yield();
  }
  
  // Send the body end
  client.print(bodyEnd);
  
  // Wait for response
  unsigned long timeout = millis() + 10000; // 10 second timeout
  while (client.connected() && millis() < timeout) {
    if (client.available()) {
      String response = client.readString();
      
      if (response.indexOf("HTTP/1.1 200 OK") >= 0) {
        client.stop();
        return true;
      } else {
        Serial.println("Telegram API error in response");
        client.stop();
        return false;
      }
    }
    delay(10);
  }
  
  Serial.println("Timeout waiting for response");
  client.stop();
  return false;
}

void setupLedFlash(int pin) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  Serial.printf("LED Flash setup on pin %d\n", pin);
}
