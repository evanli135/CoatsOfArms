#pragma once

#include <vector>
#include "include/model/tile.h"

class World {
public:
    World();
    ~World();

    const std::optional<Unit>& getUnitAt(const Position& pos) const {
        return map.at(pos.row).at(pos.col).getUnit();  
    } 

    std::optional<Unit>& getUnitAt(const Position& pos) {
        return map.at(pos.row).at(pos.col).getUnit();  
    }

    moveUnit()


private:
    std::vector<std::vector<Tile>> map;
    
};

namespace Logic {

}