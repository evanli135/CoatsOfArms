#pragma once

enum class PlayerError {
    OUTOFBOUNDS,
    OUTOFTURN,
    OUTOFREACH,
    NOTSUPPORTED,
    INVALIDTARGET,
    UNTRAVERSABLECELL
};

inline std::string playerErrorToString(PlayerError error) {
    switch (error) {
        case PlayerError::OUTOFBOUNDS:
            return "Out of bounds";
        case PlayerError::OUTOFTURN:
            return "Out of turn";
        case PlayerError::OUTOFREACH:
            return "Out of reach";
        case PlayerError::NOTSUPPORTED:
            return "Not supported";
        case PlayerError::INVALIDTARGET:
            return "Invalid target";
        case PlayerError::UNTRAVERSABLECELL:
            return "Untraversable cell";
    }

    // Defensive fallback — should never hit unless enum expands
    return "Unknown PlayerError";
}
