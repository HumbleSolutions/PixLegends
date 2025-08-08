#pragma once

#include <vector>
#include <memory>
#include <string>

// Forward declarations
class Renderer;
class Texture;
class Object;

struct Tile {
    int id;
    bool walkable;
    bool transparent;
    
    Tile(int id = 0, bool walkable = true, bool transparent = true) 
        : id(id), walkable(walkable), transparent(transparent) {}
};

class World {
public:
    World();
    ~World() = default;

    // Core functions
    void update(float deltaTime);
    void render(Renderer* renderer);
    
    // Tilemap management
    void loadTilemap(const std::string& filename);
    void setTile(int x, int y, int tileId);
    int getTile(int x, int y) const;
    bool isWalkable(int x, int y) const;
    
    // World properties
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int getTileSize() const { return tileSize; }
    
    // Object management
    void addObject(std::unique_ptr<Object> object);
    void removeObject(int x, int y);
    Object* getObjectAt(int x, int y) const;
    const std::vector<std::unique_ptr<Object>>& getObjects() const { return objects; }

private:
    // Tilemap data
    std::vector<std::vector<Tile>> tiles;
    int width, height;
    int tileSize;
    
    // Objects
    std::vector<std::unique_ptr<Object>> objects;
    
    // Rendering
    Texture* tilesetTexture;
    
    // Helper functions
    void initializeDefaultWorld();
    void renderTile(Renderer* renderer, int tileX, int tileY, int tileId);
};
