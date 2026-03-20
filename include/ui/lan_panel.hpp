#pragma once

#include "core/types.hpp"
#include "ui/format.hpp"
#include "ui/graph_widget.hpp"
#include "ui/theme.hpp"

#include <fmt/format.h>
#include <ftxui/dom/canvas.hpp>
#include <ftxui/dom/elements.hpp>

namespace htop_killer::ui {

inline ftxui::Element make_lan_panel(const core::LanStats& lan) {
    using namespace ftxui;

    if (lan.devices.empty()) {
        return vbox(
            text(" LAN Devices") | bold | color(theme::kCpuPrimary),
            separatorEmpty(),
            text("  Scanning network…") | color(theme::kDimText),
            text("  (ARP table is empty or requires elevated privileges)") | color(theme::kDimText)
        ) | border | color(theme::kDimText);
    }

    double total_rx = 0.0, total_tx = 0.0;
    for (const auto& d : lan.devices) { total_rx += d.rx_rate; total_tx += d.tx_rate; }

    auto sniffer_status = lan.sniffer_active
        ? (text(" ● packet capture active") | ftxui::color(theme::kNetRx))
        : (text(" ○ no packet capture (run as root for traffic data)")
           | ftxui::color(theme::kMemPrimary));

    auto summary = hbox(
        text(" LAN") | bold | color(theme::kCpuPrimary),
        text(fmt::format("  {} devices", lan.devices.size())) | color(theme::kDimText),
        text(fmt::format("   local {}", lan.local_ip))   | color(theme::kCpuSecondary),
        text(fmt::format("   gw {}", lan.gateway_ip))    | color(theme::kMemPrimary),
        filler(),
        text(fmt::format("total ▲ {}  ▼ {}",
             format_bytes_per_sec(total_rx),
             format_bytes_per_sec(total_tx))) | color(theme::kDimText)
    );

    auto header = hbox(
        text(fmt::format("{:<16}", "IP Address"))   | bold | color(theme::kDimText),
        text(fmt::format("{:<18}", "Hostname"))     | bold | color(theme::kDimText),
        text(fmt::format("{:<20}", "MAC Address"))  | bold | color(theme::kDimText),
        text(fmt::format("{:<16}", "Vendor"))       | bold | color(theme::kDimText),
        text(fmt::format("{:>12}", "▲ RX/s"))       | bold | color(theme::kNetRx),
        text(fmt::format("{:>12}", "▼ TX/s"))       | bold | color(theme::kNetTx),
        text(fmt::format("{:>8}",  "Conns"))        | bold | color(theme::kDimText),
        text(fmt::format("{:>8}",  "Flags"))        | bold | color(theme::kDimText)
    );

    Elements rows;
    rows.push_back(hbox(summary, filler(), sniffer_status));
    rows.push_back(separator() | color(theme::kDimText));
    rows.push_back(header);
    rows.push_back(separator() | color(theme::kDimText));

    for (const auto& dev : lan.devices) {
        ftxui::Color ip_col = theme::kBrightText;
        if (dev.is_self)    ip_col = theme::kNetRx;
        if (dev.is_gateway) ip_col = theme::kMemPrimary;

        Elements badges;
        if (dev.is_self) {
            badges.push_back(text(" self") | bold
                             | color(Color::Black)
                             | bgcolor(theme::kNetRx));
        }
        if (dev.is_gateway) {
            badges.push_back(text(" gw") | bold
                             | color(Color::Black)
                             | bgcolor(theme::kMemPrimary));
        }
        if (dev.open_ports > 10) {
            badges.push_back(text(fmt::format(" ◉{}", dev.open_ports))
                             | color(theme::kCpuSecondary));
        }
        if (badges.empty()) badges.push_back(text("        "));

        const bool has_traffic = dev.rx_rate > 0.0 || dev.tx_rate > 0.0;
        const std::string activity = has_traffic ? "●" : "○";
        const ftxui::Color act_col = has_traffic ? theme::kNetRx : theme::kDimText;

        const std::string display_host = dev.hostname.empty()
            ? std::string(16, ' ')
            : dev.hostname.substr(0, 17);

        const std::string display_vendor = dev.vendor == "Unknown"
            ? std::string{}
            : dev.vendor.substr(0, 15);

        rows.push_back(hbox(
            text(activity) | color(act_col),
            text(fmt::format("{:<15}", dev.ip))                          | bold | color(ip_col),
            text(fmt::format("{:<18}", display_host))                    | color(theme::kCpuSecondary),
            text(fmt::format("{:<20}", dev.mac))                         | color(theme::kDimText),
            text(fmt::format("{:<16}", display_vendor))                  | color(theme::kDimText),
            text(fmt::format("{:>12}", format_bytes_per_sec(dev.rx_rate))) | color(theme::kNetRx),
            text(fmt::format("{:>12}", format_bytes_per_sec(dev.tx_rate))) | color(theme::kNetTx),
            text(fmt::format("{:>8}", dev.open_ports > 0
                             ? std::to_string(dev.open_ports) : "-"))    | color(theme::kDimText),
            hbox(std::move(badges))
        ));
    }

    // TODO max rate for normalisation
    double max_rate = 0.001;
    for (const auto& d : lan.devices)
        max_rate = std::max(max_rate, std::max(d.rx_rate, d.tx_rate));

    rows.push_back(separator() | color(theme::kDimText));
    rows.push_back(text(" traffic bars") | color(theme::kDimText));

    for (const auto& dev : lan.devices) {
        if (dev.rx_rate < 1.0 && dev.tx_rate < 1.0) continue;

        const int bar_w   = 30;
        const int rx_fill = static_cast<int>(dev.rx_rate / max_rate * bar_w);
        const int tx_fill = static_cast<int>(dev.tx_rate / max_rate * bar_w);

        rows.push_back(hbox(
            text(fmt::format("{:<16}", dev.ip)) | color(theme::kDimText),
            text("▲") | color(theme::kNetRx),
            text(repeat("█", rx_fill) + repeat("░", bar_w - rx_fill)) | color(theme::kNetRx),
            text(" "),
            text("▼") | color(theme::kNetTx),
            text(repeat("█", tx_fill) + repeat("░", bar_w - tx_fill)) | color(theme::kNetTx)
        ));
    }

    return vbox(std::move(rows)) | border | color(theme::kCpuPrimary);
}

} // namespace htop_killer::ui
