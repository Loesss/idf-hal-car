#pragma once
#include "esp_camera.h"

// Initializes the camera. Returns true on success.
bool cameraInit();

// Captures a frame, compresses it to JPEG. Caller must free jpg_buf via free().
// Returns false if the frame could not be captured or compressed.
bool cameraCaptureJpeg(uint8_t **jpg_buf, size_t *jpg_len);