#include "view/error_view.h"
#include "raylib.h"

ErrorView::ErrorView() : currentError(std::nullopt) {}

void ErrorView::setError(PlayerError error) { currentError = error; }
void ErrorView::clearError()                { currentError = std::nullopt; }

void ErrorView::render(int x, int y) const {
    if (!currentError) return;
    DrawRectangle(x-6, y-4, 214, 36, Color{80, 10, 10, 200});
    DrawRectangleLines(x-6, y-4, 214, 36, Color{200, 60, 60, 255});
    DrawText(playerErrorToString(*currentError).c_str(), x, y+6, 15, Color{255, 160, 160, 255});
}
