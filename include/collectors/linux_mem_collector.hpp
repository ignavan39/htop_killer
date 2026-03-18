#pragma once

#include "core/types.hpp"

namespace htop_killer::collectors {

class LinuxMemCollector {
public:
    [[nodiscard]] core::MemStats collect();
};

} // namespace htop_killer::collectors
