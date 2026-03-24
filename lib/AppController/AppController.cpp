#include "AppController.h"

#include <stdio.h>
#include <stdlib.h>

#include "SignalConditioning.h"
#include "UartStdio.h"

namespace {
constexpr unsigned long kControlPeriodMs = 20;
constexpr unsigned long kDebounceMs = 60;
constexpr unsigned long kKeypadDebounceMs = 35;
constexpr unsigned long kReportPeriodMs = 500;
constexpr int kMotorRampStepPercent = 2;

constexpr uint8_t kBinaryActuatorPin = 13;
constexpr uint8_t kMotorIn1Pin = 4;
constexpr uint8_t kMotorIn2Pin = 3;
constexpr uint8_t kMotorEnPin = 5;
constexpr uint8_t kLcdRsPin = 12;
constexpr uint8_t kLcdEnPin = 11;
constexpr uint8_t kLcdD4Pin = 10;
constexpr uint8_t kLcdD5Pin = 9;
constexpr uint8_t kLcdD6Pin = 8;
constexpr uint8_t kLcdD7Pin = 7;

constexpr uint8_t kKeypadRowPins[4] = {A0, A1, A2, A3};
constexpr uint8_t kKeypadColPins[4] = {A4, A5, 2, 6};
constexpr char kKeypadMap[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'},
};
} // namespace

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
    serialInput_.poll();
    keypad_.poll(millis());

    const unsigned long now = millis();
    if (now - lastControlMs_ >= kControlPeriodMs) {
        lastControlMs_ = now;
        binaryActuator_.update(now, kDebounceMs);
        runAnalogConditioningAndControl();
    }

    if (now - lastReportMs_ >= kReportPeriodMs) {
        lastReportMs_ = now;
        printStatus();
        updateLcdStatus();
    }
}

void AppController::printHelp()
{
    printf("Commands:\n");
    printf("  HELP\n");
    printf("  STATUS\n");
    printf("  BIN ON|OFF\n");
    printf("  MOT SPD <0..100>\n");
    printf("  MOT DIR FWD|REV\n");
    printf("Keypad:\n");
    printf("  A=BIN ON, B=BIN OFF, C=DIR FWD, D=DIR REV\n");
    printf("  0..9=Speed input, #=Apply speed, *=Clear input\n");
}

void AppController::updateLcdStatus()
{
    lcd_.showStatus(
        binaryActuator_.getState(),
        motor_.getDirection() == MotorL298::Forward,
        motorRawPercent_,
        motor_.getAppliedPercent(),
        alertLimitReached_);
}

void AppController::printStatus()
{
    printf(
        "STAT BIN=%s PENDING=%s MOT_DIR=%s RAW=%d SAT=%d MED=%d FILT=%d TGT=%d APPLIED=%d ALERT_LIMIT=%s\n",
        binaryActuator_.getState() ? "ON" : "OFF",
        binaryActuator_.getPendingState() ? "ON" : "OFF",
        motor_.getDirection() == MotorL298::Forward ? "FWD" : "REV",
        motorRawPercent_,
        motorSatPercent_,
        motorMedianPercent_,
        motorFilteredPercent_,
        motor_.getTargetPercent(),
        motor_.getAppliedPercent(),
        alertLimitReached_ ? "YES" : "NO");
}

void AppController::setMotorRawCommand(int rawPercent)
{
    motorRawPercent_ = rawPercent;
}

void AppController::applyKeypadSpeedBuffer()
{
    if (keypadSpeedLen_ == 0) {
        printf("KEYPAD speed buffer is empty\n");
        return;
    }

    keypadSpeedBuf_[keypadSpeedLen_] = '\0';
    setMotorRawCommand(atoi(keypadSpeedBuf_));
    printf("KEYPAD speed apply: %d\n", motorRawPercent_);
}

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

void AppController::keypadEventThunk(char key, void* context)
{
    AppController* self = static_cast<AppController*>(context);
    if (self != nullptr) {
        self->handleKeypadKey(key);
    }
}

void AppController::serialEventThunk(const SerialCommandInput::Command& command, void* context)
{
    AppController* self = static_cast<AppController*>(context);
    if (self != nullptr) {
        self->handleSerialCommand(command);
    }
}
