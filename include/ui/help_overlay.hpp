#pragma once

#include "ui/theme.hpp"

#include <fmt/format.h>
#include <ftxui/dom/elements.hpp>

namespace htop_killer::ui {

inline ftxui::Element make_help_overlay() {
    using namespace ftxui;

    auto row = [](std::string_view key, std::string_view desc) -> Element {
        return hbox(
            text(fmt::format("  {:<14}", key)) | bold | color(theme::kCpuPrimary),
            text(std::string(desc)) | color(theme::kDimText)
        );
    };

    return vbox(
        text("  htop_killer  keybindings") | bold | color(theme::kCpuPrimary),
        separator(),
        row("Tab",       "next tab"),
        row("1 2 3 4",   "CPU / MEM / NET / DISK"),
        row("5 / p",     "process table"),
        row("6 / l",     "LAN devices"),
        row("c",         "toggle per-core bars"),
        row("f",         "cycle sort: CPU→MEM→PID→Name"),
        row("j / ↓",     "scroll down"),
        row("k / ↑",     "scroll up"),
        row("q / Esc",   "quit"),
        row("?",         "toggle this help")
    ) | border | size(WIDTH, EQUAL, 42);
}

} // namespace htop_killer::ui
