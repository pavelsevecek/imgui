#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"

namespace ImGui {

bool Palette(const char* label,
    const ImVec2& size,
    int* colorCount,
    const int maxColorCount,
    ImVec4* colors,
    float* positions,
    const float maxPosition,
    float grabSize,
    int* selection);

}; // namespace ImGui
