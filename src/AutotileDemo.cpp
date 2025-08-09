#include "AutotileDemo.h"
#include "Renderer.h"
#include "AssetManager.h"
#include <algorithm>

AutotileDemo::AutotileDemo(AssetManager* assetManager, int gridWidth, int gridHeight, int tileSize)
    : assetManager(assetManager), gridWidth(gridWidth), gridHeight(gridHeight), tileSize(tileSize) {
    grid.resize(gridWidth * gridHeight, 0);
    loadTextures();
    generateTestGrid();
}

void AutotileDemo::loadTextures() {
    if (!assetManager) return;
    const std::string base = AssetManager::TILESET_PATH; // assets/FULL_Fantasy Forest/Tiles/

    auto load = [&](const std::string& key, const std::string& file) {
        auto tex = assetManager->getTexture(base + file);
        textureByKey[key] = tex ? tex->getTexture() : nullptr;
    };

    // Base tiles
    load("grass_tile", "grass_tile.png");
    load("stone_tile", "stone_tile.png");

    // Edge variants
    load("stone_grass_NONE", "stone_grass_NONE.png");
    load("stone_grass_T", "stone_grass_T.png");
    load("stone_grass_B", "stone_grass_B.png");
    load("stone_grass_L", "stone_grass_L.png");
    load("stone_grass_TR", "stone_grass_TR.png");
    load("stone_grass_TL", "stone_grass_TL.png");
    load("stone_grass_BR", "stone_grass_BR.png");
    load("stone_grass_BL", "stone_grass_BL.png");
    load("stone_grass_LR", "stone_grass_LR.png");
    load("stone_grass_TB", "stone_grass_TB.png");
    load("stone_grass_TBL", "stone_grass_TBL.png");
    load("stone_grass_TBR", "stone_grass_TBR.png");
    load("stone_grass_TLR", "stone_grass_TLR.png");
    load("stone_grass_BLR", "stone_grass_BLR.png");
    load("stone_grass_ALL", "stone_grass_ALL.png");
}

void AutotileDemo::generateTestGrid() {
    // Start with all stone
    std::fill(grid.begin(), grid.end(), 0);

    // Place random square grass patches between 1x1 and 9x9
    std::mt19937 rng(1337);
    std::uniform_int_distribution<int> sizeDist(1, 9);
    std::uniform_int_distribution<int> patchCountDist(6, 12);

    const int targetPatches = patchCountDist(rng);
    int placed = 0;
    int attempts = 0;
    const int maxAttempts = 200;

    while (placed < targetPatches && attempts++ < maxAttempts) {
        int s = sizeDist(rng);
        s = std::min(s, std::min(gridWidth, gridHeight));
        if (s <= 0) continue;

        std::uniform_int_distribution<int> xDist(0, std::max(0, gridWidth - s));
        std::uniform_int_distribution<int> yDist(0, std::max(0, gridHeight - s));
        int ox = xDist(rng);
        int oy = yDist(rng);

        for (int y = oy; y < oy + s; ++y) {
            for (int x = ox; x < ox + s; ++x) {
                grid[y * gridWidth + x] = 1; // grass
            }
        }
        placed++;
    }
}

std::string AutotileDemo::buildMaskFromNeighbors(int gx, int gy) const {
    // Only used when center is stone. Check N/S/W/E for grass.
    const auto inBounds = [&](int x, int y) {
        return x >= 0 && x < gridWidth && y >= 0 && y < gridHeight;
    };
    unsigned int mask = 0;
    if (inBounds(gx, gy - 1) && grid[(gy - 1) * gridWidth + gx] == 1) mask |= AUTOTILE_TOP;
    if (inBounds(gx, gy + 1) && grid[(gy + 1) * gridWidth + gx] == 1) mask |= AUTOTILE_BOTTOM;
    if (inBounds(gx - 1, gy) && grid[gy * gridWidth + (gx - 1)] == 1) mask |= AUTOTILE_LEFT;
    if (inBounds(gx + 1, gy) && grid[gy * gridWidth + (gx + 1)] == 1) mask |= AUTOTILE_RIGHT;

    if (mask == 0) return ""; // pure stone
    if (mask == (AUTOTILE_TOP | AUTOTILE_BOTTOM | AUTOTILE_LEFT | AUTOTILE_RIGHT)) return "ALL";

    std::string s;
    if (mask & AUTOTILE_TOP) s += 'T';
    if (mask & AUTOTILE_BOTTOM) s += 'B';
    if (mask & AUTOTILE_LEFT) s += 'L';
    if (mask & AUTOTILE_RIGHT) s += 'R';
    return s;
}

SDL_Texture* AutotileDemo::getTileTexture(bool isGrass, const std::string& maskKey) const {
    if (isGrass) {
        auto it = textureByKey.find("grass_tile");
        return it != textureByKey.end() ? it->second : nullptr;
    }

    // Stone selection
    if (maskKey.empty()) {
        auto it = textureByKey.find("stone_tile");
        return it != textureByKey.end() ? it->second : nullptr;
    }

    std::string key = "stone_grass_" + maskKey;

    auto it = textureByKey.find(key);
    if (it != textureByKey.end() && it->second) return it->second;

    // Conservative fallback: plain stone, not ALL, to avoid incorrect center grass
    auto itStone = textureByKey.find("stone_tile");
    return itStone != textureByKey.end() ? itStone->second : nullptr;
}

void AutotileDemo::render(Renderer* renderer) {
    if (!renderer) return;

    for (int y = 0; y < gridHeight; ++y) {
        for (int x = 0; x < gridWidth; ++x) {
            const int cell = grid[y * gridWidth + x];
            const bool isGrass = (cell == 1);
            std::string mask;
            if (!isGrass) {
                mask = buildMaskFromNeighbors(x, y);
            }

            SDL_Rect dst{ x * tileSize, y * tileSize, tileSize, tileSize };

            if (!isGrass && mask == "R") {
                // Use left-edge graphic flipped horizontally if there is no dedicated R asset
                auto it = textureByKey.find("stone_grass_L");
                if (it != textureByKey.end() && it->second) {
                    SDL_SetTextureColorMod(it->second, 255, 255, 255);
                    SDL_RenderCopyEx(renderer->getSDLRenderer(), it->second, nullptr, &dst, 0.0, nullptr, SDL_FLIP_HORIZONTAL);
                    continue;
                }
            }

            if (!isGrass && mask == "T") {
                // If a dedicated T texture is missing, rotate B by 180 degrees
                auto itB = textureByKey.find("stone_grass_B");
                auto itT = textureByKey.find("stone_grass_T");
                if ((itT == textureByKey.end() || itT->second == nullptr) && itB != textureByKey.end() && itB->second) {
                    SDL_SetTextureColorMod(itB->second, 255, 255, 255);
                    SDL_RenderCopyEx(renderer->getSDLRenderer(), itB->second, nullptr, &dst, 180.0, nullptr, SDL_FLIP_NONE);
                    continue;
                }
            }

            SDL_Texture* tex = getTileTexture(isGrass, mask);
            if (tex) {
                SDL_SetTextureColorMod(tex, 255, 255, 255);
            }
            renderer->renderTexture(tex, nullptr, &dst);
        }
    }
}


