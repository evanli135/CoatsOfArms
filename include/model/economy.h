#pragma once

#include <string>

enum class ResourceType {
    FOOD,
    WOOD,
    STONE,
    METAL
};

enum class BuildingType {
    FOUNDRY,
    BARRACK,
    EXTRACTOR,
    SHRINE,
    UTILITY
};

class Building {
public:
    Building(BuildingType type);
    const std::string& getName() const;
    BuildingType getType() const;

    int constructionCost() const;
    int constructionCompletion() const;

    void addConstruction(int cost);

    bool isLocal() const {     return type == BuildingType::EXTRACTOR || type == BuildingType::UTILITY; }

private:
    std::string name;
    BuildingType type;
};

class BuildingFactory {
public:
    static Building createBuilding(BuildingType type) {
        switch (type) {
            case BuildingType::FOUNDRY:
                return Building(BuildingType::FOUNDRY);
            case BuildingType::BARRACK:
                return Building(BuildingType::BARRACK);
            case BuildingType::EXTRACTOR:
                return Building(BuildingType::EXTRACTOR);
            case BuildingType::SHRINE:
                return Building(BuildingType::SHRINE);
            case BuildingType::UTILITY:
                return Building(BuildingType::UTILITY);
        }
    
    }
};