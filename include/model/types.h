#include <stdexcept>

namespace Game {
    inline constexpr int MAP_WIDTH = 16;
    inline constexpr int MAP_HEIGHT = 16;
}

struct Position {
    int row, col;
    
    Position(int r, int c) : row(r), col(c) {
        if (r < 0 || r >= Game::MAP_HEIGHT || c < 0 || c >= Game::MAP_WIDTH) {
            throw std::out_of_range("Position out of bounds");
        }
    }
};

enum Terrain {
    GRASS,
    WATER,
    MOUNTAIN
};