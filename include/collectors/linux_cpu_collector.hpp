#pragma once

#include "collectors/cpu_collector.hpp"
#include "core/types.hpp"

#include <string>
#include <vector>

namespace htop_killer::collectors {

class LinuxCpuCollector final : public ICpuCollector {
public:
    LinuxCpuCollector();
    [[nodiscard]] core::CpuStats collect() override;

private:
    struct RawLine {
        std::uint32_t     id    = 0;
        bool              total = false;
        core::CpuSnapshot snap{};
    };

    [[nodiscard]] std::vector<RawLine>    read_proc_stat();
    [[nodiscard]] double                  compute_usage(const core::CpuSnapshot& prev,
                                                        const core::CpuSnapshot& curr) noexcept;
    [[nodiscard]] std::string             read_model_name();
    [[nodiscard]] std::array<double, 3>   read_loadavg();

    std::vector<RawLine> prev_lines_;
    std::string          model_name_;
};

} // namespace htop_killer::collectors
