#include "esp_camera.h"
#include "Arduino.h"
#include "WiFi.h"
#include "sod_mod.h"

// Settings
#define ENABLE_FLASH    1
#define PRINT_IMG_INFO  0

// Pin definition for CAMERA_MODEL_AI_THINKER
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

camera_fb_t * fb = NULL;


void setup() {

  if (ENABLE_FLASH)
    pinMode(4, OUTPUT);

  Serial.begin(500000);
  delay(500);
  Serial.println("\nSerial communication established.");

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
  config.pixel_format = PIXFORMAT_GRAYSCALE;   // Set to RAW GRAYSCALE to avoid compression.

  if (psramFound()) {
    config.frame_size = FRAMESIZE_QVGA;   // 320x240
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Init Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  Serial.println("=========================================\n");
}


void loop() {
  while (Serial.available()) {
    // Take a picture.
    digitalWrite(4, HIGH);
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      while (Serial.available())
        Serial.read();
      digitalWrite(4, LOW);
      break;
    }

    digitalWrite(4, LOW);


    // TIME FOR SOME SOD MAGIC!
    img = sod_make_empty_image(fb->width, fb->height, 1);
    img.data = fb->buf;
    

    // Print out some information and image metadata.
    if (PRINT_IMG_INFO) {
      Serial.println("Camera capture successful!");
      Serial.print("fb->len:");
      Serial.println(fb->len);
      Serial.print("fb->width:");
      Serial.println(fb->width);
      Serial.print("fb->height:");
      Serial.println(fb->height);

      Serial.println("fb->buf:");
    }

    // Print image array.
    for (unsigned int i = 0; i < 240; i++) {
      for (unsigned int j = 0; j < 320-1; j++) {
        Serial.printf( "%d, ", *(fb->buf + (320 * i + j)) );
      }
      Serial.printf( "%d\n", *(fb->buf + (320 * i + 320-1)) );
    }

    Serial.println(" ");

    esp_camera_fb_return(fb);    // Return buffer to be reused.

    // Flush input buffer.
    while (Serial.available())
      Serial.read();
  }
}
