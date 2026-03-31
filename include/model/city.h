#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>

#include "model/player.h"
#include "model/economy.h"

using std::vector, std::string, std::unordered_map;

const int MAX_LEVEL = 5;

class City {
public:
    City(const string& name, int buildingCapacity)
        : name(name), buildingCapacity(buildingCapacity),
          upgradeLevel(0), level(0),
          owner(&Player::null()) {}

    void addBuilding(BuildingType type) {
        if (buildings.size() >= (size_t)buildingCapacity) {
            throw std::logic_error("City has reached building capacity");
        }
        buildings[type]++;
    }

    void cancelConstruction(BuildingType type) {
        auto it = buildings.find(type);
        if (it != buildings.end() && it->second > 0) {
            it->second--;
        } else {
            throw std::logic_error("No such building under construction");
        }
    }

    /** Returns true if this city has a real (non-null) owner. */
    bool hasOwner() const { return !owner->isNull(); }

    /**
     * Returns the owning player.
     * Always safe — if no owner has been set, returns Player::null().
     */
    const Player& getOwner() const { return *owner; }

    /**
     * Sets the owner. Pass nullptr to reset to the null sentinel.
     */
    void setOwner(Player* player) {
        owner = (player && !player->isNull()) ? player : &Player::null();
    }

    void upgrade() {
        if (upgradeLevel >= MAX_LEVEL) {
            throw std::logic_error("City has reached max upgrade level");
        }
        upgradeLevel++;
    }

private:
    string name;
    unordered_map<BuildingType, int> buildings;
    const Player* owner;
    int buildingCapacity;
    int upgradeLevel;
    int level;
};
