#pragma once

#include "core/history.hpp"
#include "core/types.hpp"
#include "ui/format.hpp"
#include "ui/graph_widget.hpp"
#include "ui/theme.hpp"

#include <fmt/format.h>
#include <ftxui/dom/elements.hpp>

namespace htop_killer::ui {

inline ftxui::Element make_mem_panel(const core::MemStats& mem,
                                     const core::History&  hist)
{
    using namespace ftxui;

    auto labeled_gauge = [](std::string_view lbl, double pct, double used_gb,
                             double total_gb, ftxui::Color col) -> Element {
        return hbox(
            text(fmt::format("{:<5}", lbl)) | ftxui::color(theme::kDimText),
            gradient_gauge(pct, 28),
            text(fmt::format(" {:5.1f}%", pct)) | bold | ftxui::color(col),
            text(fmt::format("  {:.2f}/{:.2f} GiB", used_gb, total_gb))
                | ftxui::color(theme::kDimText)
        );
    };

    const double mem_used_gib  = static_cast<double>(mem.used_kb)   / (1024.0 * 1024.0);
    const double mem_total_gib = static_cast<double>(mem.total_kb)  / (1024.0 * 1024.0);
    const double swap_used_gib = static_cast<double>(mem.swap_used) / (1024.0 * 1024.0);
    const double swap_tot_gib  = static_cast<double>(mem.swap_total)/ (1024.0 * 1024.0);

    auto gauges = vbox(
        labeled_gauge("MEM",  mem.usage_pct, mem_used_gib,  mem_total_gib, theme::kMemPrimary),
        labeled_gauge("SWAP", mem.swap_pct,  swap_used_gib, swap_tot_gib,  theme::kMemSecondary)
    ) | border | ftxui::color(theme::kMemPrimary);

    auto mem_graph  = sparkgraph(hist.mem_used_pct,  "Memory", "%", 100.0, theme::kMemPrimary,  6);
    auto swap_graph = sparkgraph(hist.swap_used_pct, "Swap",   "%", 100.0, theme::kMemSecondary, 4);

    const double cached_pct  = mem.total_kb > 0 ? static_cast<double>(mem.cached_kb)  / mem.total_kb * 100.0 : 0.0;
    const double buffers_pct = mem.total_kb > 0 ? static_cast<double>(mem.buffers_kb) / mem.total_kb * 100.0 : 0.0;
    const double free_pct    = mem.total_kb > 0 ? static_cast<double>(mem.free_kb)    / mem.total_kb * 100.0 : 0.0;

    auto breakdown = vbox(
        text(" breakdown") | ftxui::color(theme::kDimText),
        hbox(text("  used    ") | ftxui::color(theme::kMemPrimary),
             gradient_gauge(mem.usage_pct, 20),
             text(fmt::format(" {:.1f}%", mem.usage_pct)) | ftxui::color(theme::kMemPrimary)),
        hbox(text("  cached  ") | ftxui::color(theme::kCpuSecondary),
             gradient_gauge(cached_pct, 20),
             text(fmt::format(" {:.1f}%", cached_pct)) | ftxui::color(theme::kCpuSecondary)),
        hbox(text("  buffers ") | ftxui::color(theme::kDiskRead),
             gradient_gauge(buffers_pct, 20),
             text(fmt::format(" {:.1f}%", buffers_pct)) | ftxui::color(theme::kDiskRead)),
        hbox(text("  free    ") | ftxui::color(theme::kNetRx),
             gradient_gauge(free_pct, 20),
             text(fmt::format(" {:.1f}%", free_pct)) | ftxui::color(theme::kNetRx))
    ) | border | ftxui::color(theme::kDimText);

    return vbox(
        gauges,
        hbox(mem_graph | flex, swap_graph | flex),
        breakdown
    );
}

} // namespace htop_killer::ui
