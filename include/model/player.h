class Player {
public:
    Player(int id) : playerId(id) {}

    int id() const { return playerId; }

private:
    int playerId;
};