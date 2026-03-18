#pragma once

#include "core/types.hpp"
#include "ui/format.hpp"
#include "ui/graph_widget.hpp"
#include "ui/theme.hpp"

#include <fmt/format.h>
#include <ftxui/dom/elements.hpp>
#include <algorithm>

namespace htop_killer::ui {

enum class ProcSortKey { Cpu, Mem, Pid, Name };

inline ftxui::Element make_process_panel(const core::ProcessList& procs,
                                          ProcSortKey              sort     = ProcSortKey::Cpu,
                                          int                      max_rows = 20,
                                          int                      scroll   = 0)
{
    using namespace ftxui;

    auto hdr = [&](std::string_view s, int w, bool active) -> Element {
        auto t = text(fmt::format("{:>{}}", s, w));
        return active ? t | bold | color(theme::kCpuPrimary)
                      : t | color(theme::kDimText);
    };

    auto header = hbox(
        hdr("PID",  7,  sort == ProcSortKey::Pid),  text(" "),
        hdr("USER", 10, false),                      text(" "),
        hdr("NI",   3,  false),                      text(" "),
        hdr("THR",  4,  false),                      text(" "),
        hdr("CPU%", 6,  sort == ProcSortKey::Cpu),   text(" "),
        text(fmt::format("{:<8}", "")) | color(theme::kDimText),
        hdr("MEM%", 6,  sort == ProcSortKey::Mem),   text(" "),
        text(fmt::format("{:<8}", "")) | color(theme::kDimText),
        hdr("MEM",  9,  false),                      text(" "),
        hdr("S",    1,  false),                      text(" "),
        text(fmt::format("{:<24}", "NAME"))
            | (sort == ProcSortKey::Name ? bold : nothing)
            | color(sort == ProcSortKey::Name ? theme::kCpuPrimary : theme::kDimText)
    );

    auto sorted = procs.processes;
    switch (sort) {
        case ProcSortKey::Cpu:  std::ranges::sort(sorted, std::greater{}, &core::ProcessInfo::cpu_pct);    break;
        case ProcSortKey::Mem:  std::ranges::sort(sorted, std::greater{}, &core::ProcessInfo::mem_rss_kb); break;
        case ProcSortKey::Pid:  std::ranges::sort(sorted, std::less{},    &core::ProcessInfo::pid);        break;
        case ProcSortKey::Name: std::ranges::sort(sorted, std::less{},    &core::ProcessInfo::name);       break;
    }

    auto summary = hbox(
        text(fmt::format(" {} processes", procs.total_count)) | color(theme::kDimText),
        text("  "),
        text(fmt::format("▶ {} running", procs.running_count))   | color(theme::kNetRx),
        text("  "),
        text(fmt::format("◌ {} sleeping", procs.sleeping_count)) | color(theme::kDimText)
    );

    Elements rows;
    rows.push_back(summary);
    rows.push_back(separator() | color(theme::kDimText));
    rows.push_back(header);
    rows.push_back(separator() | color(theme::kDimText));

    const int start = std::clamp(scroll, 0, std::max(0, (int)sorted.size() - max_rows));
    const int end   = std::min(start + max_rows, (int)sorted.size());

    for (int i = start; i < end; ++i) {
        const auto& p = sorted[i];

        ftxui::Color sc = theme::kDimText;
        if      (p.state == "R") sc = theme::kNetRx;
        else if (p.state == "Z") sc = theme::kNetTx;

        const int cpu_bar_w = 6;
        const int mem_bar_w = 6;
        const int cpu_filled = std::clamp(static_cast<int>(p.cpu_pct / 100.0 * cpu_bar_w), 0, cpu_bar_w);
        const int mem_filled = std::clamp(static_cast<int>(p.mem_pct / 100.0 * mem_bar_w), 0, mem_bar_w);

        const std::string cpu_bar = repeat("▪", cpu_filled) + repeat("·", cpu_bar_w - cpu_filled);
        const std::string mem_bar = repeat("▪", mem_filled) + repeat("·", mem_bar_w - mem_filled);

        rows.push_back(hbox(
            text(fmt::format("{:>7}", p.pid)),                                                         text(" "),
            text(fmt::format("{:<10}", p.user.substr(0, 10))) | color(theme::kCpuSecondary),           text(" "),
            text(fmt::format("{:>3}", p.nice)),                                                         text(" "),
            text(fmt::format("{:>4}", p.threads)),                                                      text(" "),
            text(fmt::format("{:>6.1f}", p.cpu_pct)) | bold | color(theme::usage_color(p.cpu_pct)),    text(" "),
            text(cpu_bar) | color(theme::usage_color(p.cpu_pct)),                                      text(" "),
            text(fmt::format("{:>6.1f}", p.mem_pct)) | color(theme::usage_color(p.mem_pct)),           text(" "),
            text(mem_bar) | color(theme::usage_color(p.mem_pct)),                                      text(" "),
            text(fmt::format("{:>9}", format_kib(p.mem_rss_kb))),                                      text(" "),
            text(p.state) | bold | color(sc),                                                           text(" "),
            text(p.name.substr(0, 24))
        ));
    }

    if ((int)sorted.size() > max_rows) {
        rows.push_back(separator() | color(theme::kDimText));
        rows.push_back(
            text(fmt::format("  {}-{} of {}   j/k to scroll", start + 1, end, sorted.size()))
            | color(theme::kDimText));
    }

    return vbox(std::move(rows)) | border;
}

} // namespace htop_killer::ui
