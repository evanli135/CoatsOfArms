#include "model/world.h"
#include "model/tile.h"
#include "controller/error.h"
#include "model/error.h"
#include "model/unit.h"

#include <queue>
#include <limits>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cassert>

using std::vector, std::unordered_map;

// Helper struct for priority queue
struct PathNode {
    float cost;
    Position position;
    
    bool operator>(const PathNode& other) const {
        return cost > other.cost;
    }
};

// Get all adjacent positions
vector<Position> MovementSystem::getNeighbors(Position pos) const {
    vector<Position> neighbors;
    
    if (pos.row() > 0) 
        neighbors.push_back(Position(pos.row() - 1, pos.col()));
    if (pos.row() < Game::HEIGHT - 1) 
        neighbors.push_back(Position(pos.row() + 1, pos.col()));
    if (pos.col() > 0) 
        neighbors.push_back(Position(pos.row(), pos.col() - 1));
    if (pos.col() < Game::WIDTH - 1) 
        neighbors.push_back(Position(pos.row(), pos.col() + 1));
    
    return neighbors;
}

// Check if we can move through this tile
bool MovementSystem::canMoveThroughTile(Position origin, Position pos, Position destination) const {
    const Tile& tile = world.getTileAt(pos);
    
    if (!tile.isWalkable()) {
        return false;
    }
    
    // Allow destination even if occupied (for attacking)
    if (pos == destination) {
        return true;
    }
    
    if (tile.hasUnit()) {
        const Unit* blockingUnit = world.getUnitAt(pos);
        assert(blockingUnit != nullptr);
        
        const Unit* movingUnit = world.getUnitAt(origin);
        assert(movingUnit != nullptr);
        
        // Can move through friendly units
        if (blockingUnit->sameOwner(*movingUnit)) {
            return true;
        }
        
        return false;  // Can't move through enemies
    }
    
    return true;
}

// Calculate terrain cost
float MovementSystem::stepCost(const Unit& unit, const Tile& tile) const {
    switch (tile.getTerrain()) {
        case Terrain::GRASS:
            return 1.0f;
        case Terrain::FOREST:
            return 1.5f;
        case Terrain::RIVER:
            return 2.5f;
        case Terrain::OCEAN:
            return 999.0f;
        case Terrain::MOUNTAIN:
            return 2.0f;
        default:
            throw InternalError::FATAL;
    }
}

// Find shortest path cost using Dijkstra
float MovementSystem::shortestPath(Position origin, Position destination) const {
    if (origin == destination) {
        return 0.0f;
    }
    
    const Unit* unit = world.getUnitAt(origin);
    if (!unit) {
        return std::numeric_limits<float>::infinity();
    }
    
    // Priority queue for exploring positions
    std::priority_queue<PathNode, vector<PathNode>, std::greater<PathNode>> pqueue;
    
    // Track best cost to reach each position - STORE BY VALUE, NOT POINTER
    unordered_map<Position, float> bestCosts;
    
    // Start at origin with cost 0
    pqueue.push({0.0f, origin});
    bestCosts[origin] = 0.0f;  // No pointer, just Position
    
    while (!pqueue.empty()) {
        PathNode current = pqueue.top();
        pqueue.pop();
        
        // Found destination
        if (current.position == destination) {
            return current.cost;
        }
        
        // Skip if we already found a better path
        if (bestCosts[current.position] < current.cost) {
            continue;
        }
        
        // Explore neighbors
        vector<Position> neighbors = getNeighbors(current.position);
        
        for (const Position& neighbor : neighbors) {
            // Skip if can't move through
            if (!canMoveThroughTile(origin, neighbor, destination)) {
                continue;
            }
            
            // Calculate cost to reach neighbor
            float moveCost = stepCost(*unit, world.getTileAt(neighbor));
            float totalCost = current.cost + moveCost;
            
            // Check if this is a better path
            bool hasNoCost = (bestCosts.find(neighbor) == bestCosts.end());
            bool foundBetterPath = !hasNoCost && (totalCost < bestCosts[neighbor]);
            
            if (hasNoCost || foundBetterPath) {
                bestCosts[neighbor] = totalCost;
                pqueue.push({totalCost, neighbor});
            }
        }
    }
    
    // Couldn't reach destination
    return std::numeric_limits<float>::infinity();
}

