#include "view/action_view.h"
#include "raylib.h"
#include <cmath>
#include <string>

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
        bool isEnabled = (i >= (int)enabled.size()) || enabled[i];

        // Split label on '\n': first part = main text, second = hint/reason
        const std::string& full = labels[i];
        auto nl = full.find('\n');
        std::string mainText = (nl != std::string::npos) ? full.substr(0, nl) : full;
        std::string subText  = (nl != std::string::npos) ? full.substr(nl + 1) : "";
        bool hasSub = !subText.empty();

        // Vertical placement: single-line centres at y+11 (size 16);
        // two-line fits: main at y+6 (size 13) + sub at y+23 (size 10).
        int mainY    = hasSub ? r.y + 6  : r.y + 11;
        int mainSize = hasSub ? 13        : 16;
        int subY     = r.y + 24;

        if (!isEnabled) {
            DrawRectangle(r.x, r.y, r.w, r.h, Color{70, 22, 22, 255});
            DrawRectangleLines(r.x, r.y, r.w, r.h, Color{150, 50, 50, 255});
            DrawText(mainText.c_str(), r.x + 10, mainY, mainSize, Color{180, 80, 80, 255});
            if (hasSub)
                DrawText(subText.c_str(), r.x + 10, subY, 10, Color{200, 95, 95, 215});
            continue;
        }

        // Active (selected): bright green with glow.
        // Enabled (available): medium green.
        Color bg  = active ? Color{35, 150, 55, 255} : Color{25, 75, 38, 255};
        Color bdr = active ? Color{80, 220, 100, 255} : Color{55, 145, 75, 255};
        Color tc  = active ? WHITE : Color{140, 220, 155, 255};
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

        DrawText(mainText.c_str(), r.x + 10, mainY, mainSize, tc);
        if (hasSub) {
            Color subTc = active ? Color{160, 235, 175, 185} : Color{95, 165, 110, 170};
            DrawText(subText.c_str(), r.x + 10, subY, 10, subTc);
        }
    }
}
