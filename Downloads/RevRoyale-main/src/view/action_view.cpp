#include "view/action_view.h"
#include "raylib.h"
#include <cmath>

ActionView::ActionView() {}

void ActionView::render(const std::vector<std::string>& labels,
                        const std::vector<Rect>& buttonSlots,
                        int pendingIndex,
                        const std::vector<bool>& enabled) const
{
    float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 5.0f);

    for (int i = 0; i < (int)labels.size() && i < (int)buttonSlots.size(); ++i) {
        const Rect& r  = buttonSlots[i];
        bool active    = (i == pendingIndex);
        bool isEnabled = (i >= (int)enabled.size()) || enabled[i]; // default true if no mask

        if (!isEnabled) {
            DrawRectangle(r.x, r.y, r.w, r.h, Color{28, 28, 38, 255});
            DrawRectangleLines(r.x, r.y, r.w, r.h, Color{55, 55, 68, 255});
            DrawText(labels[i].c_str(), r.x+10, r.y+11, 16, Color{90, 90, 105, 255});
            continue;
        }

        Color bg  = active ? Color{35, 150, 55, 255} : Color{45, 45, 65, 255};
        Color bdr = active ? Color{80, 220, 100, 255} : Color{90, 90, 110, 255};
        DrawRectangle(r.x, r.y, r.w, r.h, bg);
        DrawRectangleLines(r.x, r.y, r.w, r.h, bdr);

        if (active) {
            unsigned char g1 = (unsigned char)(90 + 90 * pulse);
            unsigned char g2 = (unsigned char)(40 + 40 * pulse);
            DrawRectangleLines(r.x-1, r.y-1, r.w+2, r.h+2, Color{100, 255, 120, g1});
            DrawRectangleLines(r.x-2, r.y-2, r.w+4, r.h+4, Color{100, 255, 120, g2});
            DrawTriangle(
                {(float)(r.x - 12), (float)(r.y + r.h/2 - 5)},
                {(float)(r.x - 12), (float)(r.y + r.h/2 + 5)},
                {(float)(r.x -  4), (float)(r.y + r.h/2)},
                Color{120, 255, 140, 255});
        }

        DrawText(labels[i].c_str(), r.x+10, r.y+11, 16, WHITE);
    }
}
