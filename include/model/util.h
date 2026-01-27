#include <stdexcept>

namespace Game {
    inline constexpr int WIDTH = 16;
    inline constexpr int HEIGHT = 16;
}

/**
 * Position is garunteed to be within the bounds of the game world.
 */
struct Position {
private:
    int row_;
    int col_;

    static void check(int r, int c) {
        if (r < 0 || r >= Game::HEIGHT ||
            c < 0 || c >= Game::WIDTH) {
            throw std::out_of_range("Position out of bounds");
        }
    }

public:
    // Constructor
    Position(int r, int c) : row_(r), col_(c) {
        check(r, c);
    }

    // Accessors
    int row() const noexcept { return row_; }
    int col() const noexcept { return col_; }

    // Setters (absolute)
    void setRow(int r) {
        check(r, col_);
        row_ = r;
    }

    void setCol(int c) {
        check(row_, c);
        col_ = c;
    }

    void set(int r, int c) {
        check(r, c);
        row_ = r;
        col_ = c;
    }

    // Changers (relative)
    void changeRow(int delta) {
        check(row_ + delta, col_);
        row_ += delta;
    }

    void changeCol(int delta) {
        check(row_, col_ + delta);
        col_ += delta;
    }

    void move(int dRow, int dCol) {
        check(row_ + dRow, col_ + dCol);
        row_ += dRow;
        col_ += dCol;
    }
};
