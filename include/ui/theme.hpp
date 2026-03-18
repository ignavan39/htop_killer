#pragma once

#include <ftxui/screen/color.hpp>

namespace htop_killer::ui::theme {

using C = ftxui::Color;

inline const C kCpuPrimary   = C::Cyan;
inline const C kCpuSecondary = C{C::Palette256{45}};
inline const C kMemPrimary   = C{C::Palette256{214}};
inline const C kMemSecondary = C{C::Palette256{208}};
inline const C kNetRx        = C{C::Palette256{46}};
inline const C kNetTx        = C{C::Palette256{201}};
inline const C kDiskRead     = C{C::Palette256{39}};
inline const C kDiskWrite    = C{C::Palette256{220}};
inline const C kDimText      = C{C::Palette256{242}};
inline const C kBrightText   = C::White;
inline const C kHeaderBg     = C{C::Palette256{234}};

inline ftxui::Color usage_color(double pct) {
    if (pct < 40.0) return C{C::Palette256{46}};
    if (pct < 70.0) return C{C::Palette256{226}};
    if (pct < 90.0) return C{C::Palette256{208}};
    return                 C{C::Palette256{196}};
}

} // namespace htop_killer::ui::theme
