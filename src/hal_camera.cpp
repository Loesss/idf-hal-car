#include "hal_camera.h"
#include "img_converters.h"
#include "esp_log.h"

static const char *TAG = "HAL_CAMERA";

// --- Camera pins (RHYX M21-45 / AI-Thinker pinout) ---
#define CAM_PIN_PWDN    32
#define CAM_PIN_RESET   -1
#define CAM_PIN_XCLK     0
#define CAM_PIN_SIOD    26
#define CAM_PIN_SIOC    27
#define CAM_PIN_Y9      35
#define CAM_PIN_Y8      34
#define CAM_PIN_Y7      39
#define CAM_PIN_Y6      36
#define CAM_PIN_Y5      21
#define CAM_PIN_Y4      19
#define CAM_PIN_Y3      18
#define CAM_PIN_Y2       5
#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    23
#define CAM_PIN_PCLK    22

bool cameraInit() {
    camera_config_t config = {};
    config.pin_pwdn = CAM_PIN_PWDN;
    config.pin_reset = CAM_PIN_RESET;
    config.pin_xclk = CAM_PIN_XCLK;
    config.pin_sccb_sda = CAM_PIN_SIOD;
    config.pin_sccb_scl = CAM_PIN_SIOC;
    config.pin_d7 = CAM_PIN_Y9;
    config.pin_d6 = CAM_PIN_Y8;
    config.pin_d5 = CAM_PIN_Y7;
    config.pin_d4 = CAM_PIN_Y6;
    config.pin_d3 = CAM_PIN_Y5;
    config.pin_d2 = CAM_PIN_Y4;
    config.pin_d1 = CAM_PIN_Y3;
    config.pin_d0 = CAM_PIN_Y2;
    config.pin_vsync = CAM_PIN_VSYNC;
    config.pin_href = CAM_PIN_HREF;
    config.pin_pclk = CAM_PIN_PCLK;
    config.xclk_freq_hz = 10000000;
    config.pixel_format = PIXFORMAT_RGB565;
    config.frame_size = FRAMESIZE_QQVGA;
    config.jpeg_quality = 12;
    config.fb_count = 2;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed: 0x%x (%s)", err, esp_err_to_name(err));
        return false;
    }
    ESP_LOGI(TAG, "Camera init OK");
    return true;
}

bool cameraCaptureJpeg(uint8_t **jpg_buf, size_t *jpg_len) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        return false;
    }
    bool converted = frame2jpg(fb, 30, jpg_buf, jpg_len);
    esp_camera_fb_return(fb);
    return converted;
}