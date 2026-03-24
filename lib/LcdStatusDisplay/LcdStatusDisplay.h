#pragma once

#include <Arduino.h>

class LcdStatusDisplay {
public:
    void init(uint8_t rsPin, uint8_t enPin, uint8_t d4Pin, uint8_t d5Pin, uint8_t d6Pin, uint8_t d7Pin);
    void showBoot();
    void showStatus(bool binaryOn, bool motorForward, int motorRawPercent, int motorAppliedPercent, bool limitAlert);

private:
    void clearLine(uint8_t row);

    bool initialized_ = false;
    uint8_t rsPin_ = 0;
    uint8_t enPin_ = 0;
    uint8_t d4Pin_ = 0;
    uint8_t d5Pin_ = 0;
    uint8_t d6Pin_ = 0;
    uint8_t d7Pin_ = 0;
};
