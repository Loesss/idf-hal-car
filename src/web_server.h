#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Starts the WiFi AP + web server (video + control).
// commandQueue — where to push control commands when a button is pressed.
void webServerStart(QueueHandle_t commandQueue);