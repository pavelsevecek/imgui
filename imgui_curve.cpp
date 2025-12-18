// [src] https://github.com/ocornut/imgui/issues/123
// [src] https://github.com/ocornut/imgui/issues/55

// v1.23 - selection index track, value range, context menu, improve manipulation controls (D.Click to add/delete, drag to add)
// v1.22 - flip button; cosmetic fixes
// v1.21 - oops :)
// v1.20 - add iq's interpolation code
// v1.10 - easing and colors
// v1.00 - jari komppa's original


#include "imgui_curve.h"
#include "imgui_internal.h"

#include <cmath>

/* To use, add this prototype somewhere..
namespace ImGui
{
    int Curve(const char *label, const ImVec2& size, int maxpoints, ImVec2 *points);
    float CurveValue(float p, int maxpoints, const ImVec2 *points);
    float CurveValueSmooth(float p, int maxpoints, const ImVec2 *points);
};
*/
/*
    Example of use:
    ImVec2 foo[10];
    int selectionIdx = -1;
    ...
    foo[0].x = ImGui::CurveTerminator; // init data so editor knows to take it from here
    ...
    if (ImGui::Curve("Das editor", ImVec2(600, 200), 10, foo, &selectionIdx))
    {
        // curve changed
    }
    ...
    float value_you_care_about = ImGui::CurveValue(0.7f, 10, foo); // calculate value at position 0.7
*/


