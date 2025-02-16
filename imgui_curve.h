// [src] https://github.com/ocornut/imgui/issues/123
// [src] https://github.com/ocornut/imgui/issues/55

// v1.23 - selection index track, value range, context menu, improve manipulation controls (D.Click
// to add/delete, drag to add) v1.22 - flip button; cosmetic fixes v1.21 - oops :) v1.20 - add iq's
// interpolation code v1.10 - easing and colors v1.00 - jari komppa's original

#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"

namespace ImGui {
extern const float CurveTerminator;

int Curve(const char* label,
    const ImVec2& size,
    float handleSize,
    const int maxpoints,
    ImVec2* points,
    int* selection,
    const ImVec2& rangeMin = ImVec2(0, 0),
    const ImVec2& rangeMax = ImVec2(1, 1));

float CurveValue(float p, int maxpoints, const ImVec2* points);
float CurveValueSmooth(float p, int maxpoints, const ImVec2* points);

}; // namespace ImGui
