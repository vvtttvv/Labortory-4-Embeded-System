#include "KeypadInput.h"

void KeypadInput::init(const uint8_t rowPins[4], const uint8_t colPins[4], const char keyMap[4][4], unsigned long debounceMs)
{
    for (uint8_t i = 0; i < 4; ++i) {
        rowPins_[i] = rowPins[i];
        colPins_[i] = colPins[i];
    }

    for (uint8_t row = 0; row < 4; ++row) {
        for (uint8_t col = 0; col < 4; ++col) {
            keyMap_[row][col] = keyMap[row][col];
        }
    }

    debounceMs_ = debounceMs;
    candidate_ = '\0';
    stable_ = '\0';
    candidateSinceMs_ = 0;

    for (uint8_t row = 0; row < 4; ++row) {
        pinMode(rowPins_[row], OUTPUT);
        digitalWrite(rowPins_[row], HIGH);
    }

    for (uint8_t col = 0; col < 4; ++col) {
        pinMode(colPins_[col], INPUT_PULLUP);
    }
}

void KeypadInput::setKeyHandler(KeyHandler handler, void* context)
{
    keyHandler_ = handler;
    keyHandlerContext_ = context;
}

void KeypadInput::poll(unsigned long nowMs)
{
    const char raw = scanRaw();

    if (raw != candidate_) {
        candidate_ = raw;
        candidateSinceMs_ = nowMs;
    }

    if (stable_ == candidate_) {
        return;
    }

    if (nowMs - candidateSinceMs_ < debounceMs_) {
        return;
    }

    stable_ = candidate_;
    if (stable_ != '\0' && keyHandler_ != nullptr) {
        keyHandler_(stable_, keyHandlerContext_);
    }
}

char KeypadInput::scanRaw() const
{
    for (uint8_t row = 0; row < 4; ++row) {
        for (uint8_t r = 0; r < 4; ++r) {
            digitalWrite(rowPins_[r], HIGH);
        }
        digitalWrite(rowPins_[row], LOW);
        delayMicroseconds(3);

        for (uint8_t col = 0; col < 4; ++col) {
            if (digitalRead(colPins_[col]) == LOW) {
                return keyMap_[row][col];
            }
        }
    }

    return '\0';
}
