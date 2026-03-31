#pragma once

#include <Arduino.h>
#include <stddef.h>

class SerialCommandInput {
public:
    enum CommandType {
        Help,
        Status,
        BinOn,
        BinOff,
        MotPos,
        MotSpd,
        MotDirFwd,
        MotDirRev,
        Unknown,
    };

    struct Command {
        CommandType type;
        int value;
    };

    using CommandHandler = void (*)(const Command& command, void* context);

    void init(CommandHandler handler, void* context);
    void poll();

private:
    static void toUpperInPlace(char* s);
    Command parseCommand(char* line) const;

    CommandHandler handler_ = nullptr;
    void* handlerContext_ = nullptr;

    char lineBuf_[64] = {0};
    size_t lineLen_ = 0;
};
