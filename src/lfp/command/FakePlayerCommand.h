#pragma once

#include <string>

#include <ll/api/command/Command.h>
#include <ll/api/command/CommandHandle.h>
#include <ll/api/command/CommandRegistrar.h>

#include "lfp/Config.h"

class SimulatedPlayer;

namespace lfp {

class FakePlayerCommand : public Command {
    static std::vector<std::string> mList;
    static std::string_view const   mListSoftEnumName;

public:
    static void initSoftEnum();
    static void setup(LeviFakePlayerConfig::CommandConfig const& config);
};

} // namespace lfp