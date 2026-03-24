#pragma once

#include <Arduino.h>

class KeypadInput {
public:
    using KeyHandler = void (*)(char key, void* context);

    void init(const uint8_t rowPins[4], const uint8_t colPins[4], const char keyMap[4][4], unsigned long debounceMs);
    void setKeyHandler(KeyHandler handler, void* context);
    void poll(unsigned long nowMs);

private:
    char scanRaw() const;

    uint8_t rowPins_[4] = {0, 0, 0, 0};
    uint8_t colPins_[4] = {0, 0, 0, 0};
    char keyMap_[4][4] = {
        {'\0', '\0', '\0', '\0'},
        {'\0', '\0', '\0', '\0'},
        {'\0', '\0', '\0', '\0'},
        {'\0', '\0', '\0', '\0'},
    };

    unsigned long debounceMs_ = 0;
    char candidate_ = '\0';
    char stable_ = '\0';
    unsigned long candidateSinceMs_ = 0;

    KeyHandler keyHandler_ = nullptr;
    void* keyHandlerContext_ = nullptr;
};
