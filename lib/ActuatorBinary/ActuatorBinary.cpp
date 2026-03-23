#include "ActuatorBinary.h"

void ActuatorBinary::init(uint8_t pin, bool activeHigh, bool initialState)
{
    pin_ = pin;
    activeHigh_ = activeHigh;
    stableState_ = initialState;
    pendingState_ = initialState;
    pendingValid_ = false;
    pendingSinceMs_ = 0;

    pinMode(pin_, OUTPUT);
    applyToPin(stableState_);
}

void ActuatorBinary::requestState(bool desiredState, unsigned long nowMs)
{
    if (desiredState == stableState_ && !pendingValid_) {
        return;
    }

    if (!pendingValid_ || desiredState != pendingState_) {
        pendingState_ = desiredState;
        pendingSinceMs_ = nowMs;
        pendingValid_ = true;
    }
}

bool ActuatorBinary::update(unsigned long nowMs, unsigned long debounceMs)
{
    if (!pendingValid_) {
        return false;
    }

    if (nowMs - pendingSinceMs_ < debounceMs) {
        return false;
    }

    pendingValid_ = false;
    if (stableState_ == pendingState_) {
        return false;
    }

    stableState_ = pendingState_;
    applyToPin(stableState_);
    return true;
}

bool ActuatorBinary::getState() const
{
    return stableState_;
}

bool ActuatorBinary::getPendingState() const
{
    return pendingValid_ ? pendingState_ : stableState_;
}

void ActuatorBinary::applyToPin(bool state)
{
    const uint8_t level = (state == activeHigh_) ? HIGH : LOW;
    digitalWrite(pin_, level);
}
