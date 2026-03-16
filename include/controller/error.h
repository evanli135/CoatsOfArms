#pragma once

enum class PlayerError {
    OUTOFBOUNDS,
    OUTOFTURN,
    UNITCANTMOVE,
    OUTOFREACH,
    NOTSUPPORTED,
    INVALIDTARGET,
    UNTRAVERSABLECELL,
    NOAVAILABLEFOUNDRY
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
        case PlayerError::UNITCANTMOVE:
            return "Unit cannot move";
    }

    // Defensive fallback — should never hit unless enum expands
    return "Unknown PlayerError";
}
