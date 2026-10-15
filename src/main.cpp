#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "car_control.h"
#include "hal_motor.h"
#include "hal_camera.h"
#include "web_server.h"

static const char *TAG = "MAIN";

QueueHandle_t commandQueue;

// Control task — CORE 1, separate from video/web server
void carTask(void *param) {
    CarCommand cmd;
    while (1) {
        xQueueReceive(commandQueue, &cmd, portMAX_DELAY);
        ESP_LOGI(TAG, "carTask отримав команду: %d", (int)cmd);
        carExecute(cmd);
    }
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "=== Car + Camera boot ===");

    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();

    // --- Camera (HAL) ---
    if (!cameraInit()) {
        ESP_LOGE(TAG, "Продовжуємо БЕЗ відео — керування моторами й надалі працює");
    } else {
        vTaskDelay(pdMS_TO_TICKS(1000));   // power stabilization
    }

    // --- Motors (HAL) + control task on CORE 1 ---
    motorInit();
    commandQueue = xQueueCreate(10, sizeof(CarCommand));
    xTaskCreatePinnedToCore(carTask, "car", 4096, NULL, 2, NULL, 1);

    // --- WiFi + web server (video on CORE 0, inside web_server.cpp) ---
    webServerStart(commandQueue);

    ESP_LOGI(TAG, "Усе запущено. Підключайся до ESP32-Car і відкривай 192.168.4.1");
}