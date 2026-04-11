#pragma once

#include <string>
#include <functional>

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

namespace std {
    template<>
    struct hash<BuildingType> {
        size_t operator()(BuildingType b) const noexcept {
            return std::hash<int>()(static_cast<int>(b));
        }
    };
    template<>
    struct hash<ResourceType> {
        size_t operator()(ResourceType r) const noexcept {
            return std::hash<int>()(static_cast<int>(r));
        }
    };
}

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