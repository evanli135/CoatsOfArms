#include "view/damage_indicators.h"
#include <algorithm>
#include <cmath>

void DamageIndicatorSystem::spawn(const std::string& text, float x, float y, Color color) {
    indicators.push_back({text, x, y, 0.0f, color});
}

void DamageIndicatorSystem::update(float dt) {
    for (auto& ind : indicators) {
        ind.age += dt;
        ind.y   -= RISE_SPEED * dt;
    }
    indicators.erase(
        std::remove_if(indicators.begin(), indicators.end(),
            [](const DamageIndicator& d) { return d.age >= DURATION; }),
        indicators.end());
}

void DamageIndicatorSystem::render() const {
    for (const auto& ind : indicators) {
        float t = ind.age / DURATION;

        // Quick fade-in (first 15%), then fade-out for the rest
        float alpha = t < 0.15f ? (t / 0.15f)
                                : (1.0f - (t - 0.15f) / 0.85f);
        alpha = std::max(0.0f, std::min(1.0f, alpha));

        Color c = ind.color;
        c.a = (unsigned char)(255.0f * alpha);

        // Outline for legibility — dark shadow offset by 1 px
        Color shadow = {0, 0, 0, c.a};
        DrawText(ind.text.c_str(), (int)ind.x + 1, (int)ind.y + 1, 20, shadow);
        DrawText(ind.text.c_str(), (int)ind.x,     (int)ind.y,     20, c);
    }
}
