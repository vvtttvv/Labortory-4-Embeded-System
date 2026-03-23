#include "MotorL298.h"

#include "SignalConditioning.h"

void MotorL298::init(uint8_t in1Pin, uint8_t in2Pin, uint8_t enPinPwm)
{
    in1Pin_ = in1Pin;
    in2Pin_ = in2Pin;
    enPinPwm_ = enPinPwm;

    pinMode(in1Pin_, OUTPUT);
    pinMode(in2Pin_, OUTPUT);
    pinMode(enPinPwm_, OUTPUT);

    direction_ = Forward;
    targetPercent_ = 0;
    appliedPercent_ = 0;
    applyOutput();
}

void MotorL298::setDirection(Direction dir)
{
    direction_ = dir;
    applyOutput();
}

void MotorL298::setTargetPercent(int percent)
{
    targetPercent_ = SignalConditioning::clampInt(percent, 0, 100);
}

void MotorL298::updateRamp(int maxDeltaPercentPerStep)
{
    appliedPercent_ = SignalConditioning::rampToward(appliedPercent_, targetPercent_, maxDeltaPercentPerStep);
    applyOutput();
}

MotorL298::Direction MotorL298::getDirection() const
{
    return direction_;
}

int MotorL298::getTargetPercent() const
{
    return targetPercent_;
}

int MotorL298::getAppliedPercent() const
{
    return appliedPercent_;
}

void MotorL298::applyOutput()
{
    if (appliedPercent_ <= 0) {
        digitalWrite(in1Pin_, LOW);
        digitalWrite(in2Pin_, LOW);
        analogWrite(enPinPwm_, 0);
        return;
    }

    if (direction_ == Forward) {
        digitalWrite(in1Pin_, HIGH);
        digitalWrite(in2Pin_, LOW);
    } else {
        digitalWrite(in1Pin_, LOW);
        digitalWrite(in2Pin_, HIGH);
    }

    const int pwm = map(appliedPercent_, 0, 100, 0, 255);
    analogWrite(enPinPwm_, pwm);
}
