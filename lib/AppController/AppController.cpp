#include "AppController.h"

#include <stdio.h>
#include <stdlib.h>

#include "SignalConditioning.h"
#include "UartStdio.h"

namespace {
constexpr unsigned long kDebounceMs = 60;
constexpr unsigned long kKeypadDebounceMs = 35;
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
}

void AppController::setup()
{
    UartStdio::init(9600);
    stateMutex_ = xSemaphoreCreateMutex();

    binaryActuator_.init(kBinaryActuatorPin, true, false);
    motor_.init(kMotorIn1Pin, kMotorIn2Pin, kMotorEnPin);
    serialInput_.init(serialEventThunk, this);
    keypad_.init(kKeypadRowPins, kKeypadColPins, kKeypadMap, kKeypadDebounceMs);
    keypad_.setKeyHandler(keypadEventThunk, this);

    lcd_.init(kLcdRsPin, kLcdEnPin, kLcdD4Pin, kLcdD5Pin, kLcdD6Pin, kLcdD7Pin);
    lcd_.showBoot();

    printHelp();
    printf("Scheduler: round-robin task separat\n");
    printStatus();
    updateLcdStatus();
}

void AppController::loop()
{
}

void AppController::taskCommandInput()
{
    const unsigned long nowMs = millis();
    serialInput_.poll();
    keypad_.poll(nowMs);
}

void AppController::taskBinaryControl()
{
    lockState();
    const unsigned long nowMs = millis();
    binaryActuator_.update(nowMs, kDebounceMs);
    unlockState();
}

void AppController::taskAnalogControl()
{
    lockState();
    runAnalogConditioningAndControl();
    unlockState();
}

void AppController::taskReporting()
{
    lockState();
    const StatusSnapshot snapshot = makeStatusSnapshotLocked();
    unlockState();

    printStatus(snapshot);
    updateLcdStatus(snapshot);
}

void AppController::lockState()
{
    if (stateMutex_ != nullptr) {
        xSemaphoreTake(stateMutex_, portMAX_DELAY);
    }
}

void AppController::unlockState()
{
    if (stateMutex_ != nullptr) {
        xSemaphoreGive(stateMutex_);
    }
}

AppController::StatusSnapshot AppController::makeStatusSnapshotLocked() const
{
    StatusSnapshot snapshot;
    snapshot.binaryState = binaryActuator_.getState();
    snapshot.pendingState = binaryActuator_.getPendingState();
    snapshot.raw = motorRawPercent_;
    snapshot.sat = motorSatPercent_;
    snapshot.med = motorMedianPercent_;
    snapshot.filt = motorFilteredPercent_;
    snapshot.target = motor_.getTargetPercent();
    snapshot.applied = motor_.getAppliedPercent();
    snapshot.speed = motorSpeedPercent_;
    snapshot.limitAlert = alertLimitReached_;
    return snapshot;
}

void AppController::printHelp()
{
    printf("Commands:\n");
    printf("  HELP\n");
    printf("  STATUS\n");
    printf("  BIN ON|OFF\n");
    printf("  MOT POS <0..100>\n");
    printf("  MOT SPD <1..100>\n");
    printf("Keypad:\n");
    printf("  A=BIN ON, B=BIN OFF\n");
    printf("  C=Target POS, D=Target SPD\n");
    printf("  0..9=Input, #=Apply target, *=Clear input\n");
}

void AppController::updateLcdStatus()
{
    const StatusSnapshot snapshot = makeStatusSnapshotLocked();
    updateLcdStatus(snapshot);
}

void AppController::printStatus()
{
    const StatusSnapshot snapshot = makeStatusSnapshotLocked();
    printStatus(snapshot);
}

void AppController::updateLcdStatus(const StatusSnapshot& snapshot)
{
    lcd_.showStatus(
        snapshot.binaryState,
        snapshot.raw,
        snapshot.applied,
        snapshot.speed,
        snapshot.limitAlert);
}

