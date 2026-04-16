#pragma once
#include "raylib.h"

// Procedurally-synthesised click sound for UI buttons.
// Call init() after InitAudioDevice(), shutdown() before CloseAudioDevice().
class AudioManager {
public:
    static AudioManager& get();

    void init();
    void shutdown();
    void playClick();

private:
    AudioManager() = default;
    Sound clickSound_{};
    bool  ready_ = false;
};
