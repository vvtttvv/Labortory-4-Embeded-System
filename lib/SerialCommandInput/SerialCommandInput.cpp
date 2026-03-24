#include "SerialCommandInput.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

void SerialCommandInput::init(CommandHandler handler, void* context)
{
    handler_ = handler;
    handlerContext_ = context;
    lineLen_ = 0;
    lineBuf_[0] = '\0';
}

void SerialCommandInput::poll()
{
    while (Serial.available() > 0) {
        const char c = (char)Serial.read();
        if (c == '\r') {
            continue;
        }

        if (c == '\n') {
            lineBuf_[lineLen_] = '\0';
            Command command = parseCommand(lineBuf_);
            if (handler_ != nullptr) {
                handler_(command, handlerContext_);
            }
            lineLen_ = 0;
            continue;
        }

        if (lineLen_ + 1 < sizeof(lineBuf_)) {
            lineBuf_[lineLen_++] = c;
        }
    }
}

void SerialCommandInput::toUpperInPlace(char* s)
{
    while (*s != '\0') {
        *s = (char)toupper((unsigned char)*s);
        ++s;
    }
}

SerialCommandInput::Command SerialCommandInput::parseCommand(char* line) const
{
    char* tokens[4] = {nullptr, nullptr, nullptr, nullptr};
    size_t tokenCount = 0;

    char* tok = strtok(line, " \t");
    while (tok != nullptr && tokenCount < 4) {
        toUpperInPlace(tok);
        tokens[tokenCount++] = tok;
        tok = strtok(nullptr, " \t");
    }

    Command command = {Unknown, 0};

    if (tokenCount == 0) {
        return command;
    }

    if (strcmp(tokens[0], "HELP") == 0) {
        command.type = Help;
        return command;
    }

    if (strcmp(tokens[0], "STATUS") == 0) {
        command.type = Status;
        return command;
    }

    if (strcmp(tokens[0], "BIN") == 0 && tokenCount >= 2) {
        if (strcmp(tokens[1], "ON") == 0) {
            command.type = BinOn;
            return command;
        }
        if (strcmp(tokens[1], "OFF") == 0) {
            command.type = BinOff;
            return command;
        }
    }

    if (strcmp(tokens[0], "MOT") == 0 && tokenCount >= 3) {
        if (strcmp(tokens[1], "SPD") == 0) {
            command.type = MotSpd;
            command.value = atoi(tokens[2]);
            return command;
        }

        if (strcmp(tokens[1], "DIR") == 0) {
            if (strcmp(tokens[2], "FWD") == 0) {
                command.type = MotDirFwd;
                return command;
            }
            if (strcmp(tokens[2], "REV") == 0) {
                command.type = MotDirRev;
                return command;
            }
        }
    }

    return command;
}
