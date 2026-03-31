# Lab 5 Report - Main Code Parts

## 1) Arduino Entry Point (src/main.cpp)

```cpp
#include "AppController.h"

namespace {
AppController g_app;
}

void setup()
{
	g_app.setup();
}

void loop()
{
	g_app.loop();
}
```

## 2) Application Architecture (lib/AppController/AppController.h)

```cpp
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
```

## 3) Setup + Main Loop (lib/AppController/AppController.cpp)

```cpp
void AppController::setup()
{
    UartStdio::init(9600);

    binaryActuator_.init(kBinaryActuatorPin, true, false);
    motor_.init(kMotorIn1Pin, kMotorIn2Pin, kMotorEnPin);
    serialInput_.init(serialEventThunk, this);
    keypad_.init(kKeypadRowPins, kKeypadColPins, kKeypadMap, kKeypadDebounceMs);
    keypad_.setKeyHandler(keypadEventThunk, this);

    lcd_.init(kLcdRsPin, kLcdEnPin, kLcdD4Pin, kLcdD5Pin, kLcdD6Pin, kLcdD7Pin);
    lcd_.showBoot();

    printHelp();
    printStatus();
    updateLcdStatus();
}

void AppController::loop()
{
    const unsigned long now = millis();
    if (now - lastInputTaskMs_ >= kInputTaskPeriodMs) {
        lastInputTaskMs_ = now;
        runTaskCommandInput(now);
    }

    if (now - lastBinaryTaskMs_ >= kBinaryTaskPeriodMs) {
        lastBinaryTaskMs_ = now;
        runTaskBinaryControl(now);
    }

    if (now - lastAnalogTaskMs_ >= kAnalogTaskPeriodMs) {
        lastAnalogTaskMs_ = now;
        runTaskAnalogControl();
    }

    if (now - lastReportTaskMs_ >= kReportPeriodMs) {
        lastReportTaskMs_ = now;
        runTaskReporting();
    }
}
```

## 3.1) Repartizarea taskurilor (cooperativ, fara FreeRTOS)

```text
Task_Input (10 ms): interpretare comenzi seriale + scanare keypad
Task_BinaryControl (50 ms): debouncing software + aplicare ON/OFF actuator binar
Task_AnalogControl (50 ms): saturare + filtru median + mediere ponderata + rampare motor
Task_Reporting (500 ms): raport structurat pe STDIO + actualizare LCD
```

## 4) Keypad Command Handling (lib/AppController/AppController.cpp)

```cpp
void AppController::handleKeypadKey(char key)
{
    if (key >= '0' && key <= '9') {
        if (keypadSpeedLen_ < 3) {
            keypadSpeedBuf_[keypadSpeedLen_++] = key;
            keypadSpeedBuf_[keypadSpeedLen_] = '\0';
            printf("KEYPAD speed buf: %s\n", keypadSpeedBuf_);
        }
        return;
    }

    switch (key) {
    case 'A':
        binaryActuator_.requestState(true, millis());
        printf("ACK BIN ON (keypad)\n");
        break;
    case 'B':
        binaryActuator_.requestState(false, millis());
        printf("ACK BIN OFF (keypad)\n");
        break;
    case 'C':
        motor_.setDirection(MotorL298::Forward);
        printf("ACK MOT DIR FWD (keypad)\n");
        break;
    case 'D':
        motor_.setDirection(MotorL298::Reverse);
        printf("ACK MOT DIR REV (keypad)\n");
        break;
    case '#':
        applyKeypadSpeedBuffer();
        break;
    case '*':
        keypadSpeedLen_ = 0;
        keypadSpeedBuf_[0] = '\0';
        printf("KEYPAD speed buffer cleared\n");
        break;
    default:
        break;
    }
}
```

## 5) Analog Signal Conditioning (lib/AppController/AppController.cpp)

