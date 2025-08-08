#include "World.h"
#include "Renderer.h"
#include "AssetManager.h"
#include "Object.h"
#include <iostream>

World::World() : width(50), height(50), tileSize(32), tilesetTexture(nullptr) {
    initializeDefaultWorld();
}

void World::initializeDefaultWorld() {
    // Initialize tile grid
    tiles.resize(height, std::vector<Tile>(width));
    
    // Create a simple grass world
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Simple pattern: grass everywhere
            tiles[y][x] = Tile(0, true, true);
        }
    }
    
    std::cout << "World initialized: " << width << "x" << height << " tiles" << std::endl;
}

void World::update(float deltaTime) {
    // Update all objects
    for (auto& object : objects) {
        object->update(deltaTime);
    }
}

void World::render(Renderer* renderer) {
    if (!renderer) {
        return;
    }
    
    // Get camera position (for now, assume centered on player)
    int cameraX = 0;
    int cameraY = 0;
    
    // Simple rendering: just draw colored rectangles for now
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int tileId = tiles[y][x].id;
            
            // Simple color coding based on tile ID
            SDL_Color color;
            switch (tileId) {
                case 0: // Grass
                    color = {34, 139, 34, 255}; // Forest green
                    break;
                case 1: // Water
                    color = {0, 191, 255, 255}; // Deep sky blue
                    break;
                case 2: // Stone
                    color = {128, 128, 128, 255}; // Gray
                    break;
                default:
                    color = {34, 139, 34, 255}; // Default to grass
                    break;
            }
            
            renderer->renderRect(x * tileSize, y * tileSize, tileSize, tileSize, color, true);
        }
    }
    
    // Render objects
    for (auto& object : objects) {
        object->render(renderer->getSDLRenderer(), cameraX, cameraY, tileSize);
    }
}

void World::loadTilemap(const std::string& filename) {
    // TODO: Implement tilemap loading from file
    std::cout << "Loading tilemap: " << filename << " (not implemented yet)" << std::endl;
}

void World::setTile(int x, int y, int tileId) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        tiles[y][x].id = tileId;
    }
}

int World::getTile(int x, int y) const {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        return tiles[y][x].id;
    }
    return -1; // Invalid tile
}

bool World::isWalkable(int x, int y) const {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        return tiles[y][x].walkable;
    }
    return false;
}

void World::renderTile(Renderer* renderer, int tileX, int tileY, int tileId) {
    // TODO: Implement proper tile rendering with tileset
    // For now, just render colored rectangles
}

void World::addObject(std::unique_ptr<Object> object) {
    if (object) {
        objects.push_back(std::move(object));
    }
}

void World::removeObject(int x, int y) {
    auto it = std::remove_if(objects.begin(), objects.end(),
        [x, y](const std::unique_ptr<Object>& obj) {
            return obj->getX() == x && obj->getY() == y;
        });
    objects.erase(it, objects.end());
}

Object* World::getObjectAt(int x, int y) const {
    for (const auto& object : objects) {
        if (object->getX() == x && object->getY() == y) {
            return object.get();
        }
    }
    return nullptr;
}
