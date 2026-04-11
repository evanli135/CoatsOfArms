#pragma once

class Player {
public:
    Player(int id) : playerId(id) {}

    int  getId()   const { return playerId; }

    /** Returns true if this is the null sentinel (no real owner). */
    bool isNull()  const { return playerId == -1; }

    /**
     * Returns the null sentinel player (id = -1).
     * Use instead of nullptr for unowned cities/tiles to avoid null dereferences.
     */
    static const Player& null() {
        static const Player nullPlayer(-1);
        return nullPlayer;
    }

private:
    int playerId;
};
