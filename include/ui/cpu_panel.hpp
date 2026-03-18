#pragma once

#include "core/history.hpp"
#include "core/types.hpp"
#include "ui/graph_widget.hpp"
#include "ui/format.hpp"
#include "ui/theme.hpp"

#include <fmt/format.h>
#include <ftxui/dom/elements.hpp>

namespace htop_killer::ui {

inline ftxui::Element make_cpu_panel(const core::CpuStats& stats,
                                     const core::History&  hist,
                                     bool                  per_core = false)
{
    using namespace ftxui;

    auto total = sparkgraph(hist.cpu_total,
                            fmt::format("CPU  {} cores  load {:.2f} {:.2f} {:.2f}",
                                        stats.core_count,
                                        stats.load_avg_1,
                                        stats.load_avg_5,
                                        stats.load_avg_15),
                            "%", 100.0, theme::kCpuPrimary, 7);

    if (!per_core || stats.cores.empty())
        return total;

    std::vector<double> usages;
    const std::size_t n = std::min(stats.cores.size(), core::History::kMaxCores);
    usages.reserve(n);
    for (std::size_t i = 0; i < n; ++i)
        usages.push_back(stats.cores[i].usage);

    Elements core_graphs;
    for (std::size_t i = 0; i < n; ++i) {
        const double pct = stats.cores[i].usage;
        core_graphs.push_back(
            vbox(
                text(fmt::format("C{:<2}", i)) | ftxui::color(theme::kDimText),
                gradient_gauge(pct, 3) ,
                text(fmt::format("{:3.0f}%", pct))
                    | bold | ftxui::color(theme::usage_color(pct))
                    | size(WIDTH, EQUAL, 4)
            ) | size(WIDTH, EQUAL, 4)
        );
    }

    auto cores_row = vbox(
        text(" per-core") | ftxui::color(theme::kDimText),
        hbox(std::move(core_graphs)) | flex
    ) | border | ftxui::color(theme::kDimText);

    return vbox(total, cores_row);
}

} // namespace htop_killer::ui
