#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "driver/ledc.h"

#include "human_face_detect_msr01.hpp"

// Select camera model
#define CAMERA_MODEL_AI_THINKER

#include "camera_pins.h"

// Enable LED FLASH setting
#define CONFIG_LED_ILLUMINATOR_ENABLED 0

// LED FLASH setup
#if CONFIG_LED_ILLUMINATOR_ENABLED
#define LED_LEDC_CHANNEL 2
#define CONFIG_LED_MAX_INTENSITY 10
int led_duty = 0;
#endif

static const char* TAG = "face_tracking";

static HumanFaceDetectMSR01 s1(0.1F, 0.5F, 10, 0.2F);

#if CONFIG_LED_ILLUMINATOR_ENABLED
void enable_led(bool en)
{
    int duty = en ? led_duty : 0;
    ledcWrite(LED_LEDC_CHANNEL, duty);
    log_i("Set LED intensity to %d", duty);
}
#endif

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

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
  config.pixel_format = PIXFORMAT_RGB565;
  config.frame_size = FRAMESIZE_QVGA;
  config.fb_count = 1;

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
    return;
  }

  // Adjust image processing settings
  sensor_t *s = esp_camera_sensor_get();
  s->set_brightness(s, 1);     // Increase brightness to 1 for better visibility in low light
  s->set_contrast(s, 1);       // Increase contrast to 1 for better detail in low light
  s->set_saturation(s, 0);     // Keep saturation at default (0) to avoid oversaturation
  s->set_special_effect(s, 0); // No special effect (0) to maintain normal color representation
  s->set_whitebal(s, 1);       // Enable white balance (1) for automatic color correction
  s->set_awb_gain(s, 1);       // Enable AWB gain (1) for automatic white balance adjustment
  s->set_wb_mode(s, 0);        // Set white balance mode to Auto (0) for automatic adaptation
  s->set_exposure_ctrl(s, 1);  // Enable exposure control (1) for automatic exposure adjustment
  s->set_aec2(s, 1);           // Enable AEC2 (1) for enhanced automatic exposure control
  s->set_ae_level(s, 1);       // Increase exposure level to 1 for brighter exposure in low light
  s->set_aec_value(s, 600);    // Increase AEC value to 600 for longer exposure time in low light
  s->set_gain_ctrl(s, 1);      // Enable gain control (1) for automatic gain adjustment
  s->set_agc_gain(s, 15);      // Set AGC gain to 15 for moderate gain boost in low light
  s->set_gainceiling(s, (gainceiling_t)30);  // Set gain ceiling to 4 for increased maximum gain limit
  s->set_bpc(s, 1);            // Enable black pixel correction (1) to reduce noise in low light
  s->set_wpc(s, 1);            // Enable white pixel correction (1) to reduce noise in low light
  s->set_raw_gma(s, 1);        // Enable gamma correction (1) for better contrast and brightness
  s->set_lenc(s, 1);           // Enable lens correction (1) to compensate for lens distortion
  s->set_hmirror(s, 0);        // Disable horizontal mirror (0) to maintain original orientation
  s->set_vflip(s, 1);          // Disable vertical flip (0) to maintain original orientation
  s->set_dcw(s, 1);            // Enable DCW (1) for enhanced dynamic range in low light
  s->set_colorbar(s, 0);       // Disable color bar (0) to avoid interference with face detection

  #if CONFIG_LED_ILLUMINATOR_ENABLED
  ledcSetup(LED_LEDC_CHANNEL, 5000, 8);
  ledcAttachPin(4, LED_LEDC_CHANNEL);  // LED Flash connected to GPIO 4
  led_duty = CONFIG_LED_MAX_INTENSITY;
  #endif
}

void loop() {
  static uint32_t frame_count = 0;
  static uint32_t start_time = millis();

  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    ESP_LOGE(TAG, "Camera capture failed");
    return;
  }

  #if CONFIG_LED_ILLUMINATOR_ENABLED
  // Turn on LED flash
  enable_led(true);
  vTaskDelay(100 / portTICK_PERIOD_MS); // Delay to keep the LED on for a short duration
  #endif

  // Print frame information
  // Serial.print("Frame size: ");
  // Serial.print(fb->width);
  // Serial.print(" x ");
  // Serial.println(fb->height);
  // Serial.print("Frame format: ");
  // Serial.println(fb->format);

  std::list<dl::detect::result_t> &results = s1.infer((uint16_t*)fb->buf, {fb->height, fb->width, 3});

  // Print inference results
  // Serial.print("Number of faces detected: ");
  // Serial.println(results.size());

  if (results.size() > 0) {
    dl::detect::result_t face = results.front();
    int face_x = (face.box[0] + face.box[2]) / 2;
    int face_y = (face.box[1] + face.box[3]) / 2;
    int frame_center_x = fb->width / 2;

    // Print face coordinates
    // Serial.print("Face coordinates: (");
    // Serial.print(face_x);
    // Serial.print(", ");
    // Serial.print(face_y);
    // Serial.println(")");

    if (face_x < frame_center_x) {
      int distance = frame_center_x - face_x;
      Serial.print("Face is to the left, distance from center: ");
      Serial.println(distance);
    } else if (face_x > frame_center_x) {
      int distance = face_x - frame_center_x;
      Serial.print("Face is to the right, distance from center: ");
      Serial.println(distance);
    } else {
      Serial.println("Face is at the center");
    } }
  // } else {
  //   Serial.println("No face detected");
  // }

  #if CONFIG_LED_ILLUMINATOR_ENABLED
  // Turn off LED flash
  enable_led(false);
  #endif

  esp_camera_fb_return(fb);

  // Calculate and print FPS
  frame_count++;
  uint32_t elapsed_time = millis() - start_time;
  if (elapsed_time >= 1000) {
    float fps = frame_count / (elapsed_time / 1000.0);
    Serial.print("FPS: ");
    Serial.println(fps);
    frame_count = 0;
    start_time = millis();
  }

  // delay(100);
}