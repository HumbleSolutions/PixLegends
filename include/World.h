#pragma once

#include <vector>
#include <memory>
#include <string>
#include <random>
#include <SDL2/SDL.h>

// Forward declarations
class Renderer;
class Texture;
class Object;
class AssetManager;

// Tile type constants
enum TileType {
    TILE_GRASS = 0,
    TILE_STONE = 1,
    TILE_STONE_GRASS = 2,
    TILE_STONE_GRASS_2 = 3,
    TILE_STONE_GRASS_3 = 4,
    TILE_STONE_GRASS_4 = 5,
    TILE_STONE_GRASS_5 = 6,
    TILE_STONE_GRASS_6 = 7,
    TILE_STONE_GRASS_7 = 8,
    TILE_STONE_GRASS_8 = 9,
    TILE_STONE_GRASS_9 = 10,
    TILE_STONE_GRASS_10 = 11,
    TILE_STONE_GRASS_11 = 12,
    TILE_STONE_GRASS_12 = 13,
    TILE_STONE_GRASS_13 = 14,
    TILE_STONE_GRASS_14 = 15,
    TILE_STONE_GRASS_15 = 16,
    TILE_STONE_GRASS_16 = 17,
    TILE_STONE_GRASS_17 = 18,
    TILE_STONE_GRASS_18 = 19,
    TILE_STONE_GRASS_19 = 20,
    TILE_WATER = 21
};

struct Tile {
    int id;
    bool walkable;
    bool transparent;
    
    Tile(int id = TILE_GRASS, bool walkable = true, bool transparent = true) 
        : id(id), walkable(walkable), transparent(transparent) {}
};

// Tile generation configuration
struct TileGenerationConfig {
    // World dimensions
    int worldWidth = 100;
    int worldHeight = 100;
    
    // Tile weights (percentages that sum to 100)
    float stoneWeight = 40.0f;         // stone_tile.png - highest priority
    float grassWeight = 20.0f;         // grass_tile.png - lower priority
    float stoneGrassWeight = 2.0f;     // stone_grass_tile.png
    float stoneGrass2Weight = 2.0f;    // stone_grass_tile_2.png
    float stoneGrass3Weight = 2.0f;    // stone_grass_tile_3.png
    float stoneGrass4Weight = 2.0f;    // stone_grass_tile_4.png
    float stoneGrass5Weight = 2.0f;    // stone_grass_tile_5.png
    float stoneGrass6Weight = 2.0f;    // stone_grass_tile_6.png
    float stoneGrass7Weight = 2.0f;    // stone_grass_tile_7.png
    float stoneGrass8Weight = 2.0f;    // stone_grass_tile_8.png
    float stoneGrass9Weight = 2.0f;    // stone_grass_tile_9.png
    float stoneGrass10Weight = 2.0f;   // stone_grass_tile_10.png
    float stoneGrass11Weight = 2.0f;   // stone_grass_tile_11.png
    float stoneGrass12Weight = 2.0f;   // stone_grass_tile_12.png
    float stoneGrass13Weight = 2.0f;   // stone_grass_tile_13.png
    float stoneGrass14Weight = 2.0f;   // stone_grass_tile_14.png
    float stoneGrass15Weight = 2.0f;   // stone_grass_tile_15.png
    float stoneGrass16Weight = 2.0f;   // stone_grass_tile_16.png
    float stoneGrass17Weight = 2.0f;   // stone_grass_tile_17.png
    float stoneGrass18Weight = 2.0f;   // stone_grass_tile_18.png
    float stoneGrass19Weight = 2.0f;   // stone_grass_tile_19.png
    
    // Generation options
    bool useFixedSeed = false;
    unsigned int fixedSeed = 12345;
    bool useNoiseDistribution = false;
    float noiseScale = 0.1f;
    float noiseThreshold = 0.5f;
    
    // Clustering options
    bool enableStoneClustering = true;
    float stoneClusterChance = 0.3f;
    int stoneClusterRadius = 3;
    
    TileGenerationConfig() = default;
};

class World {
public:
    World();
    explicit World(AssetManager* assetManager);
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

    // Asset management
    void setAssetManager(AssetManager* assetManager) { this->assetManager = assetManager; }
    
    // Tilemap generation
    void generateTilemap(const TileGenerationConfig& config = TileGenerationConfig());
    void regenerateTilemap(const TileGenerationConfig& config = TileGenerationConfig());
    void setTileGenerationConfig(const TileGenerationConfig& config) { tileGenConfig = config; }
    const TileGenerationConfig& getTileGenerationConfig() const { return tileGenConfig; }

    // Fog of war and visibility
    void updateVisibility(float playerX, float playerY);
    bool isTileVisible(int x, int y) const;
    void setVisibilityRadius(int radius) { visibilityRadius = radius; }
    int getVisibilityRadius() const { return visibilityRadius; }
    void enableFogOfWar(bool enable) { fogOfWarEnabled = enable; }
    bool isFogOfWarEnabled() const { return fogOfWarEnabled; }

private:
    // Tilemap data
    std::vector<std::vector<Tile>> tiles;
    int width, height;
    int tileSize;
    
    // Objects
    std::vector<std::unique_ptr<Object>> objects;
    
    // Rendering
    Texture* tilesetTexture;
    
    // Asset management
    AssetManager* assetManager;
    
    // Tile textures
    std::vector<Texture*> tileTextures;
    
    // Random number generation for tile placement
    std::mt19937 rng;
    
    // Tile generation configuration
    TileGenerationConfig tileGenConfig;
    
    // Fog of war and visibility
    std::vector<std::vector<bool>> visibleTiles;
    std::vector<std::vector<bool>> exploredTiles;
    int visibilityRadius;
    bool fogOfWarEnabled;
    
    // Helper functions
    void initializeDefaultWorld();
    void loadTileTextures();
    int getPrioritizedTileType(int x, int y);
    bool shouldPlaceStone(int x, int y);
    bool shouldPlaceStoneGrass(int x, int y);
    struct TileColor {
        Uint8 r, g, b, a;
        TileColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255) : r(r), g(g), b(b), a(a) {}
    };
    TileColor getTileColor(int tileId) const;
    
    // New tilemap generation functions
    void initializeRNG();
    int generateWeightedTileType(int x, int y);
    int generateNoiseBasedTileType(int x, int y);
    float generateNoise(float x, float y);
    void applyStoneClustering();
    void validateTileWeights();
    void printTileDistribution();
    
    // Visibility calculation
    void calculateVisibility(float playerX, float playerY);
    bool hasLineOfSight(int startX, int startY, int endX, int endY) const;
    void markTileVisible(int x, int y);
    void markTileExplored(int x, int y);
};
