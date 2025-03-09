#pragma once

#include <optional>
#include <string>

#include "mc/server/commands/CommandPermissionLevel.h"

#include "ll/api/io/LogLevel.h"

namespace lfp {

struct LeviFakePlayerConfig {

    int version = 1;

    std::string language = "system";

    struct CommandConfig {
        bool                   enabled              = true;
        CommandPermissionLevel permission           = CommandPermissionLevel::GameDirectors;
        std::string            alias                = "lfp";
        std::optional<bool>    enableTickingCommand = false;
    };

    struct FixConfig {
        bool fixAllSimulatedPlayer = false;
        bool dynamicUnloadFix      = false;
        int  chunkRadius           = 5;
    };

    struct {
        ll::io::LogLevel logLevel = ll::io::LogLevel::Warn;
        CommandConfig    command{};
        FixConfig        fix{};
    } config{};
};

[[nodiscard]] LeviFakePlayerConfig& getConfig();

bool saveConfig();

} // namespace lfp