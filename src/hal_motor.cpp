#include "hal_motor.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "HAL_MOTOR";

// Direction control pins (no ENA/ENB — always full speed)
// constexpr gpio_num_t IN1 = GPIO_NUM_27;   // motor A (left)
// constexpr gpio_num_t IN2 = GPIO_NUM_14;
// constexpr gpio_num_t IN3 = GPIO_NUM_12;   // motor B (right)
// constexpr gpio_num_t IN4 = GPIO_NUM_13;

// Direction control pins (ESP32-CAM, no ENA/ENB — always full speed)
constexpr gpio_num_t IN1 = GPIO_NUM_13;   // motor A (left)
constexpr gpio_num_t IN2 = GPIO_NUM_14;
constexpr gpio_num_t IN3 = GPIO_NUM_15;   // motor B (right)
constexpr gpio_num_t IN4 = GPIO_NUM_2;

static const char* dirName(MotorDir d) {
    switch (d) {
        case DIR_FORWARD:  return "FORWARD";
        case DIR_BACKWARD: return "BACKWARD";
        default:           return "STOP";
    }
}
static const char* motorName(Motor m) {
    return (m == MOTOR_LEFT) ? "LEFT" : "RIGHT";
}

void motorInit() {
    gpio_reset_pin(IN1);
    gpio_reset_pin(IN2);
    gpio_reset_pin(IN3);
    gpio_reset_pin(IN4);

    gpio_set_direction(IN1, GPIO_MODE_OUTPUT);
    gpio_set_direction(IN2, GPIO_MODE_OUTPUT);
    gpio_set_direction(IN3, GPIO_MODE_OUTPUT);
    gpio_set_direction(IN4, GPIO_MODE_OUTPUT);

    gpio_set_level(IN1, 0);
    gpio_set_level(IN2, 0);
    gpio_set_level(IN3, 0);
    gpio_set_level(IN4, 0);

    ESP_LOGI(TAG, "motorInit() — реальні піни готові");
}

// speed is ignored for now (no ENA/ENB) — motor is always full speed
void motorSet(Motor m, MotorDir dir, uint8_t speed) {
    gpio_num_t pinA, pinB;
    if (m == MOTOR_LEFT) { pinA = IN1; pinB = IN2; }
    else                 { pinA = IN3; pinB = IN4; }

    ESP_LOGI(TAG, "motorSet: motor=%d dir=%d, pinA=%d, pinB=%d", (int)m, (int)dir, (int)pinA, (int)pinB); 

    switch (dir) {
        case DIR_FORWARD:
            gpio_set_level(pinA, 1);
            gpio_set_level(pinB, 0);
            break;
        case DIR_BACKWARD:
            gpio_set_level(pinA, 0);
            gpio_set_level(pinB, 1);
            break;
        default: // STOP
            gpio_set_level(pinA, 0);
            gpio_set_level(pinB, 0);
            break;
    }
    ESP_LOGI(TAG, "motor %-5s -> %-8s", motorName(m), dirName(dir));
}

void motorStopAll() {
    motorSet(MOTOR_LEFT,  DIR_STOP, 0);
    motorSet(MOTOR_RIGHT, DIR_STOP, 0);
}