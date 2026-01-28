class Player {
public:
    Player(int id) : playerId(id) {}

    int getId() const { return playerId; }

private:
    int playerId;
};