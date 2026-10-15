#include "car_control.h"
#include "hal_motor.h"
#include "esp_log.h"

static const char *TAG = "CAR";

const char* carCommandName(CarCommand cmd) {
    switch (cmd) {
        case CAR_FORWARD:  return "FORWARD";
        case CAR_BACKWARD: return "BACKWARD";
        case CAR_LEFT:     return "LEFT";
        case CAR_RIGHT:    return "RIGHT";
        default:           return "STOP";
    }
}

// Tank-style control logic: command -> two motors
void carExecute(CarCommand cmd) {
    constexpr uint8_t SPEED = 200;   // base speed

    ESP_LOGI(TAG, "команда: %s", carCommandName(cmd));

    switch (cmd) {
        case CAR_FORWARD:
            motorSet(MOTOR_LEFT,  DIR_FORWARD, SPEED);
            motorSet(MOTOR_RIGHT, DIR_FORWARD, SPEED);
            break;
        case CAR_BACKWARD:
            motorSet(MOTOR_LEFT,  DIR_BACKWARD, SPEED);
            motorSet(MOTOR_RIGHT, DIR_BACKWARD, SPEED);
            break;
        case CAR_LEFT:   // left side back, right side forward -> turn left
            motorSet(MOTOR_LEFT,  DIR_BACKWARD, SPEED);
            motorSet(MOTOR_RIGHT, DIR_FORWARD,  SPEED);
            break;
        case CAR_RIGHT:  // opposite
            motorSet(MOTOR_LEFT,  DIR_FORWARD,  SPEED);
            motorSet(MOTOR_RIGHT, DIR_BACKWARD, SPEED);
            break;
        case CAR_STOP:
        default:
            motorStopAll();
            break;
    }
}