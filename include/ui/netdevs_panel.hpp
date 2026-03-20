#pragma once

#include "core/network_device.hpp"
#include "ui/format.hpp"
#include "ui/graph_widget.hpp"
#include "ui/theme.hpp"

#include <fmt/format.h>
#include <ftxui/dom/elements.hpp>
#include <chrono>

namespace htop_killer::ui {

inline ftxui::Element make_netdevs_panel(const core::NetworkDeviceList& devs) {
    using namespace ftxui;

    auto last_scan_str = [&] {
        if (devs.last_scan == std::chrono::steady_clock::time_point{}) return std::string{"never"};
        const auto age = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - devs.last_scan).count();
        return fmt::format("{}s ago", age);
    }();

    auto info_bar = hbox(
        text(" subnet ") | color(theme::kDimText),
        text(devs.subnet.empty() ? "?" : devs.subnet) | bold | color(theme::kCpuPrimary),
        text("   local ") | color(theme::kDimText),
        text(devs.local_ip.empty() ? "?" : devs.local_ip) | bold,
        text("   gateway ") | color(theme::kDimText),
        text(devs.gateway_ip.empty() ? "?" : devs.gateway_ip) | bold | color(theme::kNetRx),
        text("   devices ") | color(theme::kDimText),
        text(fmt::format("{}", devs.devices.size())) | bold | color(theme::kMemPrimary),
        text("   scanned ") | color(theme::kDimText),
        text(last_scan_str) | color(theme::kDimText),
        devs.scanning ? text("  ◌ scanning...") | color(theme::kCpuSecondary) | bold
                      : text("  ✓ idle")         | color(theme::kNetRx),
        filler()
    ) | bgcolor(theme::kHeaderBg);

    auto col = [](std::string_view s, int w, ftxui::Color c = theme::kDimText) {
        return text(fmt::format("{:<{}}", s, w)) | color(c) | bold;
    };
    auto colr = [](std::string_view s, int w, ftxui::Color c = theme::kDimText) {
        return text(fmt::format("{:>{}}", s, w)) | color(c) | bold;
    };

    auto tbl_header = hbox(
        col("",         2),
        col("IP",      16),
        col("Hostname", 18),
        col("MAC",      20),
        col("Vendor",   14),
        colr("▲ RX/s",  12, theme::kNetRx),
        colr("▼ TX/s",  12, theme::kNetTx),
        colr("RX total", 12, theme::kDimText),
        colr("TX total", 12, theme::kDimText),
        col("Iface",    8)
    );

    Elements rows;
    rows.push_back(info_bar);
    rows.push_back(separator() | color(theme::kDimText));
    rows.push_back(tbl_header);
    rows.push_back(separator() | color(theme::kDimText));

    if (devs.devices.empty()) {
        rows.push_back(
            text("  No devices found. ARP table empty or scanning...") | color(theme::kDimText)
        );
    }

    //TODO: find better indecators
    for (const auto& dev : devs.devices) {
        std::string icon  = dev.is_gateway ? "◈ " : "● ";
        ftxui::Color icon_col = dev.is_gateway ? theme::kNetRx
                                                : theme::kCpuSecondary;

        const std::string display_host = dev.hostname.empty()
                                         ? std::string(16, ' ')
                                         : dev.hostname.substr(0, 16);

        // rx_pct: scale in MiB/s = 100%
        const double rx_pct = std::min(dev.rx_rate / 1e6 / 10.0 * 100.0, 100.0);
        const double tx_pct = std::min(dev.tx_rate / 1e6 / 10.0 * 100.0, 100.0);

        const auto age_sec = std::chrono::duration_cast<std::chrono::seconds>(
            devs.last_scan - dev.first_seen).count();
        (void)age_sec;

        rows.push_back(hbox(
            text(icon) | color(icon_col),
            text(fmt::format("{:<16}", dev.ip))
                | color(dev.is_gateway ? theme::kNetRx : theme::kBrightText),
            text(fmt::format("{:<18}", display_host))
                | color(dev.hostname.empty() ? theme::kDimText : theme::kCpuSecondary),
            text(fmt::format("{:<20}", dev.mac)) | color(theme::kDimText),
            text(fmt::format("{:<14}", dev.vendor.empty() ? "-" : dev.vendor))
                | color(theme::kMemPrimary),
            text(fmt::format("{:>12}", format_bytes_per_sec(dev.rx_rate)))
                | color(theme::kNetRx),
            text(fmt::format("{:>12}", format_bytes_per_sec(dev.tx_rate)))
                | color(theme::kNetTx),
            text(fmt::format("{:>12}", format_bytes(static_cast<double>(dev.rx_bytes))))
                | color(theme::kDimText),
            text(fmt::format("{:>12}", format_bytes(static_cast<double>(dev.tx_bytes))))
                | color(theme::kDimText),
            text(fmt::format("{:<8}", dev.iface)) | color(theme::kDimText)
        ));

        if (dev.rx_rate > 0.0 || dev.tx_rate > 0.0) {
            const int rx_filled = static_cast<int>(rx_pct / 100.0 * 20);
            const int tx_filled = static_cast<int>(tx_pct / 100.0 * 20);
            rows.push_back(hbox(
                text("  ") ,
                text("▲ ") | color(theme::kNetRx),
                text(repeat("█", rx_filled) + repeat("░", 20 - rx_filled))
                    | color(theme::kNetRx),
                text("  ▼ ") | color(theme::kNetTx),
                text(repeat("█", tx_filled) + repeat("░", 20 - tx_filled))
                    | color(theme::kNetTx)
            ));
        }
    }

    rows.push_back(separator() | color(theme::kDimText));
    rows.push_back(hbox(
        text("  ARP table from /proc/net/arp") | color(theme::kDimText),
        text("   traffic from /proc/net/nf_conntrack") | color(theme::kDimText),
        text(fmt::format("   rescan every {}s", 15)) | color(theme::kDimText)
    ));

    return vbox(std::move(rows)) | border | color(theme::kCpuPrimary);
}

} // namespace htop_killer::ui
