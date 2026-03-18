#pragma once

#include "core/types.hpp"
#include "ui/format.hpp"
#include "ui/graph_widget.hpp"
#include "ui/theme.hpp"

#include <fmt/format.h>
#include <ftxui/dom/elements.hpp>
#include <chrono>
#include <ctime>
#include <fstream>

namespace htop_killer::ui {

inline ftxui::Element make_header(const core::SystemSnapshot& snap) {
    using namespace ftxui;

    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    char timebuf[16], datebuf[16];
    std::strftime(timebuf, sizeof(timebuf), "%H:%M:%S", std::localtime(&t));
    std::strftime(datebuf, sizeof(datebuf), "%Y-%m-%d", std::localtime(&t));

    auto load_col = [&](double l) -> ftxui::Color {
        const double r = snap.cpu.core_count > 0 ? l / snap.cpu.core_count : 0.0;
        if (r < 0.5) return theme::kNetRx;
        if (r < 1.0) return theme::kMemPrimary;
        return theme::kNetTx;
    };

    std::string uptime_str = "-";
    {
        std::ifstream uf{"/proc/uptime"};
        double sec = 0.0;
        if (uf >> sec)
            uptime_str = format_uptime(static_cast<std::uint64_t>(sec));
    }

    const double cpu_pct = snap.cpu.total_usage;
    const double mem_pct = snap.mem.usage_pct;
    const int bar_w = 10;

    const std::string hbar = repeat("█", static_cast<int>(cpu_pct / 100.0 * bar_w))
                           + repeat("░", bar_w - static_cast<int>(cpu_pct / 100.0 * bar_w));
    const std::string mbar = repeat("█", static_cast<int>(mem_pct / 100.0 * bar_w))
                           + repeat("░", bar_w - static_cast<int>(mem_pct / 100.0 * bar_w));

    const auto& p = snap.procs;

    auto left = hbox(
        text(" ◈ ") | color(theme::kCpuSecondary),
        text("htop_killer") | bold | color(theme::kCpuPrimary),
        text("  "),
        text(snap.cpu.model_name) | color(theme::kDimText)
    );

    auto middle = hbox(
        text(" CPU ") | color(theme::kDimText),
        text(hbar) | color(theme::usage_color(cpu_pct)),
        text(fmt::format(" {:4.1f}%", cpu_pct)) | bold | color(theme::usage_color(cpu_pct)),
        text("  MEM ") | color(theme::kDimText),
        text(mbar) | color(theme::usage_color(mem_pct)),
        text(fmt::format(" {:4.1f}%", mem_pct)) | bold | color(theme::usage_color(mem_pct))
    );

    auto right = hbox(
        text("up ") | color(theme::kDimText),
        text(uptime_str) | bold,
        text("  load ") | color(theme::kDimText),
        text(fmt::format("{:.2f}", snap.cpu.load_avg_1))  | bold | color(load_col(snap.cpu.load_avg_1)),
        text(" "),
        text(fmt::format("{:.2f}", snap.cpu.load_avg_5))  | color(load_col(snap.cpu.load_avg_5)),
        text(" "),
        text(fmt::format("{:.2f}", snap.cpu.load_avg_15)) | color(load_col(snap.cpu.load_avg_15)),
        text("  "),
        text(fmt::format("▶{} ", p.running_count))  | color(theme::kNetRx),
        text(fmt::format("◌{} ", p.sleeping_count)) | color(theme::kDimText),
        text(fmt::format("Σ{}",  p.total_count))     | color(theme::kDimText),
        text("  "),
        text(datebuf) | color(theme::kDimText),
        text(" "),
        text(timebuf) | bold | color(theme::kBrightText),
        text(" ")
    );

    return hbox(left, filler(), middle, filler(), right) | bgcolor(theme::kHeaderBg);
}

} // namespace htop_killer::ui
