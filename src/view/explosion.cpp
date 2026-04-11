#include "view/explosion.h"
#include <cmath>
#include <algorithm>

// Simple deterministic pseudo-random using a mutable counter so we don't
// need <random> headers — fine for a purely visual effect.
static float frand(float& seed) {
    seed = seed * 1664525.0f + 1013904223.0f;
    union { float f; unsigned u; } u;
    u.u = (0x3f800000u | (static_cast<unsigned>(seed) & 0x007fffffu));
    return u.f - 1.0f;   // [0, 1)
}

void ExplosionSystem::spawn(float x, float y, Color baseColor) {
    float seed = x * 1234.5f + y * 6789.0f + (float)particles.size() * 17.3f;

    for (int i = 0; i < COUNT; ++i) {
        float angle  = frand(seed) * 6.2831853f;          // full circle
        float speed  = SPEED_MIN + frand(seed) * (SPEED_MAX - SPEED_MIN);
        float radius = RADIUS_MAX * (0.4f + 0.6f * frand(seed));

        // Randomise colour slightly: orange/yellow for fire, white sparks
        Color c = baseColor;
        bool  spark = frand(seed) > 0.7f;
        if (spark) {
            c = Color{255, 240, 200, 255};
        } else {
            c.r = (unsigned char)std::min(255, (int)c.r + (int)(frand(seed) * 60));
            c.g = (unsigned char)(c.g * (0.3f + 0.4f * frand(seed)));
            c.b = (unsigned char)(c.b * 0.2f);
        }

        // Bias velocity upward so the burst pops visually above the unit
        particles.push_back({x, y,
                              std::cos(angle) * speed,
                              std::sin(angle) * speed - 40.0f,
                              radius, 0.0f, c});
    }
}

void ExplosionSystem::update(float dt) {
    for (auto& p : particles) {
        p.age += dt;
        p.x   += p.vx * dt;
        p.y   += p.vy * dt;
        // Drag
        p.vx  *= (1.0f - 4.5f * dt);
        p.vy  *= (1.0f - 4.5f * dt);
        // Shrink towards end of life
        float t    = p.age / DURATION;
        p.radius   = RADIUS_MAX * (1.0f - t) * (p.radius / RADIUS_MAX);
    }
    particles.erase(
        std::remove_if(particles.begin(), particles.end(),
            [](const Particle& p){ return p.age >= DURATION; }),
        particles.end());
}

void ExplosionSystem::render() const {
    for (const auto& p : particles) {
        float t     = p.age / DURATION;
        float alpha = 1.0f - t * t;   // quadratic fade
        alpha       = std::max(0.0f, std::min(1.0f, alpha));

        Color c = p.color;
        c.a = (unsigned char)(255.0f * alpha);

        DrawCircleV({p.x, p.y}, std::max(1.0f, p.radius), c);
    }
}
