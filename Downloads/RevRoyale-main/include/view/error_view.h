#pragma once
#include <optional>
#include "controller/error.h"

// ---------------------------------------------------------------------------
// ErrorView — pill overlay shown centred above the grid on player mistakes.
// ---------------------------------------------------------------------------
class ErrorView {
public:
    ErrorView();
    void setError(PlayerError error);
    void clearError();
    void render(int x, int y) const;
private:
    std::optional<PlayerError> currentError;
};