```cpp
void AppController::runAnalogConditioningAndControl()
{
    motorSatPercent_ = SignalConditioning::clampInt(motorRawPercent_, 0, 100);
    alertLimitReached_ = (motorSatPercent_ != motorRawPercent_);

    rawHist_[2] = rawHist_[1];
    rawHist_[1] = rawHist_[0];
    rawHist_[0] = motorSatPercent_;

    motorMedianPercent_ = SignalConditioning::median3(rawHist_[0], rawHist_[1], rawHist_[2]);
    motorFilteredPercent_ = SignalConditioning::weightedAverageInt(motorFilteredPercent_, motorMedianPercent_, 1, 4);

    motor_.setTargetPercent(motorFilteredPercent_);
    motor_.updateRamp(kMotorRampStepPercent);
}
```

## 6) Serial Command Handling (lib/AppController/AppController.cpp)

```cpp
void AppController::handleSerialCommand(const SerialCommandInput::Command& command)
{
    switch (command.type) {
    case SerialCommandInput::Help:
        printHelp();
        break;
    case SerialCommandInput::Status:
        printStatus();
        break;
    case SerialCommandInput::BinOn:
        binaryActuator_.requestState(true, millis());
        printf("ACK BIN ON\n");
        break;
    case SerialCommandInput::BinOff:
        binaryActuator_.requestState(false, millis());
        printf("ACK BIN OFF\n");
        break;
    case SerialCommandInput::MotSpd:
        setMotorRawCommand(command.value);
        printf("ACK MOT SPD RAW %d\n", motorRawPercent_);
        break;
    case SerialCommandInput::MotDirFwd:
        motor_.setDirection(MotorL298::Forward);
        printf("ACK MOT DIR FWD\n");
        break;
    case SerialCommandInput::MotDirRev:
        motor_.setDirection(MotorL298::Reverse);
        printf("ACK MOT DIR REV\n");
        break;
    case SerialCommandInput::Unknown:
    default:
        printf("ERR Unknown command. Type HELP\n");
        break;
    }
}
```

## 7) Key Module Interfaces (Modularity)

### 7.1 Keypad Module (lib/KeypadInput/KeypadInput.h)

```cpp
class KeypadInput {
public:
    using KeyHandler = void (*)(char key, void* context);

    void init(const uint8_t rowPins[4], const uint8_t colPins[4], const char keyMap[4][4], unsigned long debounceMs);
    void setKeyHandler(KeyHandler handler, void* context);
    void poll(unsigned long nowMs);
};
```

### 7.2 Serial Parser Module (lib/SerialCommandInput/SerialCommandInput.h)

```cpp
class SerialCommandInput {
public:
    enum CommandType {
        Help, Status, BinOn, BinOff, MotSpd, MotDirFwd, MotDirRev, Unknown,
    };

    struct Command {
        CommandType type;
        int value;
    };

    using CommandHandler = void (*)(const Command& command, void* context);

    void init(CommandHandler handler, void* context);
    void poll();
};
```

### 7.3 LCD Module (lib/LcdStatusDisplay/LcdStatusDisplay.h)

```cpp
class LcdStatusDisplay {
public:
    void init(uint8_t rsPin, uint8_t enPin, uint8_t d4Pin, uint8_t d5Pin, uint8_t d6Pin, uint8_t d7Pin);
    void showBoot();
    void showStatus(bool binaryOn, bool motorForward, int motorRawPercent, int motorAppliedPercent, bool limitAlert);
};
```

### 7.4 Binary Actuator Module (lib/ActuatorBinary/ActuatorBinary.h)

```cpp
class ActuatorBinary {
public:
    void init(uint8_t pin, bool activeHigh, bool initialState = false);
    void requestState(bool desiredState, unsigned long nowMs);
    bool update(unsigned long nowMs, unsigned long debounceMs);

    bool getState() const;
    bool getPendingState() const;
};
```

### 7.5 Analog Actuator Module (lib/MotorL298/MotorL298.h)

```cpp
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
};
```

### 7.6 Signal Conditioning Utils (lib/SignalConditioning/SignalConditioning.h)

```cpp
class SignalConditioning {
public:
    static int clampInt(int value, int minValue, int maxValue);
    static int median3(int a, int b, int c);
    static int rampToward(int current, int target, int maxDelta);
    static int weightedAverageInt(int prev, int input, int alphaNum, int alphaDen);
};
```
