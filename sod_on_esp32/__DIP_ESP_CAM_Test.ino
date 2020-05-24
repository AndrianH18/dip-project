#include "esp_camera.h"
#include "Arduino.h"
#include "WiFi.h"
#include "sod_mod.h"

// Settings
#define ENABLE_FLASH    1

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
    digitalWrite(4, LOW);
    if (!fb) {
      Serial.println("Camera capture failed");
      delay(50);   // Wait until all characters arrive via UART.
      flush_serial_buffer();
      break;
    }

    delay(50);   // Wait until all characters arrive via UART.
    flush_serial_buffer();


    // TIME FOR SOME SOD MAGIC!
    sod_img input_img = sod_make_empty_image(fb->width, fb->height, SOD_IMG_GRAYSCALE);
    input_img.data = fb->buf;

    sod_img output_img = sod_canny_edge_image(input_img, 0);  // Do not reduce noise.

    // Print out some information and image metadata.
    Serial.print("Width:");
    Serial.println(output_img.w);
    Serial.print("Height:");
    Serial.println(output_img.h);
    Serial.println(" ");

    // Print image array.
    if (output_img.data != NULL && output_img.data != 0) {
      for (unsigned int i = 0; i < 240; i++) {
        for (unsigned int j = 0; j < 320 - 1; j++) {
          Serial.print( *(output_img.data + (320 * i + j)) );
          Serial.print(F(", "));
        }
        Serial.println( *(output_img.data + (320 * i + 320 - 1)) );
      }
    }
    else {
      Serial.println("output_img pointer is NULL or 0!");
    }

    Serial.println(F(" "));

    esp_camera_fb_return(fb);    // Return buffer to be reused.

    // Flush input buffer.
    while (Serial.available())
      Serial.read();
  }
}


void pause(String message) {
  Serial.println(message);
  while (!Serial.available());
  delay(200);
  while (Serial.available())
    Serial.read();
}


void flush_serial_buffer() {
  while (Serial.available())
    Serial.read();
}


void print_free_psram() {
  Serial.print(F("Free PSRAM: "));
  Serial.println(ESP.getFreePsram());
  Serial.println(F(" "));
}
