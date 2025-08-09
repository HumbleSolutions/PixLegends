#pragma once

#include <SDL2/SDL.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <random>

class Renderer;
class AssetManager;

// Side bitmask
enum AutotileSide : unsigned int {
    AUTOTILE_TOP    = 1u << 0,
    AUTOTILE_BOTTOM = 1u << 1,
    AUTOTILE_LEFT   = 1u << 2,
    AUTOTILE_RIGHT  = 1u << 3
};

class AutotileDemo {
public:
    AutotileDemo(AssetManager* assetManager, int gridWidth = 20, int gridHeight = 12, int tileSize = 32);
    ~AutotileDemo() = default;

    void render(Renderer* renderer);

private:
    AssetManager* assetManager;
    int gridWidth;
    int gridHeight;
    int tileSize;

    // 0 = stone, 1 = grass
    std::vector<int> grid;

    // Textures map
    std::unordered_map<std::string, SDL_Texture*> textureByKey;

    void loadTextures();
    void generateTestGrid();

    // Helpers
    std::string buildMaskFromNeighbors(int gx, int gy) const; // returns e.g. "T", "LR", "TBL", "ALL", ""
    SDL_Texture* getTileTexture(bool isGrass, const std::string& maskKey) const;
};


