#pragma once

#include <Arduino.h>

class ActuatorBinary {
public:
    void init(uint8_t pin, bool activeHigh, bool initialState = false);
    void requestState(bool desiredState, unsigned long nowMs);
    bool update(unsigned long nowMs, unsigned long debounceMs);

    bool getState() const;
    bool getPendingState() const;

private:
    void applyToPin(bool state);

    uint8_t pin_ = 0;
    bool activeHigh_ = true;
    bool stableState_ = false;
    bool pendingValid_ = false;
    bool pendingState_ = false;
    unsigned long pendingSinceMs_ = 0;
};
