#pragma once

#include <string>

class BlockSource;
class BlockPos;

namespace lfp {
std::pair<std::string, int>
genTickingInfo(::BlockSource const& region, BlockPos const& bpos, int range);

class TickingCommand {
public:
    static void setup();
};

} // namespace lfp