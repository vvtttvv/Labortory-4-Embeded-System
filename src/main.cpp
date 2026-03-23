
#include <Arduino.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ActuatorBinary.h"
#include "MotorL298.h"
#include "SignalConditioning.h"
#include "UartStdio.h"

namespace {
constexpr unsigned long kControlPeriodMs = 20;
constexpr unsigned long kDebounceMs = 60;
constexpr unsigned long kReportPeriodMs = 500;
constexpr int kMotorRampStepPercent = 2;

constexpr uint8_t kBinaryActuatorPin = 13;
constexpr uint8_t kMotorIn1Pin = 4;
constexpr uint8_t kMotorIn2Pin = 3;
constexpr uint8_t kMotorEnPin = 5;

ActuatorBinary g_binaryActuator;
MotorL298 g_motor;

unsigned long g_lastControlMs = 0;
unsigned long g_lastReportMs = 0;

char g_lineBuf[64];
size_t g_lineLen = 0;

void toUpperInPlace(char* s)
{
	while (*s != '\0') {
		*s = (char)toupper((unsigned char)*s);
		++s;
	}
}

void printHelp()
{
	printf("Commands:\n");
	printf("  HELP\n");
	printf("  STATUS\n");
	printf("  BIN ON|OFF\n");
	printf("  MOT SPD <0..100>\n");
	printf("  MOT DIR FWD|REV\n");
}

void printStatus()
{
	printf(
		"STAT BIN=%s PENDING=%s MOT_DIR=%s MOT_TGT=%d MOT_APPLIED=%d\n",
		g_binaryActuator.getState() ? "ON" : "OFF",
		g_binaryActuator.getPendingState() ? "ON" : "OFF",
		g_motor.getDirection() == MotorL298::Forward ? "FWD" : "REV",
		g_motor.getTargetPercent(),
		g_motor.getAppliedPercent());
}

void handleCommand(char* line)
{
	char* tokens[4] = {nullptr, nullptr, nullptr, nullptr};
	size_t tokenCount = 0;

	char* tok = strtok(line, " \t");
	while (tok != nullptr && tokenCount < 4) {
		toUpperInPlace(tok);
		tokens[tokenCount++] = tok;
		tok = strtok(nullptr, " \t");
	}

	if (tokenCount == 0) {
		return;
	}

	if (strcmp(tokens[0], "HELP") == 0) {
		printHelp();
		return;
	}

	if (strcmp(tokens[0], "STATUS") == 0) {
		printStatus();
		return;
	}

	if (strcmp(tokens[0], "BIN") == 0 && tokenCount >= 2) {
		if (strcmp(tokens[1], "ON") == 0) {
			g_binaryActuator.requestState(true, millis());
			printf("ACK BIN ON\n");
			return;
		}
		if (strcmp(tokens[1], "OFF") == 0) {
			g_binaryActuator.requestState(false, millis());
			printf("ACK BIN OFF\n");
			return;
		}
	}

	if (strcmp(tokens[0], "MOT") == 0 && tokenCount >= 3) {
		if (strcmp(tokens[1], "SPD") == 0) {
			int spd = atoi(tokens[2]);
			spd = SignalConditioning::clampInt(spd, 0, 100);
			g_motor.setTargetPercent(spd);
			printf("ACK MOT SPD %d\n", spd);
			return;
		}

		if (strcmp(tokens[1], "DIR") == 0) {
			if (strcmp(tokens[2], "FWD") == 0) {
				g_motor.setDirection(MotorL298::Forward);
				printf("ACK MOT DIR FWD\n");
				return;
			}
			if (strcmp(tokens[2], "REV") == 0) {
				g_motor.setDirection(MotorL298::Reverse);
				printf("ACK MOT DIR REV\n");
				return;
			}
		}
	}

	printf("ERR Unknown command. Type HELP\n");
}

void pollCommands()
{
	while (Serial.available() > 0) {
		const char c = (char)Serial.read();
		if (c == '\r') {
			continue;
		}

		if (c == '\n') {
			g_lineBuf[g_lineLen] = '\0';
			handleCommand(g_lineBuf);
			g_lineLen = 0;
			continue;
		}

		if (g_lineLen + 1 < sizeof(g_lineBuf)) {
			g_lineBuf[g_lineLen++] = c;
		}
	}
}
} // namespace

void setup()
{
	UartStdio::init(9600);

	g_binaryActuator.init(kBinaryActuatorPin, true, false);
	g_motor.init(kMotorIn1Pin, kMotorIn2Pin, kMotorEnPin);

	printHelp();
	printStatus();
}

void loop()
{
	pollCommands();

	const unsigned long now = millis();
	if (now - g_lastControlMs >= kControlPeriodMs) {
		g_lastControlMs = now;

		g_binaryActuator.update(now, kDebounceMs);
		g_motor.updateRamp(kMotorRampStepPercent);
	}

	if (now - g_lastReportMs >= kReportPeriodMs) {
		g_lastReportMs = now;
		printStatus();
	}
}