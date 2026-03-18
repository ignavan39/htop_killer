#pragma once

#include "core/ring_buffer.hpp"
#include "ui/theme.hpp"

#include <ftxui/dom/canvas.hpp>
#include <ftxui/dom/elements.hpp>
#include <fmt/format.h>

#include <algorithm>
#include <string>
#include <vector>

namespace htop_killer::ui {

inline std::string repeat(std::string_view s, int n) {
    std::string out;
    out.reserve(s.size() * n);
    for (int i = 0; i < n; ++i) out += s;
    return out;
}

template <std::size_t N>
ftxui::Element braille_graph(
    const core::RingBuffer<double, N>& buf,
    double                             max_val,
    ftxui::Color                       line_color,
    int                                char_height = 5)
{
    using namespace ftxui;

    std::vector<float> data;
    data.reserve(buf.size());
    for (std::size_t i = 0; i < buf.size(); ++i) {
        float v = max_val > 0.0 ? static_cast<float>(buf[i] / max_val) : 0.0f;
        data.push_back(std::clamp(v, 0.0f, 1.0f));
    }

    const int px_h = char_height * 4;

    auto fill_elem = canvas([data, px_h](Canvas& c) {
        const int px_w = c.width();
        const int n    = static_cast<int>(data.size());
        auto val_to_y  = [&](float v) {
            return px_h - 1 - static_cast<int>(v * (px_h - 1));
        };
        for (int x = 0; x < px_w; ++x) {
            int di = n - px_w + x;
            float v = (di >= 0 && di < n) ? data[di] : 0.0f;
            int y0 = val_to_y(v);
            for (int y = y0 + 1; y < px_h; ++y)
                c.DrawPointOn(x, y);
        }
    }) | size(HEIGHT, EQUAL, char_height)
       | color(Color{Color::Palette256{236}});

    auto line_elem = canvas([data, px_h](Canvas& c) {
        const int px_w = c.width();
        const int n    = static_cast<int>(data.size());
        auto val_to_y  = [&](float v) {
            return px_h - 1 - static_cast<int>(v * (px_h - 1));
        };
        for (int x = 0; x + 1 < px_w; ++x) {
            int di0 = n - px_w + x,   di1 = n - px_w + x + 1;
            float v0 = (di0 >= 0 && di0 < n) ? data[di0] : 0.0f;
            float v1 = (di1 >= 0 && di1 < n) ? data[di1] : 0.0f;
            int y0 = val_to_y(v0), y1 = val_to_y(v1);
            int ya = std::min(y0, y1), yb = std::max(y0, y1);
            for (int y = ya; y <= yb; ++y)
                c.DrawPointOn(x, y);
        }
    }) | size(HEIGHT, EQUAL, char_height)
       | color(line_color);

    return dbox(Elements{fill_elem, line_elem}) | flex;
}

inline ftxui::Element gradient_gauge(double pct, int width = 24) {
    using namespace ftxui;
    pct = std::clamp(pct, 0.0, 100.0);
    const int filled = static_cast<int>(pct / 100.0 * width);
    Elements segs;
    for (int i = 0; i < width; ++i) {
        const double seg_pct = static_cast<double>(i) / (width - 1) * 100.0;
        if (i < filled)
            segs.push_back(text("█") | color(theme::usage_color(seg_pct)));
        else
            segs.push_back(text("░") | color(theme::kDimText));
    }
    return hbox(std::move(segs));
}

inline std::string mini_bar(int filled, int total,
                              std::string_view on  = "▪",
                              std::string_view off = "·") {
    return repeat(on, filled) + repeat(off, total - filled);
}

inline ftxui::Element yaxis(double max_val, std::string_view unit, int height) {
    using namespace ftxui;
    Elements rows;
    const int label_w = (unit == "%") ? 4 : 6;
    for (int i = 0; i < height; ++i) {
        const double v = max_val * (1.0 - static_cast<double>(i) / std::max(height - 1, 1));
        if (i == 0 || i == height / 2 || i == height - 1) {
            std::string s = (unit == "%")
                ? fmt::format("{:3.0f}%", v)
                : fmt::format("{:.1f}", v);
            rows.push_back(text(s) | color(theme::kDimText) | size(WIDTH, EQUAL, label_w));
        } else {
            rows.push_back(text(std::string(label_w, ' ')));
        }
    }
    return vbox(std::move(rows));
}

template <std::size_t N>
ftxui::Element sparkgraph(
    const core::RingBuffer<double, N>& buf,
    std::string_view                   label,
    std::string_view                   unit,
    double                             max_val,
    ftxui::Color                       col,
    int                                height = 6)
{
    using namespace ftxui;

    const double current = buf.empty() ? 0.0 : buf.back();
    double peak = 0.0;
    for (std::size_t i = 0; i < buf.size(); ++i) peak = std::max(peak, buf[i]);

    const double pct = (unit == "%") ? current
                                     : (max_val > 0 ? current / max_val * 100.0 : 0.0);

    const int bar_w  = 10;
    const int filled = static_cast<int>(pct / 100.0 * bar_w);
    std::string hbar = repeat("█", filled) + repeat("░", bar_w - filled);

    auto header = hbox(
        text(std::string(label)) | bold | color(col),
        text("  "),
        text(hbar) | color(theme::usage_color(pct)),
        text(fmt::format("  {:.1f}{}", current, unit)) | bold | color(theme::usage_color(pct)),
        filler(),
        text("peak ") | color(theme::kDimText),
        text(fmt::format("{:.1f}{}", peak, unit)) | color(theme::kDimText)
    );

    auto body = hbox(
        yaxis(max_val, unit, height),
        text("┤") | color(theme::kDimText),
        braille_graph(buf, max_val, col, height)
    );

    return vbox(header, separator() | color(theme::kDimText), body)
           | border | color(col);
}

template <std::size_t N>
ftxui::Element dual_sparkgraph(
    const core::RingBuffer<double, N>& buf_a,
    const core::RingBuffer<double, N>& buf_b,
    std::string_view                   label,
    std::string_view                   label_a,
    std::string_view                   label_b,
    std::string_view                   unit,
    double                             max_val,
    ftxui::Color                       color_a,
    ftxui::Color                       color_b,
    int                                height = 8)
{
    using namespace ftxui;

    const double cur_a = buf_a.empty() ? 0.0 : buf_a.back();
    const double cur_b = buf_b.empty() ? 0.0 : buf_b.back();

    double dyn_max = max_val;
    if (dyn_max <= 0.0) {
        for (std::size_t i = 0; i < buf_a.size(); ++i) dyn_max = std::max(dyn_max, buf_a[i]);
        for (std::size_t i = 0; i < buf_b.size(); ++i) dyn_max = std::max(dyn_max, buf_b[i]);
        dyn_max = std::max(dyn_max, 0.001);
    }

    const int half = std::max(height / 2, 2);

    auto header = hbox(
        text(std::string(label)) | bold | color(theme::kBrightText),
        filler(),
        text(fmt::format(" ▲ {}: ", label_a)) | color(color_a),
        text(fmt::format("{:.2f}{}", cur_a, unit)) | bold | color(color_a),
        text(fmt::format("   ▼ {}: ", label_b)) | color(color_b),
        text(fmt::format("{:.2f}{}", cur_b, unit)) | bold | color(color_b),
        text(fmt::format("   max {:.2f}{}", dyn_max, unit)) | color(theme::kDimText)
    );

    auto scale_hi = text(fmt::format("{:.1f}{}", dyn_max, unit))
                    | color(theme::kDimText) | size(WIDTH, EQUAL, 8);
    auto scale_lo = text("0       ") | color(theme::kDimText) | size(WIDTH, EQUAL, 8);

    return vbox(
        header,
        separator() | color(theme::kDimText),
        hbox(scale_hi, text("┤") | color(color_a),
             braille_graph(buf_a, dyn_max, color_a, half)),
        hbox(scale_lo, text("┤") | color(theme::kDimText), separatorEmpty() | flex),
        hbox(scale_hi, text("┤") | color(color_b),
             braille_graph(buf_b, dyn_max, color_b, half) | inverted)
    ) | border;
}

} // namespace htop_killer::ui
