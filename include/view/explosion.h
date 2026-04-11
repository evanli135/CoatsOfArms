#pragma once
#include <vector>
#include "raylib.h"

// ---------------------------------------------------------------------------
// ExplosionSystem — burst-of-particles pop effect for unit deaths.
//
// Spawn a burst at a screen position; particles fly outward, shrink, and
// fade over DURATION seconds. Entirely self-contained; no model access.
// ---------------------------------------------------------------------------

struct Particle {
    float x, y;     // current position
    float vx, vy;   // velocity (px/s)
    float radius;   // current radius
    float age;      // seconds alive
    Color color;
};

class ExplosionSystem {
public:
    static constexpr float DURATION    = 1.0f;   // seconds before particles expire
    static constexpr int   COUNT       = 40;     // particles per burst
    static constexpr float SPEED_MIN   = 80.0f;
    static constexpr float SPEED_MAX   = 230.0f;
    static constexpr float RADIUS_MAX  = 11.0f;

    /** Spawn a burst centred at (x, y) with the given base colour. */
    void spawn(float x, float y, Color baseColor);

    /** Advance all particles by dt seconds; prune expired ones. */
    void update(float dt);

    /** Draw all live particles. */
    void render() const;

private:
    std::vector<Particle> particles;
};
