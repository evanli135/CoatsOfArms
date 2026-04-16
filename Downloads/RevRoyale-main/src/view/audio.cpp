#include "view/audio.h"
#include "raylib.h"
#include <cmath>
#include <vector>

AudioManager& AudioManager::get() {
    static AudioManager inst;
    return inst;
}

// ---------------------------------------------------------------------------
// Procedural click synthesis
//
// Three-layer design:
//   1. Transient  — brief 1400 Hz sine, τ = 3 ms  (gives the "snap")
//   2. Chirp body — 900 → 350 Hz sweep, τ = 20 ms  (gives the "body")
//   3. Noise burst — low-amplitude, τ = 4 ms        (mechanical texture)
// Total duration: 80 ms, 44100 Hz, 16-bit mono.
// ---------------------------------------------------------------------------

static constexpr float PI2 = 6.28318530717958647f;

void AudioManager::init() {
    const int   SR  = 44100;
    const float DUR = 0.080f;
    const int   N   = (int)(SR * DUR);   // ~3528 samples

    std::vector<short> buf(N);

    // LCG for deterministic white noise (seed = 0xDEAD)
    unsigned int rng = 0xDEAD;

    for (int i = 0; i < N; ++i) {
        float t = (float)i / (float)SR;

        // --- Layer 1: transient snap ---
        float transient = 0.40f * sinf(PI2 * 1400.f * t)
                          * expf(-t / 0.003f);

        // --- Layer 2: chirp body ---
        // Frequency sweeps from 900 Hz → 350 Hz via (1 - e^(-t/tau)).
        // Phase = integral of 2π·freq(τ) dτ from 0 to t.
        //   ∫ 2π·[900 - 550·(1-e^(-τ/0.015))] dτ
        // = 2π·[ 350·t + 8.25·(1 - e^(-t/0.015)) ]
        float chirpPhase = PI2 * (350.f * t + 8.25f * (1.f - expf(-t / 0.015f)));
        float body = 0.80f * sinf(chirpPhase) * expf(-t / 0.020f);

        // --- Layer 3: noise burst ---
        rng = rng * 1664525u + 1013904223u;
        float rn = (float)(int)(rng >> 1) / (float)0x40000000 - 1.f;
        float noise = 0.12f * rn * expf(-t / 0.004f);

        // Mix and clamp
        float s = transient + body + noise;
        if (s >  1.f) s =  1.f;
        if (s < -1.f) s = -1.f;
        buf[i] = (short)(s * 32767.f);
    }

    Wave wave{};
    wave.frameCount = (unsigned int)N;
    wave.sampleRate = (unsigned int)SR;
    wave.sampleSize = 16;
    wave.channels   = 1;
    wave.data       = buf.data();

    clickSound_ = LoadSoundFromWave(wave);
    ready_ = true;
}

void AudioManager::shutdown() {
    if (ready_) {
        UnloadSound(clickSound_);
        ready_ = false;
    }
}

void AudioManager::playClick() {
    if (ready_) PlaySound(clickSound_);
}
