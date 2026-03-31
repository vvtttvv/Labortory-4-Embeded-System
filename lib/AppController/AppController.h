#pragma once

#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>

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
    void taskCommandInput();
    void taskBinaryControl();
    void taskAnalogControl();
    void taskReporting();

private:
    enum KeypadTarget {
        KeypadTargetPosition = 0,
        KeypadTargetSpeed = 1,
    };

    struct StatusSnapshot {
        bool binaryState;
        bool pendingState;
        int raw;
        int sat;
        int med;
        int filt;
        int target;
        int applied;
        int speed;
        bool limitAlert;
    };

    void lockState();
    void unlockState();
    StatusSnapshot makeStatusSnapshotLocked() const;

    void printHelp();
    void updateLcdStatus();
    void updateLcdStatus(const StatusSnapshot& snapshot);
    void printStatus();
    void printStatus(const StatusSnapshot& snapshot);
    void setMotorRawCommand(int rawPercent);
    void applyKeypadBuffer();
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
    SemaphoreHandle_t stateMutex_ = nullptr;

    char keypadSpeedBuf_[4] = {'\0', '\0', '\0', '\0'};
    size_t keypadSpeedLen_ = 0;
    KeypadTarget keypadTarget_ = KeypadTargetPosition;

    int motorRawPercent_ = 0;
    int motorSatPercent_ = 0;
    int motorMedianPercent_ = 0;
    int motorFilteredPercent_ = 0;
    int motorSpeedPercent_ = 40;
    int rawHist_[3] = {0, 0, 0};

    bool alertLimitReached_ = false;
};
