#pragma once

#include <string>

/**
 * Errors that can be returned across the controller ↔ model boundary.
 *
 * Returned as optional<PlayerError> from World::applyControllerRequest()
 * and propagated back up through the controller to the view for display.
 * These are player-facing errors — they describe invalid actions the player
 * attempted, not internal model faults (see model/error.h for those).
 *
 *   OUTOFBOUNDS        — the target position is outside the 16×16 grid.
 *   OUTOFTURN          — the action was attempted during another player's turn.
 *   UNITCANTMOVE       — the unit has already acted this turn (moved flag set).
 *   OUTOFREACH         — the target tile is beyond the unit's movement or attack range.
 *   NOTSUPPORTED       — the requested action is not implemented for this context.
 *   INVALIDTARGET      — the selected tile or unit is not a valid target for this action.
 *   UNTRAVERSABLECELL  — the path to the destination passes through impassable terrain.
 *   NOAVAILABLEFOUNDRY — a construction action was attempted but no free foundry exists.
 */
enum class PlayerError {
    OUTOFBOUNDS,
    OUTOFTURN,
    UNITCANTMOVE,
    OUTOFREACH,
    NOTSUPPORTED,
    INVALIDTARGET,
    UNTRAVERSABLECELL,
    NOAVAILABLEFOUNDRY,
    INSUFFICIENTRESOURCES
};

/**
 * Converts a PlayerError to a human-readable string for display in the UI.
 *
 * @param error  The error to convert.
 * @return       A short descriptive string (e.g. "Out of bounds").
 *               Returns "Unknown PlayerError" as a defensive fallback if
 *               the enum is extended without updating this function.
 */
inline std::string playerErrorToString(PlayerError error) {
    switch (error) {
        case PlayerError::OUTOFBOUNDS:         return "Out of bounds";
        case PlayerError::OUTOFTURN:           return "Out of turn";
        case PlayerError::OUTOFREACH:          return "Out of reach";
        case PlayerError::NOTSUPPORTED:        return "Not supported";
        case PlayerError::INVALIDTARGET:       return "Invalid target";
        case PlayerError::UNTRAVERSABLECELL:   return "Untraversable cell";
        case PlayerError::UNITCANTMOVE:        return "Unit cannot move";
        case PlayerError::NOAVAILABLEFOUNDRY:    return "No available foundry";
        case PlayerError::INSUFFICIENTRESOURCES: return "Insufficient resources";
    }

    return "Unknown PlayerError";
}