void AppController::printStatus(const StatusSnapshot& snapshot)
{
    printf(
    "STAT BIN=%s PENDING=%s POS_RAW=%d POS_SAT=%d POS_MED=%d POS_FILT=%d POS_TGT=%d POS_APPLIED=%d SPD=%d ALERT_LIMIT=%s\n",
        snapshot.binaryState ? "ON" : "OFF",
        snapshot.pendingState ? "ON" : "OFF",
        snapshot.raw,
        snapshot.sat,
        snapshot.med,
        snapshot.filt,
        snapshot.target,
        snapshot.applied,
        snapshot.speed,
        snapshot.limitAlert ? "YES" : "NO");
}

void AppController::setMotorRawCommand(int rawPercent)
{
    motorRawPercent_ = rawPercent;
}

void AppController::applyKeypadBuffer()
{
    if (keypadSpeedLen_ == 0) {
        printf("KEYPAD buffer is empty\n");
        return;
    }

    keypadSpeedBuf_[keypadSpeedLen_] = '\0';
    const int value = atoi(keypadSpeedBuf_);

    if (keypadTarget_ == KeypadTargetPosition) {
        setMotorRawCommand(value);
        printf("KEYPAD apply POS: %d\n", motorRawPercent_);
    } else {
        motorSpeedPercent_ = SignalConditioning::clampInt(value, 1, 100);
        printf("KEYPAD apply SPD: %d\n", motorSpeedPercent_);
    }

    keypadSpeedLen_ = 0;
    keypadSpeedBuf_[0] = '\0';
}

void AppController::handleKeypadKey(char key)
{
    lockState();

    if (key >= '0' && key <= '9') {
        if (keypadSpeedLen_ < 3) {
            keypadSpeedBuf_[keypadSpeedLen_++] = key;
            keypadSpeedBuf_[keypadSpeedLen_] = '\0';
            printf("KEYPAD buf (%s): %s\n", keypadTarget_ == KeypadTargetPosition ? "POS" : "SPD", keypadSpeedBuf_);
        }
        unlockState();
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
        keypadTarget_ = KeypadTargetPosition;
        keypadSpeedLen_ = 0;
        keypadSpeedBuf_[0] = '\0';
        printf("KEYPAD target POS\n");
        break;
    case 'D':
        keypadTarget_ = KeypadTargetSpeed;
        keypadSpeedLen_ = 0;
        keypadSpeedBuf_[0] = '\0';
        printf("KEYPAD target SPD\n");
        break;
    case '#':
        applyKeypadBuffer();
        break;
    case '*':
        keypadSpeedLen_ = 0;
        keypadSpeedBuf_[0] = '\0';
        printf("KEYPAD buffer cleared\n");
        break;
    default:
        break;
    }

    unlockState();
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
    const int speedClamped = SignalConditioning::clampInt(motorSpeedPercent_, 1, 100);
    const int dynamicRampStep = map(speedClamped, 1, 100, 1, 10);
    motor_.updateRamp(dynamicRampStep > 0 ? dynamicRampStep : kMotorRampStepPercent);
}

void AppController::handleSerialCommand(const SerialCommandInput::Command& command)
{
    lockState();

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
    case SerialCommandInput::MotPos:
        setMotorRawCommand(command.value);
        printf("ACK MOT POS RAW %d\n", motorRawPercent_);
        break;
    case SerialCommandInput::MotSpd:
        motorSpeedPercent_ = SignalConditioning::clampInt(command.value, 1, 100);
        printf("ACK MOT SPD %d\n", motorSpeedPercent_);
        break;
    case SerialCommandInput::MotDirFwd:
    case SerialCommandInput::MotDirRev:
        printf("WARN MOT DIR ignored for servo mode. Use MOT POS and MOT SPD\n");
        break;
    case SerialCommandInput::Unknown:
    default:
        printf("ERR Unknown command. Type HELP\n");
        break;
    }

    unlockState();
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
