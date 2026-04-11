#pragma once

#include <string>
#include <vector>
#include "raylib.h"

// ---------------------------------------------------------------------------
// DamageIndicatorSystem
//
// Manages floating text indicators (damage numbers, "MISS", etc.) that spawn
// at a screen position, drift upward, and fade out over their lifetime.
// ---------------------------------------------------------------------------

struct DamageIndicator {
    std::string text;
    float x, y;       // current screen position
    float age;        // seconds since spawn
    Color color;
};

class DamageIndicatorSystem {
public:
    /** Spawn a new floating label at screen position (x, y). */
    void spawn(const std::string& text, float x, float y, Color color);

    /** Advance all indicators by dt seconds and remove expired ones. */
    void update(float dt);

    /** Draw all active indicators. */
    void render() const;

private:
    static constexpr float DURATION   = 1.4f;   // total lifetime in seconds
    static constexpr float RISE_SPEED = 50.0f;  // pixels per second upward

    std::vector<DamageIndicator> indicators;
};
