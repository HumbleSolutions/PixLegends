#pragma once

#include <vector>
#include <memory>
#include <string>
#include <random>
#include <SDL2/SDL.h>
#include <unordered_map> // Added for unordered_map

// Forward declarations
class Renderer;
class Texture;
class Object;
class Enemy;
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
    // World dimensions (in tiles)
    int worldWidth = 1000;  // Increased from 50 to 1000 (32,000 pixels)
    int worldHeight = 1000; // Increased from 50 to 1000 (32,000 pixels)
    
    // Chunk system for infinite world
    int chunkSize = 64;     // 64x64 tiles per chunk
    int renderDistance = 3; // Render chunks within 3 chunks of player
    
    // Tile distribution weights
    float stoneWeight = 40.0f;
    float grassWeight = 20.0f;
    float stoneGrassWeight = 2.0f;
    float stoneGrass2Weight = 2.0f;
    float stoneGrass3Weight = 2.0f;
    float stoneGrass4Weight = 2.0f;
    float stoneGrass5Weight = 2.0f;
    float stoneGrass6Weight = 2.0f;
    float stoneGrass7Weight = 2.0f;
    float stoneGrass8Weight = 2.0f;
    float stoneGrass9Weight = 2.0f;
    float stoneGrass10Weight = 2.0f;
    float stoneGrass11Weight = 2.0f;
    float stoneGrass12Weight = 2.0f;
    float stoneGrass13Weight = 2.0f;
    float stoneGrass14Weight = 2.0f;
    float stoneGrass15Weight = 2.0f;
    float stoneGrass16Weight = 2.0f;
    float stoneGrass17Weight = 2.0f;
    float stoneGrass18Weight = 2.0f;
    float stoneGrass19Weight = 2.0f;
    
    // Noise generation
    bool useFixedSeed = false;
    unsigned int fixedSeed = 42;
    bool useNoiseDistribution = true;
    float noiseScale = 0.05f;  // Reduced for larger world
    float noiseThreshold = 0.5f;
    
    // Clustering options
    bool enableStoneClustering = true;
    float stoneClusterChance = 0.3f;
    int stoneClusterRadius = 3;
    
    // Biome system
    bool enableBiomes = true;
    float biomeScale = 0.02f;  // Larger scale for biomes
    
    TileGenerationConfig() = default;
};

// Chunk structure for efficient world management
struct Chunk {
    int chunkX, chunkY;
    std::vector<std::vector<Tile>> tiles;
    std::vector<std::vector<bool>> visibleTiles;
    std::vector<std::vector<bool>> exploredTiles;
    bool isGenerated;
    bool isVisible;
    
    Chunk(int x, int y, int size) : chunkX(x), chunkY(y), isGenerated(false), isVisible(false) {
        tiles.resize(size, std::vector<Tile>(size));
        visibleTiles.resize(size, std::vector<bool>(size, false));
        exploredTiles.resize(size, std::vector<bool>(size, false));
    }
};

class World {
public:
    World();
    explicit World(AssetManager* assetManager);
    ~World() = default;

    // Core functions
    void update(float deltaTime);
    void render(Renderer* renderer);
    void updateEnemies(float deltaTime, float playerX, float playerY);
    
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

    // Enemy management
    void addEnemy(std::unique_ptr<Enemy> enemy);
    const std::vector<std::unique_ptr<Enemy>>& getEnemies() const { return enemies; }
    std::vector<std::unique_ptr<Enemy>>& getEnemies() { return enemies; }

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

    // Chunk management
    void generateChunk(int chunkX, int chunkY);
    void updateVisibleChunks(float playerX, float playerY);
    Chunk* getChunk(int chunkX, int chunkY);
    std::pair<int, int> worldToChunkCoords(int worldX, int worldY) const;
    std::pair<int, int> chunkToWorldCoords(int chunkX, int chunkY) const;

private:
    // Tilemap data
    std::vector<std::vector<Tile>> tiles;
    int width, height;
    int tileSize;
    
    // Chunk system
    std::unordered_map<std::string, std::unique_ptr<Chunk>> chunks;
    std::vector<Chunk*> visibleChunks;
    
    // Objects
    std::vector<std::unique_ptr<Object>> objects;
    // Enemies
    std::vector<std::unique_ptr<Enemy>> enemies;
    
    // Rendering
    Texture* tilesetTexture;
    
    // Asset management
    AssetManager* assetManager;
    
    // Tile textures
    std::vector<Texture*> tileTextures;
    // Edge textures for autotiling (stone with grass edges)
    std::unordered_map<std::string, Texture*> edgeTextures;
    
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
    // Autotiling helpers (tile coordinates, not pixels)
    std::string buildMaskFromNeighbors(int tx, int ty);
    Texture* getEdgeTextureForMask(const std::string& mask);
    bool isGrassAt(int tx, int ty);
    void carveRandomGrassPatches(Chunk* chunk);
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
    float generateNoise(float x, float y) const;
    void applyStoneClustering();
    void validateTileWeights();
    void printTileDistribution();
    
    // Chunk generation
    void generateChunkTiles(Chunk* chunk);
    std::string getChunkKey(int chunkX, int chunkY) const;
    
    // Biome system
    int getBiomeType(int x, int y) const;
    int generateBiomeBasedTile(int x, int y, int biomeType) const;
    
    // Visibility calculation
    void calculateVisibility(float playerX, float playerY);
    bool hasLineOfSight(int startX, int startY, int endX, int endY) const;
    void markTileVisible(int x, int y);
    void markTileExplored(int x, int y);
};
