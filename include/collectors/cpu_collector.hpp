#pragma once

#include "core/types.hpp"

namespace htop_killer::collectors {

class ICpuCollector {
public:
    virtual ~ICpuCollector() = default;
    [[nodiscard]] virtual core::CpuStats collect() = 0;
};

} // namespace htop_killer::collectors