namespace ImGui
{
const float CurveTerminator = -10000;

// [src] http://iquilezles.org/www/articles/minispline/minispline.htm
// key format (for dim == 1) is (t0,x0,t1,x1 ...)
// key format (for dim == 2) is (t0,x0,y0,t1,x1,y1 ...)
// key format (for dim == 3) is (t0,x0,y0,z0,t1,x1,y1,z1 ...)
template<int DIM>
void spline(const float* key, int num, float t, float* v)
{
    static float coefs[16] = {
        -1.0f, 2.0f,-1.0f, 0.0f,
         3.0f,-5.0f, 0.0f, 2.0f,
        -3.0f, 4.0f, 1.0f, 0.0f,
         1.0f,-1.0f, 0.0f, 0.0f
    };

    const int size = DIM + 1;

    // find key
    int k = 1;
    while (k < num - 1 && key[k * size] < t)
        k++;   

    const float key0 = key[(k - 1) * size];
    const float key1 = key[k * size];

    // interpolant
    const float h = key1 != key0 ? (t - key0) / (key1 - key0) : 0.f;

    // init result
    for (int i = 0; i < DIM; i++)
        v[i] = 0.0f;

    // add basis functions
    for (int i = 0; i < 4; ++i)
    {
        const float* co = &coefs[4 * i];
        const float b = 0.5f * (((co[0] * h + co[1]) * h + co[2]) * h + co[3]);

        const int kn = ImClamp(k + i - 2, 0, num - 1);
        for (int j = 0; j < DIM; j++)
            v[j] += b * key[kn * size + j + 1];
    }
}

float CurveValueSmooth(float p, int maxpoints, const ImVec2* points)
{
    if (maxpoints < 2 || points == 0)
        return 0;
    if (p < 0)
        return points[0].y;

    float* input = new float[maxpoints * 2];
    float output[4];

    for (int i = 0; i < maxpoints; ++i)
    {
        input[i * 2 + 0] = points[i].x;
        input[i * 2 + 1] = points[i].y;
    }

    spline<1>(input, maxpoints, p, output);

    delete[] input;
    return output[0];
}

float CurveValue(float p, int maxpoints, const ImVec2* points)
{
    if (maxpoints < 2 || points == 0)
        return 0;
    if (p < 0)
        return points[0].y;

    int left = 0;
    while (left < maxpoints && points[left].x < p && points[left].x != -1)
        left++;
    if (left)
        left--;

    if (left == maxpoints - 1)
        return points[maxpoints - 1].y;

    float d = (p - points[left].x) / (points[left + 1].x - points[left].x);

    return points[left].y + (points[left + 1].y - points[left].y) * d;
}

static inline float ImRemap(float v, float a, float b, float c, float d)
{
    return (c + (d - c) * (v - a) / (b - a));
}

static inline ImVec2 ImRemap(const ImVec2& v, const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d)
{
    return ImVec2(ImRemap(v.x, a.x, b.x, c.x, d.x), ImRemap(v.y, a.y, b.y, c.y, d.y));
}

int Curve(const char* label, const ImVec2& size, float handleSize, const int maxpoints, ImVec2* points, int* selection, const ImVec2& rangeMin, const ImVec2& rangeMax)
{
    int modified = 0;
    int i;
    if (maxpoints < 2 || points == nullptr)
        return 0;

    if (points[0].x <= CurveTerminator)
    {
        points[0] = rangeMin;
        points[1] = rangeMax;
        points[2].x = CurveTerminator;
    }

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

    const bool hovered = ImGui::ItemHoverable(bb, id, ImGuiItemFlags_None);
    if (hovered) {
        SetLastItemData(id, ImGuiItemFlags_None, ImGuiItemStatusFlags_HoveredRect, bb);
    } 

    int pointCount = 0;
    while (pointCount < maxpoints && points[pointCount].x >= rangeMin.x)
        pointCount++;

    const ImGuiStyle& style = g.Style;
    RenderFrame(bb.Min, bb.Max, GetColorU32(ImGuiCol_FrameBg, 1), true, style.FrameRounding);

    const float ht = bb.Max.y - bb.Min.y;
    const float wd = bb.Max.x - bb.Min.x;

    int hoveredPoint = -1;

    const float pointRadiusInPixels = handleSize;

    // Handle point selection
    if (hovered)
    {
        ImVec2 hoverPos = (g.IO.MousePos - bb.Min) / (bb.Max - bb.Min);
        hoverPos.y = 1.0f - hoverPos.y;

        ImVec2 pos = ImRemap(hoverPos, ImVec2(0, 0), ImVec2(1, 1), rangeMin, rangeMax);

        int left = 0;
        while (left < pointCount && points[left].x < pos.x)
            left++;
        if (left)
            left--;

        const ImVec2 hoverPosScreen = ImRemap(hoverPos, ImVec2(0, 0), ImVec2(1, 1), bb.Min, bb.Max);
        const ImVec2 p1s = ImRemap(points[left], rangeMin, rangeMax, bb.Min, bb.Max);
        const ImVec2 p2s = ImRemap(points[left + 1], rangeMin, rangeMax, bb.Min, bb.Max);

        const float p1d = ImSqrt(ImLengthSqr(p1s - hoverPosScreen));
        const float p2d = ImSqrt(ImLengthSqr(p2s - hoverPosScreen));

        if (p1d < 2 * pointRadiusInPixels)
            hoveredPoint = left;

        if (p2d < 2 * pointRadiusInPixels)
            hoveredPoint = left + 1;

        if (g.IO.MouseDown[0])
        {
            if (currentSelection == -1)
                currentSelection = hoveredPoint;
        }
        else
            currentSelection = -1;

        enum
        {
            action_none,
            action_add_point,
            action_delete_point
        };

        int action = action_none;

        if (currentSelection == -1)
        {
            if (g.IO.MouseDoubleClicked[0])
                action = action_add_point;
        }
        else if(g.IO.MouseDoubleClicked[0])
            action = action_delete_point;

        if (action == action_add_point)
        {
            if (pointCount < maxpoints)
            {
                // select
                currentSelection = left + 1;

                ++pointCount;
                for (i = pointCount; i > left; --i)
                    points[i] = points[i - 1];

                points[left + 1] = pos;

                if (pointCount < maxpoints)
                    points[pointCount].x = CurveTerminator;

                modified = 1;
            }
        }
        else if (action == action_delete_point)
        {
            // delete point
            if (currentSelection > 0 && currentSelection < pointCount - 1)
            {
                for (i = currentSelection; i < maxpoints - 1; ++i)
                    points[i] = points[i + 1];

                --pointCount;
                points[pointCount].x = CurveTerminator;
                currentSelection = -1;
                modified = 1;
            }
        }
    }

    // handle point dragging
    const bool draggingPoint = IsMouseDragging(0) && currentSelection != -1;

    if (draggingPoint)
    {
        if (selection)
            SetActiveID(id, window);

        SetFocusID(id, window);
        FocusWindow(window);

        modified = 1;

        ImVec2 pos = (g.IO.MousePos - bb.Min) / (bb.Max - bb.Min);

        // constrain Y to min/max
        pos.y = 1.0f - pos.y;
        pos = ImRemap(pos, ImVec2(0, 0), ImVec2(1, 1), rangeMin, rangeMax);

        // constrain X to the min left/ max right
        const float pointXRangeMin = (currentSelection > 0) ? points[currentSelection - 1].x + 0.01f : rangeMin.x;
        const float pointXRangeMax = (currentSelection + 1 < pointCount) ? points[currentSelection + 1].x - 0.01f : rangeMax.x;

        pos = ImClamp(pos, ImVec2(pointXRangeMin, rangeMin.y), ImVec2(pointXRangeMax, rangeMax.y));

        points[currentSelection] = pos;

        // snap X first/last to min/max
        if (points[0].x < points[pointCount - 1].x)
        {
            points[0].x = rangeMin.y;
            points[pointCount - 1].x = rangeMax.x;
        }
        else
        {
            points[0].x = rangeMax.x;
            points[pointCount - 1].x = rangeMin.y;
        }
    }

    if (!IsMouseDragging(0) && GetActiveID() == id && selection && *selection != -1 && currentSelection == -1)
    {
        ClearActiveID();
    }

    const ImU32 gridColor1 = GetColorU32(ImGuiCol_TextDisabled, 0.5f);
    const ImU32 gridColor2 = GetColorU32(ImGuiCol_TextDisabled, 0.25f);

    ImDrawList* drawList = window->DrawList;

    // bg grid
    drawList->AddLine(ImVec2(bb.Min.x, bb.Min.y + ht / 2), ImVec2(bb.Max.x, bb.Min.y + ht / 2), gridColor1, 3);

    drawList->AddLine(ImVec2(bb.Min.x, bb.Min.y + ht / 4), ImVec2(bb.Max.x, bb.Min.y + ht / 4), gridColor1);

    drawList->AddLine(ImVec2(bb.Min.x, bb.Min.y + ht / 4 * 3), ImVec2(bb.Max.x, bb.Min.y + ht / 4 * 3), gridColor1);

    for (i = 0; i < 7; i++)
    {
        drawList->AddLine(ImVec2(bb.Min.x + (wd / 8) * (i + 1), bb.Min.y), ImVec2(bb.Min.x + (wd / 8) * (i + 1), bb.Max.y), gridColor2);
    }

    drawList->PushClipRect(bb.Min, bb.Max);

    // smooth curve
    enum
    {
        smoothness = 256
    }; // the higher the smoother
    for (i = 0; i <= (smoothness - 1); ++i)
    {
        float px = (i + 0) / float(smoothness);
        float qx = (i + 1) / float(smoothness);

        px = ImRemap(px, 0, 1, rangeMin.x, rangeMax.x);
        qx = ImRemap(qx, 0, 1, rangeMin.x, rangeMax.x);

        const float py = CurveValueSmooth(px, pointCount, points);
        const float qy = CurveValueSmooth(qx, pointCount, points);

        ImVec2 p = ImRemap(ImVec2(px, py), rangeMin, rangeMax, ImVec2(0,0), ImVec2(1,1));
        ImVec2 q = ImRemap(ImVec2(qx, qy), rangeMin, rangeMax, ImVec2(0,0), ImVec2(1,1));
        p.y = 1.0f - p.y;
        q.y = 1.0f - q.y;

        p = ImRemap(p, ImVec2(0,0), ImVec2(1,1), bb.Min, bb.Max);
        q = ImRemap(q, ImVec2(0,0), ImVec2(1,1), bb.Min, bb.Max);

        drawList->AddLine(p, q, GetColorU32(ImGuiCol_PlotHistogram));
    }

    // lines
    /* for (i = 1; i < pointCount; i++)
    {
        ImVec2 a = ImRemap(points[i - 1], rangeMin, rangeMax, ImVec2(0, 0), ImVec2(1, 1));
        ImVec2 b = ImRemap(points[i], rangeMin, rangeMax, ImVec2(0, 0), ImVec2(1, 1));

        a.y = 1.0f - a.y;
        b.y = 1.0f - b.y;

        a = ImRemap(a, ImVec2(0,0), ImVec2(1,1), bb.Min, bb.Max);
        b = ImRemap(b, ImVec2(0,0), ImVec2(1,1), bb.Min, bb.Max);

        drawList->AddLine(a, b, GetColorU32(ImGuiCol_PlotLines, 0.5f));
    }*/

    if (hovered || draggingPoint)
    {
        // control points
        for (i = 0; i < pointCount; i++)
        {
            ImVec2 p = ImRemap(points[i], rangeMin, rangeMax, ImVec2(0, 0), ImVec2(1, 1));
            p.y = 1.0f - p.y;
            p = ImRemap(p, ImVec2(0, 0), ImVec2(1, 1), bb.Min, bb.Max);

            ImVec2 a = p - ImVec2(pointRadiusInPixels, pointRadiusInPixels);
            ImVec2 b = p + ImVec2(pointRadiusInPixels, pointRadiusInPixels);
            if (hoveredPoint == i)
                drawList->AddRect(a, b, IM_COL32_WHITE);
            else
                drawList->AddCircle(p, pointRadiusInPixels, IM_COL32_WHITE);
        }
    }

    drawList->PopClipRect();

    PopID();

    if (selection)
    {
        *selection = currentSelection;
    }

    return modified;
}

}; // namespace ImGui
