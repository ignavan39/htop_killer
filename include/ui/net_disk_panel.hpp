#pragma once

#include "core/history.hpp"
#include "core/types.hpp"
#include "ui/format.hpp"
#include "ui/graph_widget.hpp"
#include "ui/theme.hpp"

#include <fmt/format.h>
#include <ftxui/dom/elements.hpp>

namespace htop_killer::ui {

inline ftxui::Element make_net_panel(const core::NetStats& net,
                                     const core::History&  hist)
{
    using namespace ftxui;

    auto g = dual_sparkgraph(
        hist.net_rx_mbps, hist.net_tx_mbps,
        "Network I/O", "RX", "TX", " MiB/s", 0.0,
        theme::kNetRx, theme::kNetTx, 10);

    Elements rows;
    rows.push_back(hbox(
        text(fmt::format("{:<14}", "Interface")) | bold | color(theme::kDimText),
        text(fmt::format("{:>16}", "▲ Receive/s"))   | bold | color(theme::kNetRx),
        text(fmt::format("{:>16}", "▼ Transmit/s"))  | bold | color(theme::kNetTx),
        text(fmt::format("{:>12}", "RX total"))      | color(theme::kDimText),
        text(fmt::format("{:>12}", "TX total"))      | color(theme::kDimText)
    ));
    rows.push_back(separator() | color(theme::kDimText));

    for (const auto& ifc : net.interfaces) {
        rows.push_back(hbox(
            text(fmt::format("{:<14}", ifc.name)) | bold,
            text(fmt::format("{:>16}", format_bytes_per_sec(ifc.rx_rate))) | color(theme::kNetRx),
            text(fmt::format("{:>16}", format_bytes_per_sec(ifc.tx_rate))) | color(theme::kNetTx),
            text(fmt::format("{:>12}", format_bytes(static_cast<double>(ifc.rx_bytes)))) | color(theme::kDimText),
            text(fmt::format("{:>12}", format_bytes(static_cast<double>(ifc.tx_bytes)))) | color(theme::kDimText)
        ));
    }

    return vbox(g, vbox(std::move(rows)) | border);
}

inline ftxui::Element make_disk_panel(const core::DiskStats& disk,
                                      const core::History&   hist)
{
    using namespace ftxui;

    auto g = dual_sparkgraph(
        hist.disk_read_mbps, hist.disk_write_mbps,
        "Disk I/O", "Read", "Write", " MiB/s", 0.0,
        theme::kDiskRead, theme::kDiskWrite, 10);

    Elements rows;
    rows.push_back(hbox(
        text(fmt::format("{:<14}", "Device"))    | bold | color(theme::kDimText),
        text(fmt::format("{:>16}", "▲ Read/s"))  | bold | color(theme::kDiskRead),
        text(fmt::format("{:>16}", "▼ Write/s")) | bold | color(theme::kDiskWrite),
        text(fmt::format("{:>10}", "Util%"))     | bold
    ));
    rows.push_back(separator() | color(theme::kDimText));

    for (const auto& dev : disk.devices) {
        const int filled = static_cast<int>(dev.util_pct / 100.0 * 8);
        const std::string util_bar = repeat("█", filled) + repeat("░", 8 - filled);

        rows.push_back(hbox(
            text(fmt::format("{:<14}", dev.name)) | bold,
            text(fmt::format("{:>16}", format_bytes_per_sec(dev.read_rate)))  | color(theme::kDiskRead),
            text(fmt::format("{:>16}", format_bytes_per_sec(dev.write_rate))) | color(theme::kDiskWrite),
            text(util_bar) | color(theme::usage_color(dev.util_pct)),
            text(fmt::format(" {:4.1f}%", dev.util_pct)) | color(theme::usage_color(dev.util_pct))
        ));
    }

    return vbox(g, vbox(std::move(rows)) | border);
}

} // namespace htop_killer::ui