// Check if unit can move to destination
bool MovementSystem::canMove(Position origin, Position destination) const {
    // Basic checks
    if (!world.getTileAt(destination).isWalkable()) {
        return false;
    }
    
    if (world.hasUnitAt(destination)) {
        return false;
    }
    
    const Unit* unit = world.getUnitAt(origin);
    if (!unit || !unit->canMove()) {
        return false;
    }
    
    // Calculate path cost
    float pathCost = shortestPath(origin, destination);
    
    // Check if within movement range
    return pathCost <= unit->getMovement();
}

std::optional<PlayerError> MovementSystem::move(Position origin, Position destination) {
    assert(world.hasUnitAt(origin));  // Should be guaranteed by controller

    if (!canMove(origin, destination)) {
        return PlayerError::UNITCANTMOVE;
    }

    auto unitId = world.getTileAt(origin).removeUnit();

    if (unitId.has_value()) {
        world.getTileAt(destination).placeUnit(unitId.value());
        world.getUnitAt(destination)->setMoved(true);
        return std::nullopt;
    } else {
        throw std::logic_error("FATAL INTERNAL: No unit at the source position");
    }
}

// Returns the tile sequence of the shortest path from origin to destination.
// Returns an empty vector if no path exists.
vector<Position> MovementSystem::getPath(Position origin, Position destination) const {
    if (origin == destination) return {origin};

    const Unit* unit = world.getUnitAt(origin);
    if (!unit) return {};

    std::priority_queue<PathNode, vector<PathNode>, std::greater<PathNode>> pqueue;
    unordered_map<Position, float>    bestCosts;
    unordered_map<Position, Position> predecessor;

    pqueue.push({0.0f, origin});
    bestCosts[origin] = 0.0f;

    while (!pqueue.empty()) {
        PathNode current = pqueue.top();
        pqueue.pop();

        if (current.position == destination) break;
        if (bestCosts.count(current.position) && bestCosts[current.position] < current.cost) continue;

        for (const Position& neighbor : getNeighbors(current.position)) {
            if (!canMoveThroughTile(origin, neighbor, destination)) continue;

            float totalCost = current.cost + stepCost(*unit, world.getTileAt(neighbor));
            if (!bestCosts.count(neighbor) || totalCost < bestCosts[neighbor]) {
                bestCosts[neighbor] = totalCost;
                predecessor.insert_or_assign(neighbor, current.position);
                pqueue.push({totalCost, neighbor});
            }
        }
    }

    if (!predecessor.count(destination)) return {};

    vector<Position> path;
    Position cur = destination;
    while (cur != origin) {
        path.push_back(cur);
        cur = predecessor.at(cur);
    }
    path.push_back(origin);
    std::reverse(path.begin(), path.end());
    return path;
}

vector<Position> MovementSystem::getMovementSnapshot(Position origin) const {
    vector<Position> reachablePositions;

    const Unit* unit = world.getUnitAt(origin);
    if (!unit || !unit->canMove()) {
        return reachablePositions;
    }

    // Only scan within the potential range
    int potentialRange = unit->getMovement();

    int leftBound = std::max(0, origin.col() - potentialRange);
    int rightBound = std::min(Game::WIDTH - 1, origin.col() + potentialRange);
    
    int lowerBound = std::max(0, origin.row() - potentialRange);
    int upperBound = std::min(Game::HEIGHT - 1, origin.row() + potentialRange);

    for (int row = lowerBound; row <= upperBound; row++) {
        for (int col = leftBound; col <= rightBound; col++) {
            Position pos(row, col);
            if (canMove(origin, pos)) {
                reachablePositions.push_back(pos);
            }
        }
    }

    return reachablePositions;
}