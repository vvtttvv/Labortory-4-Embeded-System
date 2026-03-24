#pragma once

#include <Arduino.h>

#include "ActuatorBinary.h"
#include "KeypadInput.h"
#include "LcdStatusDisplay.h"
#include "MotorL298.h"
#include "SerialCommandInput.h"

class AppController {
public:
    AppController() = default;

    void setup();
    void loop();

private:
    void printHelp();
    void updateLcdStatus();
    void printStatus();
    void setMotorRawCommand(int rawPercent);
    void applyKeypadSpeedBuffer();
    void handleKeypadKey(char key);
    void runAnalogConditioningAndControl();
    void handleSerialCommand(const SerialCommandInput::Command& command);

    static void keypadEventThunk(char key, void* context);
    static void serialEventThunk(const SerialCommandInput::Command& command, void* context);

    ActuatorBinary binaryActuator_;
    MotorL298 motor_;
    LcdStatusDisplay lcd_;
    KeypadInput keypad_;
    SerialCommandInput serialInput_;

    unsigned long lastControlMs_ = 0;
    unsigned long lastReportMs_ = 0;

    char keypadSpeedBuf_[4] = {'\0', '\0', '\0', '\0'};
    size_t keypadSpeedLen_ = 0;

    int motorRawPercent_ = 0;
    int motorSatPercent_ = 0;
    int motorMedianPercent_ = 0;
    int motorFilteredPercent_ = 0;
    int rawHist_[3] = {0, 0, 0};

    bool alertLimitReached_ = false;
};
