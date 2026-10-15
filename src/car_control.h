#pragma once

// High-level commands (what the driver wants)
enum CarCommand {
    CAR_STOP = 0,
    CAR_FORWARD,
    CAR_BACKWARD,
    CAR_LEFT,
    CAR_RIGHT
};

void carExecute(CarCommand cmd);   // execute the command (via motor HAL)
const char* carCommandName(CarCommand cmd);