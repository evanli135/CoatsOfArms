#pragma once
#include <optional>

#include "include/model/util.h"
#include "include/model/unit.h"

enum class TileModifier {
    ONFIRE
};

enum Terrain {
    GRASS,
    FOREST,
    WATER,
    MOUNTAIN
};


class Tile {
    public:
        // Tile() : terrain(Probability::roll(75.0f) ? GRASS : FOREST) {}
        Tile() : terrain(GRASS) {}
        Tile(Terrain t) : terrain(t) {}

        const std::optional<Unit>& getUnit() const {
            return unit;
        }

        std::optional<Unit>& getUnit() {
            return unit;
        }

        Terrain getTerrain() const {
            return terrain;
        }

        bool hasUnit() const {
            return unit.has_value();
        }

        bool isWalkable() const {
            return terrain != WATER && !unit.has_value();
        }

        void placeUnit(const Unit& u) {
            unit = u;
        }

        void setTerrain(Terrain t) {
            terrain = t;
        }

        const std::optional<Unit> removeUnit() {
            auto u = unit;
            unit.reset();
            return u;
        }
        

private:
    Terrain terrain;
    std::optional<Unit> unit;
};

