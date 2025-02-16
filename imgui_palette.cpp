#include "imgui_curve.h"
#include "imgui_internal.h"

#include <cmath>

namespace ImGui {

static inline float ImRemap(float v, float a, float b, float c, float d) {
    return (c + (d - c) * (v - a) / (b - a));
}

static inline ImVec2 ImRemap(const ImVec2& v,
    const ImVec2& a,
    const ImVec2& b,
    const ImVec2& c,
    const ImVec2& d) {
    return ImVec2(ImRemap(v.x, a.x, b.x, c.x, d.x), ImRemap(v.y, a.y, b.y, c.y, d.y));
}

static ImVec4 ContrastColor(ImVec4 color) {
    const float maxContrast = 0.75f;
    const float minContrast = 0.5f;
    const float y = 0.299f * color.x + 0.587f * color.y + 0.114f * color.z;
    float oy = 1.f - y;
    float dy = oy - y;
    const int sign = dy > 0.f ? 1 : -1;
    if (abs(dy) > maxContrast) {
        dy = sign * maxContrast;
        oy = y + dy;
    } else if (abs(dy) < minContrast) {
        dy = sign * minContrast;
        oy = y + dy;
    }
    return ImVec4(oy, oy, oy, 1);
}

bool Palette(const char* label,
    const ImVec2& size,
    int* colorCount,
    const int maxColorCount,
    ImVec4* colors,
    float* positions,
    const float maxPosition,
    float grabSize,
    int* selection) {
    bool modified = false;
    int i;
    if (maxColorCount < 2 || colors == nullptr)
        return 0;

    ImGuiWindow* window = GetCurrentWindow();
    ImGuiContext& g = *GImGui;

    const ImGuiID id = window->GetID(label);
    if (window->SkipItems)
        return 0;

    ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
    ItemSize(bb);
    if (!ItemAdd(bb, id))
        return 0;

    PushID(label);

    int currentSelection = selection ? *selection : -1;

    const bool hovered = ItemHoverable(bb, id, ImGuiItemFlags_None);
    if (hovered) {
        SetLastItemData(id, ImGuiItemFlags_None, ImGuiItemStatusFlags_HoveredRect, bb);
    }

    const ImGuiStyle& style = g.Style;
    RenderFrame(bb.Min, bb.Max, GetColorU32(ImGuiCol_FrameBg, 1), true, style.FrameRounding);

    const float ht = bb.Max.y - bb.Min.y;
    const float wd = bb.Max.x - bb.Min.x;

    int hoveredPoint = -1;

    static ImVec4 referenceColor;
    // Handle point selection
    if (hovered) {
        float pos = ImRemap(g.IO.MousePos.x, bb.Min.x, bb.Max.x, 0, maxPosition);

        int left = 0;
        while (left < *colorCount && positions[left] < pos)
            left++;
        if (left)
            left--;

        const float p1s = ImRemap(positions[left], 0, maxPosition, bb.Min.x, bb.Max.x);
        const float p2s = ImRemap(positions[left + 1], 0, maxPosition, bb.Min.x, bb.Max.x);

        const float p1d = abs(p1s - g.IO.MousePos.x);
        const float p2d = abs(p2s - g.IO.MousePos.x);

        if (p1d < 2 * grabSize)
            hoveredPoint = left;

        if (p2d < 2 * grabSize)
            hoveredPoint = left + 1;

        if (g.IO.MouseDown[0] || g.IO.MouseClicked[1]) {
            if (currentSelection == -1)
                currentSelection = hoveredPoint;
        } else
            currentSelection = -1;

        enum { action_none, action_add_point, action_delete_point, action_color_pick };

        int action = action_none;

        if (currentSelection == -1) {
            if (g.IO.MouseDoubleClicked[0])
                action = action_add_point;
        } else {
            if (g.IO.MouseDoubleClicked[0] && currentSelection > 0 &&
                currentSelection < *colorCount - 1)
                action = action_delete_point;
            else if (g.IO.MouseClicked[1])
                action = action_color_pick;
        }

        if (action == action_add_point) {
            if (*colorCount < maxColorCount) {
                // select
                currentSelection = left + 1;
                ImVec4 leftColor = colors[left];
                ImVec4 rightColor = colors[left + 1];
                float leftPos = positions[left];
                float rightPos = positions[left + 1];
                float blend = (pos - leftPos) / (rightPos - leftPos);
                ImVec4 midColor;
                midColor.x = (1 - blend) * leftColor.x + blend * rightColor.x;
                midColor.y = (1 - blend) * leftColor.y + blend * rightColor.y;
                midColor.z = (1 - blend) * leftColor.z + blend * rightColor.z;
                midColor.w = 1;


                ++(*colorCount);
                for (i = *colorCount; i > left; --i) {
                    colors[i] = colors[i - 1];
                    positions[i] = positions[i - 1];
                }

                positions[left + 1] = pos;
                colors[left + 1] = midColor;

                modified = true;
            }
        } else if (action == action_delete_point) {
            // delete point
            if (currentSelection > 0 && currentSelection < maxColorCount - 1) {
                for (i = currentSelection; i < maxColorCount - 1; ++i) {
                    colors[i] = colors[i + 1];
                    positions[i] = positions[i + 1];
                }

                --(*colorCount);
                currentSelection = -1;
                modified = true;
            }
        }
        if (action == action_color_pick) {
            OpenPopup("picker", ImGuiPopupFlags_MouseButtonLeft);
            referenceColor = colors[currentSelection];
        }
    }

    if (currentSelection != -1) {
        const float frameSize = GetFrameHeight() * 9.0f;
        SetNextWindowPos(ImVec2(std::min(bb.Min.x, bb.Max.x - 1.25f * frameSize), bb.Max.y));
        if (BeginPopup("picker")) {
            // picker_active_window = g.CurrentWindow;
            /* if(label != label_display_end) {
                TextEx(label, label_display_end);
                Spacing();
            }*/
            ImGuiColorEditFlags picker_flags =
                ImGuiColorEditFlags_Uint8 | ImGuiColorEditFlags_DisplayRGB |
                ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_InputRGB |
                ImGuiColorEditFlags_NoAlpha;

            SetNextItemWidth(frameSize);
            modified |= ColorPicker4(
                "##picker", &colors[currentSelection].x, picker_flags, &referenceColor.x);
            EndPopup();
        }
    }

    // handle point dragging
    const bool draggingPoint = IsMouseDragging(0) && currentSelection > 0 &&
                               currentSelection < *colorCount - 1 && !IsPopupOpen("picker");

    if (draggingPoint) {
        if (selection)
            SetActiveID(id, window);

        SetFocusID(id, window);
        FocusWindow(window);

        modified = true;

        float p = (g.IO.MousePos.x - bb.Min.x) / (bb.Max.x - bb.Min.x);
        float x = ImRemap(g.IO.MousePos.x, bb.Min.x, bb.Max.x, 0, maxPosition);
        positions[currentSelection] = x;
    }

    if (selection && *selection != -1) {
        // constrain X to the min left/ max right
        bool deleteIfMouseUp = false;
        if (*selection > 0 && positions[*selection] <= positions[*selection - 1]) {
            positions[*selection] = positions[*selection - 1];
            deleteIfMouseUp = true;
        }

        if (*selection + 1 < *colorCount && positions[*selection] >= positions[*selection + 1]) {
            positions[*selection] = positions[*selection + 1];
            deleteIfMouseUp = true;
        }

        if (deleteIfMouseUp && IsMouseReleased(0)) {
            for (i = *selection; i < maxColorCount - 1; ++i) {
                colors[i] = colors[i + 1];
                positions[i] = positions[i + 1];
            }

            --(*colorCount);
            //*selection = -1;
            modified = true;
        }
    }

    if (!IsMouseDragging(0) && GetActiveID() == id && selection && *selection != -1 &&
        currentSelection == -1) {
        ClearActiveID();
    }

    const ImU32 gridColor1 = GetColorU32(ImGuiCol_TextDisabled, 0.5f);
    const ImU32 gridColor2 = GetColorU32(ImGuiCol_TextDisabled, 0.25f);

    ImDrawList* drawList = window->DrawList;

    drawList->PushClipRect(bb.Min, bb.Max);

    // draw palette
    for (i = 1; i < *colorCount; i++) {
        ImU32 c1 = ColorConvertFloat4ToU32(colors[i - 1]);
        ImU32 c2 = ColorConvertFloat4ToU32(colors[i]);
        float x1 = positions[i - 1];
        float x2 = positions[i];

        float p1 = ImRemap(x1, 0, maxPosition, bb.Min.x, bb.Max.x);
        float p2 = ImRemap(x2, 0, maxPosition, bb.Min.x, bb.Max.x);

        drawList->AddRectFilledMultiColor(
            ImVec2(p1, bb.Min.y), ImVec2(p2, bb.Max.y), c1, c2, c2, c1);
    }

    if (hovered || draggingPoint) {
        // control points
        for (i = 0; i < *colorCount; i++) {
            float x = positions[i];
            float p = ImRemap(x, 0, maxPosition, bb.Min.x, bb.Max.x);

            ImU32 color = ColorConvertFloat4ToU32(GetStyleColorVec4(ImGuiCol_FrameBg));
            if (i == hoveredPoint) {
                drawList->AddRectFilled(ImVec2(p - grabSize, bb.Min.y),
                    ImVec2(p + grabSize, bb.Min.y + grabSize),
                    color);
                drawList->AddRectFilled(ImVec2(p - grabSize, bb.Max.y - grabSize),
                    ImVec2(p + grabSize, bb.Max.y),
                    color);
                drawList->AddRect(ImVec2(p - grabSize, bb.Min.y),
                    ImVec2(p + grabSize, bb.Min.y + grabSize),
                    IM_COL32_WHITE);
                drawList->AddRect(ImVec2(p - grabSize, bb.Max.y - grabSize),
                    ImVec2(p + grabSize, bb.Max.y),
                    IM_COL32_WHITE);
            } else {
                drawList->AddTriangleFilled(ImVec2(p - grabSize, bb.Min.y),
                    ImVec2(p + grabSize, bb.Min.y),
                    ImVec2(p, bb.Min.y + grabSize),
                    color);
                drawList->AddTriangleFilled(ImVec2(p - grabSize, bb.Max.y),
                    ImVec2(p + grabSize, bb.Max.y),
                    ImVec2(p, bb.Max.y - grabSize),
                    color);
                drawList->AddTriangle(ImVec2(p - grabSize, bb.Min.y),
                    ImVec2(p + grabSize, bb.Min.y),
                    ImVec2(p, bb.Min.y + grabSize),
                    IM_COL32_WHITE);
                drawList->AddTriangle(ImVec2(p - grabSize, bb.Max.y),
                    ImVec2(p + grabSize, bb.Max.y),
                    ImVec2(p, bb.Max.y - grabSize),
                    IM_COL32_WHITE);
            }

            drawList->AddLine(
                ImVec2(p, bb.Max.y - grabSize), ImVec2(p, bb.Min.y + grabSize), IM_COL32_WHITE);
        }
    }

    drawList->PopClipRect();

    PopID();

    if (selection) {
        *selection = currentSelection;
    }

    return modified;
}

}; // namespace ImGui
