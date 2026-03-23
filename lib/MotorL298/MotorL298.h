#pragma once

#include <Arduino.h>

class MotorL298 {
public:
    enum Direction {
        Forward = 0,
        Reverse = 1,
    };

    void init(uint8_t in1Pin, uint8_t in2Pin, uint8_t enPinPwm);
    void setDirection(Direction dir);
    void setTargetPercent(int percent);
    void updateRamp(int maxDeltaPercentPerStep);

    Direction getDirection() const;
    int getTargetPercent() const;
    int getAppliedPercent() const;

private:
    void applyOutput();

    uint8_t in1Pin_ = 0;
    uint8_t in2Pin_ = 0;
    uint8_t enPinPwm_ = 0;

    Direction direction_ = Forward;
    int targetPercent_ = 0;
    int appliedPercent_ = 0;
};
