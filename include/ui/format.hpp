#pragma once

#include <cstdint>
#include <fmt/format.h>
#include <string>
#include <vector>
#include <algorithm>

namespace htop_killer::ui {

inline std::string format_bytes(double bytes) {
    if (bytes < 1024.0)             return fmt::format("{:.0f} B",   bytes);
    if (bytes < 1024.0 * 1024.0)   return fmt::format("{:.1f} KiB", bytes / 1024.0);
    if (bytes < 1024.0 * 1048576.0) return fmt::format("{:.1f} MiB", bytes / 1048576.0);
    return                                 fmt::format("{:.2f} GiB", bytes / 1073741824.0);
}

inline std::string format_bytes_per_sec(double bps) {
    return format_bytes(bps) + "/s";
}

inline std::string format_kib(std::uint64_t kib) {
    return format_bytes(static_cast<double>(kib) * 1024.0);
}

inline std::string format_uptime(std::uint64_t sec) {
    const auto d = sec / 86400;
    const auto h = (sec % 86400) / 3600;
    const auto m = (sec % 3600) / 60;
    const auto s = sec % 60;
    if (d > 0) return fmt::format("{}d {:02}h {:02}m", d, h, m);
    if (h > 0) return fmt::format("{:02}h {:02}m {:02}s", h, m, s);
    return fmt::format("{:02}m {:02}s", m, s);
}

} // namespace htop_killer::ui
