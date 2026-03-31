#pragma once
#include <optional>
#include <unordered_map>

#include "model/util.h"
#include "model/unit.h"
#include "model/economy.h"
#include "model/city.h"

enum class TileModifier {
    ONFIRE
};

enum class Terrain {
    GRASS,
    FOREST,
    RIVER,
    OCEAN,
    MOUNTAIN,
};


class Tile {
public:
    // Tile() : terrain(Probability::roll(75.0f) ? GRASS : FOREST) {}
    Tile() : terrain(Terrain::GRASS) {}
    Tile(Terrain t) : terrain(t) {}
    Tile(City city) : terrain(Terrain::GRASS), city(city) {}
    Tile(Terrain t, City city) : terrain(t), city(city) {}


    const std::optional<UnitId>& getUnit() const {
        return unit;
    }

    bool hasCity() const {
        return city.has_value();
    }

    const City* getCity() const {
        if (!hasCity()) {
            return nullptr;
        }
        return &city.value();
    }
    


    std::optional<UnitId>& getUnit() {
        return unit;
    }

    Terrain getTerrain() const {
        return terrain;
    }

    bool hasUnit() const {
        return unit.has_value();
    }

    bool isWalkable() const {
        return terrain != Terrain::OCEAN && !unit.has_value();
    }

    void placeUnit(const Unit& u) {
        unit = u.getId();
    }

    void placeUnit(UnitId id) {
        unit = id;
    }

    void setTerrain(Terrain t) {
        terrain = t;
    }

    const std::optional<UnitId> removeUnit() {
        auto u = unit;
        unit.reset();
        return u;
    }

private:
    Terrain terrain;
    std::optional<UnitId> unit;
    std::unordered_map<BuildingType, bool> buildings;
    std::optional<City> city;
};
