#pragma once
#include <stdint.h>

// Which motor (side of the car)
enum Motor {
    MOTOR_LEFT = 0,
    MOTOR_RIGHT
};

// Direction
enum MotorDir {
    DIR_STOP = 0,
    DIR_FORWARD,
    DIR_BACKWARD
};

// Motor HAL interface — logic code uses ONLY this,
// without knowing about pins/driver.
void motorInit();                                   // configure (pins/PWM)
void motorSet(Motor m, MotorDir dir, uint8_t speed); // direction + speed 0..255
void motorStopAll();                                 // stop both