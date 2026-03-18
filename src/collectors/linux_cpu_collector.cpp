#include "collectors/linux_cpu_collector.hpp"

#include <charconv>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace htop_killer::collectors {

LinuxCpuCollector::LinuxCpuCollector() {
    model_name_ = read_model_name();
    prev_lines_ = read_proc_stat();
}

core::CpuStats LinuxCpuCollector::collect() {
    auto curr_lines = read_proc_stat();
    auto loadavg    = read_loadavg();

    core::CpuStats stats;
    stats.model_name  = model_name_;
    stats.timestamp   = core::Clock::now();
    stats.load_avg_1  = loadavg[0];
    stats.load_avg_5  = loadavg[1];
    stats.load_avg_15 = loadavg[2];

    for (std::size_t i = 0; i < curr_lines.size(); ++i) {
        const auto& cur  = curr_lines[i];
        const auto* prev = (i < prev_lines_.size()) ? &prev_lines_[i] : nullptr;
        const double usage = prev ? compute_usage(prev->snap, cur.snap) : 0.0;

        if (cur.total) {
            stats.total_usage = usage;
        } else {
            stats.cores.push_back({cur.id, usage, 0.0});
        }
    }

    stats.core_count = static_cast<std::uint32_t>(stats.cores.size());
    prev_lines_      = std::move(curr_lines);
    return stats;
}

std::vector<LinuxCpuCollector::RawLine> LinuxCpuCollector::read_proc_stat() {
    std::ifstream file{"/proc/stat"};
    if (!file.is_open())
        throw std::runtime_error{"Cannot open /proc/stat"};

    std::vector<RawLine> lines;
    std::string          line;

    while (std::getline(file, line)) {
        if (!line.starts_with("cpu")) break;

        RawLine row;
        std::istringstream ss{line};
        std::string label;
        ss >> label;

        if (label == "cpu") {
            row.total = true;
        } else {
            std::from_chars(label.data() + 3, label.data() + label.size(), row.id);
        }

        ss >> row.snap.user >> row.snap.nice >> row.snap.system >> row.snap.idle
           >> row.snap.iowait >> row.snap.irq >> row.snap.softirq >> row.snap.steal;

        lines.push_back(row);
    }

    return lines;
}

double LinuxCpuCollector::compute_usage(const core::CpuSnapshot& prev,
                                        const core::CpuSnapshot& curr) noexcept {
    const auto idle_prev = prev.idle + prev.iowait;
    const auto idle_curr = curr.idle + curr.iowait;
    const auto busy_prev = prev.user + prev.nice + prev.system + prev.irq + prev.softirq + prev.steal;
    const auto busy_curr = curr.user + curr.nice + curr.system + curr.irq + curr.softirq + curr.steal;

    const double total = static_cast<double>((idle_curr + busy_curr) - (idle_prev + busy_prev));
    const double idle  = static_cast<double>(idle_curr - idle_prev);

    if (total <= 0.0) return 0.0;
    return (1.0 - idle / total) * 100.0;
}

std::string LinuxCpuCollector::read_model_name() {
    std::ifstream file{"/proc/cpuinfo"};
    std::string   line;

    while (std::getline(file, line)) {
        if (line.starts_with("model name")) {
            const auto pos = line.find(':');
            if (pos != std::string::npos)
                return line.substr(pos + 2);
        }
    }
    return "Unknown CPU";
}

std::array<double, 3> LinuxCpuCollector::read_loadavg() {
    std::ifstream         file{"/proc/loadavg"};
    std::array<double, 3> avg{};
    if (file.is_open())
        file >> avg[0] >> avg[1] >> avg[2];
    return avg;
}

} // namespace htop_killer::collectors
