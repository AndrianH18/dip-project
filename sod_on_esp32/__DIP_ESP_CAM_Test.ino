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

static int filter_cb(int width, int height)
    {
      /* A filter callback invoked by the blob routine each time
       * a potential blob region is identified.
       * We use the `width` and `height` parameters supplied
       * to discard regions of non interest (i.e. too big or too small).
       */
      if ((width > 300 && height > 300) || width < 35 || height < 35) {
        /* Ignore small or big boxes */
        return 0;
      }
      return 1; /* Region accepted */
    }




void loop() {
  while (Serial.available()) {

    unsigned long start_time = millis();

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


    // TIME FOR SOME SOD MAGIC!
    sod_img input_img = sod_make_empty_image(fb->width, fb->height, SOD_IMG_GRAYSCALE);
    input_img.data = fb->buf;
    
//    sod_img output_img = sod_sobel_image(sod_gaussian_noise_reduce(input_img));  // Do not reduce noise.
//    sod_threshold_image(output_img, 210);
//    sod_otsu_binarize_image(output_img);
//    sod_sobel_threshold_image(output_img, 30);

/* Blobs detection and draw box */

    sod_img output_img = sod_binarize_image(input_img, 1);
    

    
    sod_box *box = 0;
    int i, nbox;
    sod_image_find_blobs(output_img, &box, &nbox, filter_cb);
    /* `filter_cb`: Our filter callback which was defined above and used to discard small and big regions */
    /*
     * Draw a rectangle on each extracted & validated blob region.
     */
    Serial.print("number of box :");
    Serial.println(nbox);
    for (i = 0; i < nbox; i++) {
      sod_image_draw_bbox_width(input_img, box[i], 5, 255, 255, 255); /* rose box */
    }





//    sod_pts * aLines;
//    int i, nPts, nLines;
//    /* Perform hough line detection on the canny edged image
//     * Depending on the analyzed image/frame, you should experiment
//     * with different thresholds for best results.
//     */
//    aLines = sod_hough_lines_detect(output_img, 100, &nPts);
//    /* Report */
//    nLines = nPts / 2;
//    printf("%d line(s) were detected\n", nLines);
//    /* Draw a rose line for each entry on the full color image copy */
//    for (i = 0; i < nLines; i += 2) {
//      sod_image_draw_line(output_img, aLines[i], aLines[i + 1], 255, 255, 255);
//    }
//    for (i = 0; i < nPts; i++){
//      Serial.print(aLines[i].x);
//      Serial.print(" ");
//      Serial.println(aLines[i].y);
//    }
    Serial.println(millis() - start_time);
    // Print out some information and image metadata.
    Serial.print("Width:");
    Serial.println(output_img.w);
    Serial.print("Height:");
    Serial.println(output_img.h);
    Serial.println(" ");

    // Print image array.
    if (input_img.data != NULL && input_img.data != 0) {
      for (unsigned int i = 0; i < 240; i++) {
        for (unsigned int j = 0; j < 320 - 1; j++) {
          Serial.print( *(input_img.data + (320 * i + j)) );
          Serial.print(F(", "));
        }
        Serial.println( *(input_img.data + (320 * i + 320 - 1)) );
      }
    }
    else {
      Serial.println("output_img pointer is NULL or 0!");
    }
    
    Serial.println(F(" "));

    print_free_psram();

    esp_camera_fb_return(fb);    // Return buffer to be reused.
    flush_serial_buffer();       // Flush input buffer.



//    sod_hough_lines_release(aLines);
    sod_free_image(input_img);
    sod_free_image(output_img);
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
