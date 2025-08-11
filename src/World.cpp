#include "World.h"
#include "Renderer.h"
#include "AssetManager.h"
#include "Object.h"
#include "Enemy.h"
#include <iostream>
#include <random>
#include <cmath> // Required for sin and cos
#include <algorithm> // Required for std::max
#include <cstdio>
#include <cstdlib> // Required for std::abs

World::World() : width(1000), height(1000), tileSize(32), tilesetTexture(nullptr), assetManager(nullptr), rng(std::random_device{}()), visibilityRadius(30), fogOfWarEnabled(true) {
    // Initialize default tile generation config
    tileGenConfig.worldWidth = width;
    tileGenConfig.worldHeight = height;
    initializeDefaultWorld();
}

World::World(AssetManager* assetManager) : width(1000), height(1000), tileSize(32), tilesetTexture(nullptr), assetManager(assetManager), rng(std::random_device{}()), visibilityRadius(30), fogOfWarEnabled(true) {
    // Initialize default tile generation config
    tileGenConfig.worldWidth = width;
    tileGenConfig.worldHeight = height;
    loadTileTextures();
    initializeDefaultWorld();
}

void World::initializeDefaultWorld() {
    // Initialize chunk system instead of full tile grid
    std::cout << "Initializing chunk-based world: " << width << "x" << height << " tiles (" 
              << (width * tileSize) << "x" << (height * tileSize) << " pixels)" << std::endl;
    std::cout << "Chunk size: " << tileGenConfig.chunkSize << "x" << tileGenConfig.chunkSize << " tiles" << std::endl;
    
    // Initialize visibility arrays for the entire world (for fog of war)
    visibleTiles.resize(height, std::vector<bool>(width, false));
    exploredTiles.resize(height, std::vector<bool>(width, false));
    
    // Use the new tilemap generation system with default configuration
    TileGenerationConfig defaultConfig;
    defaultConfig.worldWidth = width;
    defaultConfig.worldHeight = height;
    defaultConfig.chunkSize = 64;
    defaultConfig.renderDistance = 3; // Render current chunk and neighbors (reduces edge void)
    // New stone-first priority system
    defaultConfig.stoneWeight = 40.0f;         // stone_tile.png - highest priority
    defaultConfig.grassWeight = 20.0f;         // grass_tile.png - lower priority
    defaultConfig.stoneGrassWeight = 2.0f;     // stone_grass_tile.png
    defaultConfig.stoneGrass2Weight = 2.0f;    // stone_grass_tile_2.png
    defaultConfig.stoneGrass3Weight = 2.0f;    // stone_grass_tile_3.png
    defaultConfig.stoneGrass4Weight = 2.0f;    // stone_grass_tile_4.png
    defaultConfig.stoneGrass5Weight = 2.0f;    // stone_grass_tile_5.png
    defaultConfig.stoneGrass6Weight = 2.0f;    // stone_grass_tile_6.png
    defaultConfig.stoneGrass7Weight = 2.0f;    // stone_grass_tile_7.png
    defaultConfig.stoneGrass8Weight = 2.0f;    // stone_grass_tile_8.png
    defaultConfig.stoneGrass9Weight = 2.0f;    // stone_grass_tile_9.png
    defaultConfig.stoneGrass10Weight = 2.0f;   // stone_grass_tile_10.png
    defaultConfig.stoneGrass11Weight = 2.0f;   // stone_grass_tile_11.png
    defaultConfig.stoneGrass12Weight = 2.0f;   // stone_grass_tile_12.png
    defaultConfig.stoneGrass13Weight = 2.0f;   // stone_grass_tile_13.png
    defaultConfig.stoneGrass14Weight = 2.0f;   // stone_grass_tile_14.png
    defaultConfig.stoneGrass15Weight = 2.0f;   // stone_grass_tile_15.png
    defaultConfig.stoneGrass16Weight = 2.0f;   // stone_grass_tile_16.png
    defaultConfig.stoneGrass17Weight = 2.0f;   // stone_grass_tile_17.png
    defaultConfig.stoneGrass18Weight = 2.0f;   // stone_grass_tile_18.png
    defaultConfig.stoneGrass19Weight = 2.0f;   // stone_grass_tile_19.png
    defaultConfig.useFixedSeed = false;
    defaultConfig.useNoiseDistribution = true;
    defaultConfig.noiseScale = 0.05f;
    defaultConfig.enableStoneClustering = true;
    defaultConfig.enableBiomes = true;
    defaultConfig.biomeScale = 0.02f;
    
    generateTilemap(defaultConfig);
    
    std::cout << "Chunk-based world initialized successfully!" << std::endl;
    std::cout << "Fog of war enabled with visibility radius: " << visibilityRadius << " tiles" << std::endl;
}

void World::update(float deltaTime) {
    // Update all objects
    for (auto& object : objects) {
        object->update(deltaTime);
    }
}

void World::updateEnemies(float deltaTime, float playerX, float playerY) {
    for (auto& enemy : enemies) {
        if (enemy) enemy->update(deltaTime, playerX, playerY);
    }
}

void World::render(Renderer* renderer) {
    if (!renderer) {
        return;
    }
    
    // Get camera position from renderer
    int cameraX, cameraY;
    renderer->getCamera(cameraX, cameraY);
    
    // Debug: Count rendered chunks (only once every 300 frames to reduce spam)
    static int debugFrameCount = 0;
    if (debugFrameCount++ % 300 == 0) {
        std::cout << "Rendering " << visibleChunks.size() << " visible chunks" << std::endl;
    }
    
    // Render only visible chunks
    int renderedTiles = 0;
    int visibleTilesCount = 0;
    
    for (Chunk* chunk : visibleChunks) {
        if (!chunk || !chunk->isGenerated) continue;
        
        int chunkSize = tileGenConfig.chunkSize;
        int worldStartX = chunk->chunkX * chunkSize;
        int worldStartY = chunk->chunkY * chunkSize;
        
        for (int y = 0; y < chunkSize; y++) {
            for (int x = 0; x < chunkSize; x++) {
                int worldX = worldStartX + x;
                int worldY = worldStartY + y;
                
                // Determine if tile is within the finite world arrays
                const bool inWorldBounds = (worldX >= 0 && worldX < width && worldY >= 0 && worldY < height);
                
                int tileId = chunk->tiles[y][x].id;
                
                // Validate tile ID to catch any invalid values
                if (tileId < 0 || tileId > TILE_LAST) {
                    std::cout << "ERROR: Invalid tile ID " << tileId << " at position (" << worldX << ", " << worldY << "). Setting to grass." << std::endl;
                    tileId = TILE_GRASS;
                    chunk->tiles[y][x].id = TILE_GRASS;
                }
                
                    // Check visibility - render all tiles, but apply fog of war effect if enabled
                bool isVisible = true;
                bool isExplored = true;
                
                if (fogOfWarEnabled) {
                    if (inWorldBounds) {
                        isVisible = visibleTiles[worldY][worldX];
                        isExplored = exploredTiles[worldY][worldX];
                    } else {
                        // For tiles outside the finite world arrays (negative/overflow), render normally
                        isVisible = true;
                        isExplored = true;
                    }
                }
                
                // Calculate world position
                int worldPosX = worldX * tileSize;
                int worldPosY = worldY * tileSize;
                
                // Calculate screen position (with camera offset and zoom)
                float z = renderer->getZoom();
                // Use edge-difference scaling to avoid per-tile rounding gaps
                auto scaledEdge = [cameraX, cameraY, z](int wx, int wy) -> SDL_Point {
                    float sx = (static_cast<float>(wx - cameraX)) * z;
                    float sy = (static_cast<float>(wy - cameraY)) * z;
                    return SDL_Point{ static_cast<int>(std::floor(sx)), static_cast<int>(std::floor(sy)) };
                };
                SDL_Point tl = scaledEdge(worldPosX, worldPosY);
                SDL_Point br = scaledEdge(worldPosX + tileSize, worldPosY + tileSize);
                int screenX = tl.x;
                int screenY = tl.y;
                
                // Check if the tile is visible on screen (with generous margin to prevent black areas)
                if (screenX + tileSize > -200 && screenX < 1920 + 200 && 
                    screenY + tileSize > -200 && screenY < 1080 + 200) {
                    
                    renderedTiles++;
                    if (isVisible) {
                        visibleTilesCount++;
                    }
                    
                    // Variant-based rendering per material type
                    SDL_Rect destRect = { screenX, screenY, std::max(1, br.x - tl.x), std::max(1, br.y - tl.y) };

                    // Special-cases: animated sprite sheets for deep water and lava
                    if (tileId == TILE_WATER_DEEP && deepWaterSpriteSheet && deepWaterSpriteSheet->getTexture()) {
                        int total = std::max(1, deepWaterSpriteSheet->getTotalFrames());
                        // Time-based animation with a slight per-tile phase offset for variety
                        Uint32 ticks = SDL_GetTicks();
                        int frame = static_cast<int>((ticks / 150) % total); // global sync
                        SDL_Rect src = deepWaterSpriteSheet->getFrameRect(frame);
                        SDL_Texture* sdlTex = deepWaterSpriteSheet->getTexture()->getTexture();
                        SDL_RenderCopy(renderer->getSDLRenderer(), sdlTex, &src, &destRect);
                        continue;
                    }
                    if (tileId == TILE_LAVA && lavaSpriteSheet && lavaSpriteSheet->getTexture()) {
                        int total = std::max(1, lavaSpriteSheet->getTotalFrames());
                        Uint32 ticks = SDL_GetTicks();
                        int frame = static_cast<int>((ticks / 120) % total); // global sync
                        SDL_Rect src = lavaSpriteSheet->getFrameRect(frame);
                        SDL_Texture* sdlTex = lavaSpriteSheet->getTexture()->getTexture();
                        SDL_RenderCopy(renderer->getSDLRenderer(), sdlTex, &src, &destRect);
                        continue;
                    }

                    Texture* chosen = nullptr;
                    if (tileId >= 0 && tileId < static_cast<int>(tileVariantTextures.size()) && !tileVariantTextures[tileId].empty()) {
                        size_t idx = static_cast<size_t>(getPreferredVariantIndex(tileId, worldX, worldY));
                        idx = std::min(idx, tileVariantTextures[tileId].size() - 1);
                        chosen = tileVariantTextures[tileId][idx];
                    } else if (tileId >= 0 && tileId < static_cast<int>(tileTextures.size())) {
                        chosen = tileTextures[tileId];
                    }
                    SDL_Texture* sdlTex = chosen ? chosen->getTexture() : nullptr;
                    if (sdlTex) {
                        SDL_SetTextureBlendMode(sdlTex, SDL_BLENDMODE_BLEND);
                        if (fogOfWarEnabled && isExplored && !isVisible) {
                            SDL_SetTextureColorMod(sdlTex, 100, 100, 100);
                        } else if (fogOfWarEnabled && !isExplored) {
                            SDL_SetTextureColorMod(sdlTex, 50, 50, 50);
                        } else {
                            SDL_SetTextureColorMod(sdlTex, 255, 255, 255);
                        }
                        SDL_RenderCopy(renderer->getSDLRenderer(), sdlTex, nullptr, &destRect);
                    }
                }
            }
        }
    }
    
    // Debug output (only print once every 300 frames to avoid spam)
    if (debugFrameCount % 300 == 0) {
        std::cout << "Rendered tiles: " << renderedTiles << " from " << visibleChunks.size() << " chunks" << std::endl;
        if (fogOfWarEnabled) {
            std::cout << "Visible tiles: " << visibleTilesCount << " out of " << renderedTiles << " rendered tiles" << std::endl;
        }
    }
    
    // Render objects (only those in visible area) - objects stay anchored to world positions
    for (auto& object : objects) {
        int objX = object->getX();
        int objY = object->getY();
        
        // Calculate object world position in pixels
        int objWorldX = objX * tileSize;
        int objWorldY = objY * tileSize;
        
        // Calculate object screen position
        int objScreenX = objWorldX - cameraX;
        int objScreenY = objWorldY - cameraY;
        
        // Check if object is visible on screen (with generous margin)
        if (objScreenX + tileSize > -200 && objScreenX < 1920 + 200 && 
            objScreenY + tileSize > -200 && objScreenY < 1080 + 200) {
            object->render(renderer->getSDLRenderer(), cameraX, cameraY, tileSize, renderer->getZoom());
        }
    }

    // Render enemies (screen-space culling)
    for (auto& enemy : enemies) {
        if (!enemy) continue;
        float z = renderer->getZoom();
        int screenX = static_cast<int>((static_cast<int>(enemy->getX()) - cameraX) * z);
        int screenY = static_cast<int>((static_cast<int>(enemy->getY()) - cameraY) * z);
        int cullW = static_cast<int>(512 * z);
        int cullH = static_cast<int>(512 * z);
        if (screenX + cullW > -200 && screenX < 1920 + 200 &&
            screenY + cullH > -200 && screenY < 1080 + 200) {
            enemy->render(renderer);
            enemy->renderProjectiles(renderer);
        }
    }
}

void World::renderMinimap(Renderer* renderer, int x, int y, int width, int height, float playerX, float playerY) const {
    if (!renderer || width <= 0 || height <= 0) return;
    SDL_Renderer* sdl = renderer->getSDLRenderer();
    if (!sdl) return;

    SDL_SetRenderDrawBlendMode(sdl, SDL_BLENDMODE_BLEND);
    // No external border; minimap will be embedded within HUD frame
    // Inner drawing area (fit inside border)
    const int margin = 8;
    int ix = x + margin, iy = y + margin;
    int iw = std::max(1, width - margin * 2);
    int ih = std::max(1, height - margin * 2);
    SDL_Rect inner{ ix, iy, iw, ih };
    SDL_SetRenderDrawColor(sdl, 0, 0, 0, 200);
    SDL_RenderFillRect(sdl, &inner);

    // Center the view around the player
    int centerTileX = static_cast<int>(playerX / tileSize);
    int centerTileY = static_cast<int>(playerY / tileSize);
    // Define a square region to sample that maps to minimap pixels
    // Show a fixed span around the player for readable minimap
    const float spanTilesX = 200.0f;
    const float spanTilesY = 200.0f;
    const float halfSpanX = spanTilesX * 0.5f;
    const float halfSpanY = spanTilesY * 0.5f;

    for (int py = 0; py < ih; ++py) {
        for (int px = 0; px < iw; ++px) {
            // World tile being sampled for this pixel, centered on player
            float tfx = centerTileX - halfSpanX + (px + 0.5f) * (spanTilesX / iw);
            float tfy = centerTileY - halfSpanY + (py + 0.5f) * (spanTilesY / ih);
            int tx = std::max(0, std::min(this->width - 1, static_cast<int>(tfx)));
            int ty = std::max(0, std::min(this->height - 1, static_cast<int>(tfy)));
            tx = std::max(0, std::min(this->width - 1, tx));
            ty = std::max(0, std::min(this->height - 1, ty));

            // Undiscovered tiles: black
            bool explored = (ty >= 0 && ty < static_cast<int>(exploredTiles.size()) && tx >= 0 && tx < static_cast<int>(exploredTiles[ty].size())) ? exploredTiles[ty][tx] : false;
            if (!explored) {
                SDL_SetRenderDrawColor(sdl, 0, 0, 0, 255);
            } else {
                int id = tiles[ty][tx].id;
                World::TileColor c = getMinimapColor(id);
                SDL_SetRenderDrawColor(sdl, c.r, c.g, c.b, c.a);
            }
            SDL_RenderDrawPoint(sdl, ix + px, iy + py);
        }
    }
    
    // Draw player dot at center
    SDL_SetRenderDrawColor(sdl, 255, 0, 0, 255);
    int cx = ix + iw / 2;
    int cy = iy + ih / 2;
    SDL_RenderDrawPoint(sdl, cx, cy);

    // Border removed: HUD art provides enclosing frame
}

World::TileColor World::getMinimapColor(int tileId) const {
    switch (tileId) {
        case TILE_GRASS:          return TileColor(46, 160, 67, 255);  // brighter green
        case TILE_DIRT:           return TileColor(139, 92, 40, 255);  // warm brown
        case TILE_STONE:          return TileColor(170, 170, 170, 255); // light gray
        case TILE_ASPHALT:        return TileColor(70, 70, 70, 255);
        case TILE_CONCRETE:       return TileColor(180, 180, 180, 255);
        case TILE_SAND:           return TileColor(230, 200, 120, 255);
        case TILE_SNOW:           return TileColor(245, 250, 255, 255);
        case TILE_GRASSY_ASPHALT: return TileColor(90, 140, 90, 255);
        case TILE_GRASSY_CONCRETE:return TileColor(100, 160, 100, 255);
        case TILE_SANDY_DIRT:     return TileColor(205, 170, 110, 255);
        case TILE_SANDY_STONE:    return TileColor(205, 190, 150, 255);
        case TILE_SNOWY_STONE:    return TileColor(230, 235, 245, 255);
        case TILE_STONY_DIRT:     return TileColor(160, 130, 110, 255);
        case TILE_WET_DIRT:       return TileColor(120, 90, 70, 255);
        case TILE_WATER_SHALLOW:  return TileColor(80, 180, 255, 255);  // clear bright blue
        case TILE_WATER_DEEP:     return TileColor(0, 120, 210, 255);   // deeper blue
        case TILE_LAVA:           return TileColor(255, 90, 0, 255);
        default:                  return TileColor(46, 160, 67, 255);
    }
}

void World::loadTileTextures() {
    if (!assetManager) {
        std::cout << "AssetManager not available, skipping tile texture loading" << std::endl;
        return;
    }
    // Initialize textures containers
    tileTextures.clear();
    tileVariantTextures.clear();
    tileTextures.resize(TILE_LAST + 1, nullptr);
    tileVariantTextures.resize(TILE_LAST + 1);

    auto load8 = [&](int type, const std::string& dir, const std::string& prefix){
        std::vector<Texture*> list;
        for (int i = 1; i <= 8; ++i) {
            char name[64];
            std::snprintf(name, sizeof(name), "%s_%02d.png", prefix.c_str(), i);
            std::string path = std::string("assets/Textures/Tiles/") + dir + "/" + name;
            Texture* t = assetManager->getTexture(path);
            if (t) list.push_back(t);
        }
        if (!list.empty()) {
            // Save ordered base variants for chaining (01..08)
            if (static_cast<int>(tileBaseVariants.size()) <= type) tileBaseVariants.resize(type + 1);
            tileBaseVariants[type] = list;

            // Also create a biased set for non-chained selection
            auto pick = [&](int idx){ return (idx >= 0 && idx < static_cast<int>(list.size())) ? list[idx] : nullptr; };
            std::vector<Texture*> biased;
            for (int k = 0; k < 6; ++k) { if (pick(5)) biased.push_back(pick(5)); }
            for (int k = 0; k < 6; ++k) { if (pick(6)) biased.push_back(pick(6)); }
            for (int k = 0; k < 6; ++k) { if (pick(7)) biased.push_back(pick(7)); }
            biased.insert(biased.end(), list.begin(), list.end());

            tileVariantTextures[type] = biased;
            tileTextures[type] = pick(5) ? pick(5) : list.front();
        }
    };

    // Base materials
    load8(TILE_GRASS,           "Grass",            "Grass");
    load8(TILE_DIRT,            "Dirt",             "Dirt");
    load8(TILE_STONE,           "Stone",            "Stone");
    load8(TILE_ASPHALT,         "Asphalt",          "Asphalt");
    load8(TILE_CONCRETE,        "Concrete",         "Concrete");
    load8(TILE_SAND,            "Sand",             "Sand");
    load8(TILE_SNOW,            "Snow",             "Snow");

    // Transition/buffer materials
    load8(TILE_GRASSY_ASPHALT,  "Grassy Asphalt",   "GrassyAsphalt");
    load8(TILE_GRASSY_CONCRETE, "Grassy Concrete",  "GrassyConcrete");
    load8(TILE_SANDY_DIRT,      "Sandy Dirt",       "SandyDirt");
    load8(TILE_SANDY_STONE,     "Sandy Stone",      "SandyStone");
    load8(TILE_SNOWY_STONE,     "Snowy Stone",      "SnowyStone");
    load8(TILE_STONY_DIRT,      "Stony Dirt",       "StonyDirt");
    load8(TILE_WET_DIRT,        "Wet Dirt",         "WetDirt");

    // Fluids/hazards
    tileTextures[TILE_WATER_SHALLOW] = assetManager->getTexture("assets/Textures/Tiles/Water/water_shallow.png");
    // Also set a static fallback texture for deep water in case sprite sheet fails
    tileTextures[TILE_WATER_DEEP] = assetManager->getTexture("assets/Textures/Tiles/Water/water_deep_01.png");
    // water_deep_01.png: auto-detect layout (horizontal or vertical). totalFrames=4.
    deepWaterSpriteSheet = assetManager->loadSpriteSheet("assets/Textures/Tiles/Water/water_deep_01.png", 32, 32, 0, 4);
    lavaSpriteSheet = assetManager->loadSpriteSheet("assets/Textures/Tiles/Lava/lava.png", 32, 32, 9, 9);
}

bool World::isGrassAt(int tx, int ty) {
    auto [chunkX, chunkY] = worldToChunkCoords(tx, ty);
    Chunk* c = getChunk(chunkX, chunkY);
    if (!c) return false;
    const int s = tileGenConfig.chunkSize;
    int lx = tx - chunkX * s;
    int ly = ty - chunkY * s;
    if (lx < 0 || ly < 0 || lx >= s || ly >= s) return false;
    return c->tiles[ly][lx].id == TILE_GRASS;
}

std::string World::buildMaskFromNeighbors(int tx, int ty) {
    // Only for stone center tiles
    std::string m;
    if (isGrassAt(tx, ty - 1)) m += 'T';
    if (isGrassAt(tx, ty + 1)) m += 'B';
    if (isGrassAt(tx - 1, ty)) m += 'L';
    if (isGrassAt(tx + 1, ty)) m += 'R';
    if (m.empty()) return "";
    if (m.size() == 4) return "ALL";
    return m;
}

Texture* World::getEdgeTextureForMask(const std::string& mask) {
    if (mask.empty()) return tileTextures[TILE_STONE];
    auto it = edgeTextures.find(mask);
    if (it != edgeTextures.end() && it->second) return it->second;
    if (mask == "R") {
        auto itLR = edgeTextures.find("LR");
        if (itLR != edgeTextures.end() && itLR->second) return itLR->second;
    }
    return tileTextures[TILE_STONE];
}

void World::loadTilemap(const std::string& filename) {
    // TODO: Implement tilemap loading from file
    std::cout << "Loading tilemap: " << filename << " (not implemented yet)" << std::endl;
}

void World::setTile(int x, int y, int tileId) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        // Validate tile ID to ensure it's within valid range
        if (tileId < 0 || tileId > TILE_LAST) {
            std::cout << "WARNING: Invalid tile ID " << tileId << " at position (" << x << ", " << y << "). Setting to grass." << std::endl;
            tileId = TILE_GRASS;
        }
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

bool World::isHazardTileId(int tileId) const {
    return tileId == TILE_WATER_SHALLOW || tileId == TILE_WATER_DEEP || tileId == TILE_LAVA;
}

bool World::isSafeTile(int tileX, int tileY) const {
    if (tileX < 0 || tileX >= width || tileY < 0 || tileY >= height) return false;
    const Tile &t = tiles[tileY][tileX];
    return t.walkable && !isHazardTileId(t.id);
}



void World::addObject(std::unique_ptr<Object> object) {
    if (object) {
        objects.push_back(std::move(object));
    }
}

void World::addEnemy(std::unique_ptr<Enemy> enemy) {
    if (enemy) {
        enemies.push_back(std::move(enemy));
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

int World::getPrioritizedTileType(int x, int y) {
    // Simple fallback: Stone, Grass, then Dirt
    float noise = (sin(x * 0.1f) + cos(y * 0.1f) + sin((x + y) * 0.05f)) / 3.0f;
    noise = (noise + 1.0f) / 2.0f; // 0..1
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float randomValue = dist(rng);
    float v = (noise * 0.3f + randomValue * 0.7f);
    if (v < 0.4f) return TILE_STONE;
    if (v < 0.6f) return TILE_GRASS;
    return TILE_DIRT;
}

bool World::shouldPlaceStone(int x, int y) {
    // Stone placement logic - place stone in clusters
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float randomValue = dist(rng);
    
    // Check if nearby tiles are stone to create clusters
    int stoneCount = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int nx = x + dx;
            int ny = y + dy;
            if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                int tileId = tiles[ny][nx].id;
                if (tileId == TILE_STONE) {
                    stoneCount++;
                }
            }
        }
    }
    
    // Higher chance of stone if nearby tiles are also stone
    float stoneProbability = 0.2f + (stoneCount * 0.1f);
    return randomValue < stoneProbability;
}

bool World::shouldPlaceStoneGrass(int x, int y) {
    // Stone grass placement logic - place near stone tiles
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float randomValue = dist(rng);
    
    // Check if nearby tiles are stone to create transitions
    int stoneCount = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int nx = x + dx;
            int ny = y + dy;
            if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                int tileId = tiles[ny][nx].id;
                if (tileId == TILE_STONE) {
                    stoneCount++;
                }
            }
        }
    }
    
    // Higher chance of stone grass if nearby tiles are stone
    float stoneGrassProbability = 0.15f + (stoneCount * 0.05f);
    return randomValue < stoneGrassProbability;
}

// New tilemap generation methods

void World::generateTilemap(const TileGenerationConfig& config) {
    // Update the tile generation config
    tileGenConfig = config;
    
    // Update world dimensions if specified
    if (config.worldWidth > 0 && config.worldHeight > 0) {
        width = config.worldWidth;
        height = config.worldHeight;
    }
    
    // Initialize RNG with fixed seed if specified
    initializeRNG();
    
    // Validate tile weights
    validateTileWeights();
    
    // Initialize tile grid
    tiles.resize(height, std::vector<Tile>(width));
    
    std::cout << "Generating tilemap: " << width << "x" << height << " tiles" << std::endl;
    std::cout << "Tile weights configured; using biome-based generation with transition buffers." << std::endl;
    
    // Generate tiles based on configuration
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int tileType;
            if (tileGenConfig.useNoiseDistribution) {
                tileType = generateNoiseBasedTileType(x, y);
            } else {
                tileType = generateWeightedTileType(x, y);
            }
            
            // Set tile properties based on type
            bool walkable = true;
            bool transparent = true;
            
            // Validate tile type to ensure it's within valid range
            if (tileType < 0 || tileType > TILE_LAST) {
                std::cout << "ERROR: Invalid tile type " << tileType << " generated at position (" << x << ", " << y << "). Setting to grass." << std::endl;
                tileType = TILE_GRASS;
            }
            
            switch (tileType) {
                case TILE_GRASS:
                case TILE_DIRT:
                case TILE_STONE:
                case TILE_ASPHALT:
                case TILE_CONCRETE:
                case TILE_SAND:
                case TILE_SNOW:
                case TILE_GRASSY_ASPHALT:
                case TILE_GRASSY_CONCRETE:
                case TILE_SANDY_DIRT:
                case TILE_SANDY_STONE:
                case TILE_SNOWY_STONE:
                case TILE_STONY_DIRT:
                case TILE_WET_DIRT:
                    walkable = true;
                    transparent = true;
                    break;
                case TILE_WATER_SHALLOW:
                case TILE_WATER_DEEP:
                case TILE_LAVA:
                    walkable = false;
                    transparent = true;
                    break;
                default:
                    walkable = true;
                    transparent = true;
                    break;
            }
            
            tiles[y][x] = Tile(tileType, walkable, transparent);
        }
    }
    
    // Apply stone clustering if enabled (legacy behavior)
    if (tileGenConfig.enableStoneClustering) {
        applyStoneClustering();
    }
    
    // Print tile distribution for debugging
    printTileDistribution();
    
    // Additional debugging: Check for any invalid tile types
    int invalidTileCount = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (tiles[y][x].id < 0 || tiles[y][x].id > TILE_LAST) {
                invalidTileCount++;
                std::cout << "ERROR: Invalid tile ID " << tiles[y][x].id << " at position (" << x << ", " << y << ")" << std::endl;
            }
        }
    }
    
    if (invalidTileCount > 0) {
        std::cout << "WARNING: Found " << invalidTileCount << " invalid tile IDs!" << std::endl;
    } else {
        std::cout << "All tile IDs are valid." << std::endl;
    }
    
    std::cout << "Tilemap generation complete!" << std::endl;
}

void World::regenerateTilemap(const TileGenerationConfig& config) {
    // Clear existing tiles and regenerate
    tiles.clear();
    
    // Reinitialize visibility arrays
    visibleTiles.clear();
    exploredTiles.clear();
    visibleTiles.resize(height, std::vector<bool>(width, false));
    exploredTiles.resize(height, std::vector<bool>(width, false));
    
    generateTilemap(config);
}

void World::initializeRNG() {
    if (tileGenConfig.useFixedSeed) {
        rng.seed(tileGenConfig.fixedSeed);
        std::cout << "Using fixed seed: " << tileGenConfig.fixedSeed << std::endl;
    } else {
        std::random_device rd;
        rng.seed(rd());
        std::cout << "Using random seed" << std::endl;
    }
}

int World::generateWeightedTileType(int x, int y) {
    // Generate a random value between 0 and 100
    std::uniform_real_distribution<float> dist(0.0f, 100.0f);
    float randomValue = dist(rng);
    
    // Apply weights in order - stone is now highest priority
    float cumulativeWeight = 0.0f;
    
    // Stone (highest priority - 40%)
    cumulativeWeight += tileGenConfig.stoneWeight;
    if (randomValue < cumulativeWeight) {
        return TILE_STONE;
    }
    
    // Grass (lower priority - 20%)
    cumulativeWeight += tileGenConfig.grassWeight;
    if (randomValue < cumulativeWeight) {
        return TILE_GRASS;
    }
    
    // Stone grass variants (40% total, distributed evenly)
    // Each variant gets approximately 2.1% chance
    float stoneGrassWeight = tileGenConfig.stoneGrassWeight;
    float stoneGrass2Weight = tileGenConfig.stoneGrass2Weight;
    float stoneGrass3Weight = tileGenConfig.stoneGrass3Weight;
    float stoneGrass4Weight = tileGenConfig.stoneGrass4Weight;
    float stoneGrass5Weight = tileGenConfig.stoneGrass5Weight;
    float stoneGrass6Weight = tileGenConfig.stoneGrass6Weight;
    // No stone_grass variants anymore; fallback sequence
    if (randomValue < cumulativeWeight + stoneGrassWeight) {
        return TILE_DIRT;
    }
    // Otherwise return grass as low-priority
    return TILE_GRASS;
}

int World::generateNoiseBasedTileType(int x, int y) {
    // Generate noise value for this position
    float noiseValue = generateNoise(x * tileGenConfig.noiseScale, y * tileGenConfig.noiseScale);
    
    // Use noise to determine tile type with weighted distribution
    float normalizedNoise = (noiseValue + 1.0f) / 2.0f; // Normalize to 0-1
    
    // Simplified thresholds for legacy path
    float stoneThreshold = tileGenConfig.stoneWeight / 100.0f;
    float grassThreshold = stoneThreshold + (tileGenConfig.grassWeight / 100.0f);
    
    // Determine tile type based on noise value
    if (normalizedNoise < stoneThreshold) {
        return TILE_STONE;
    } else if (normalizedNoise < grassThreshold) {
        return TILE_GRASS;
    } else {
        return TILE_DIRT;
    }
}

float World::generateNoise(float x, float y) const {
    // Simple Perlin-like noise implementation using sine waves
    float noise = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float persistence = 0.5f;
    int octaves = 4;
    
    for (int i = 0; i < octaves; i++) {
        noise += amplitude * sin(x * frequency + y * frequency * 0.5f);
        amplitude *= persistence;
        frequency *= 2.0f;
    }
    
    return noise / (1.0f - pow(persistence, octaves));
}

void World::applyStoneClustering() {
    std::cout << "Applying stone clustering..." << std::endl;
    
    // Create a copy of the current tiles for reference
    std::vector<std::vector<Tile>> tempTiles = tiles;
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Check if this tile is stone
            if (tempTiles[y][x].id == TILE_STONE) {
                // Check surrounding tiles within cluster radius
                int stoneCount = 0;
                int totalCount = 0;
                
                for (int dy = -tileGenConfig.stoneClusterRadius; dy <= tileGenConfig.stoneClusterRadius; dy++) {
                    for (int dx = -tileGenConfig.stoneClusterRadius; dx <= tileGenConfig.stoneClusterRadius; dx++) {
                        int nx = x + dx;
                        int ny = y + dy;
                        
                        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                            totalCount++;
                            if (tempTiles[ny][nx].id == TILE_STONE) {
                                stoneCount++;
                            }
                        }
                    }
                }
                
                // If there are enough stone tiles nearby, increase the chance of stone in surrounding areas
                float stoneRatio = static_cast<float>(stoneCount) / totalCount;
                if (stoneRatio > 0.3f) { // If more than 30% of nearby tiles are stone
                    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
                    float randomValue = dist(rng);
                    
                    // Increase stone probability in surrounding areas
                    if (randomValue < tileGenConfig.stoneClusterChance) {
                        // Convert some nearby grass tiles to stone or dirt (legacy transition removed)
                        for (int dy = -1; dy <= 1; dy++) {
                            for (int dx = -1; dx <= 1; dx++) {
                                int nx = x + dx;
                                int ny = y + dy;
                                
                                if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                                    if (tempTiles[ny][nx].id == TILE_GRASS) {
                                        // 50% chance to convert to stone, 50% to stone grass
                                        if (dist(rng) < 0.5f) {
                                            tiles[ny][nx].id = TILE_STONE;
                                        } else {
                                            tiles[ny][nx].id = TILE_DIRT;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void World::validateTileWeights() {
    float totalWeight = tileGenConfig.stoneWeight + tileGenConfig.grassWeight + 
                       tileGenConfig.stoneGrassWeight + tileGenConfig.stoneGrass2Weight +
                       tileGenConfig.stoneGrass3Weight + tileGenConfig.stoneGrass4Weight +
                       tileGenConfig.stoneGrass5Weight + tileGenConfig.stoneGrass6Weight +
                       tileGenConfig.stoneGrass7Weight + tileGenConfig.stoneGrass8Weight +
                       tileGenConfig.stoneGrass9Weight + tileGenConfig.stoneGrass10Weight +
                       tileGenConfig.stoneGrass11Weight + tileGenConfig.stoneGrass12Weight +
                       tileGenConfig.stoneGrass13Weight + tileGenConfig.stoneGrass14Weight +
                       tileGenConfig.stoneGrass15Weight + tileGenConfig.stoneGrass16Weight +
                       tileGenConfig.stoneGrass17Weight + tileGenConfig.stoneGrass18Weight +
                       tileGenConfig.stoneGrass19Weight;
    
    if (std::abs(totalWeight - 100.0f) > 0.01f) {
        std::cout << "Warning: Tile weights do not sum to 100% (current sum: " << totalWeight << "%)" << std::endl;
        std::cout << "Normalizing weights..." << std::endl;
        
        // Normalize weights
        tileGenConfig.stoneWeight = (tileGenConfig.stoneWeight / totalWeight) * 100.0f;
        tileGenConfig.grassWeight = (tileGenConfig.grassWeight / totalWeight) * 100.0f;
        tileGenConfig.stoneGrassWeight = (tileGenConfig.stoneGrassWeight / totalWeight) * 100.0f;
        tileGenConfig.stoneGrass2Weight = (tileGenConfig.stoneGrass2Weight / totalWeight) * 100.0f;
        tileGenConfig.stoneGrass3Weight = (tileGenConfig.stoneGrass3Weight / totalWeight) * 100.0f;
        tileGenConfig.stoneGrass4Weight = (tileGenConfig.stoneGrass4Weight / totalWeight) * 100.0f;
        tileGenConfig.stoneGrass5Weight = (tileGenConfig.stoneGrass5Weight / totalWeight) * 100.0f;
        tileGenConfig.stoneGrass6Weight = (tileGenConfig.stoneGrass6Weight / totalWeight) * 100.0f;
        tileGenConfig.stoneGrass7Weight = (tileGenConfig.stoneGrass7Weight / totalWeight) * 100.0f;
        tileGenConfig.stoneGrass8Weight = (tileGenConfig.stoneGrass8Weight / totalWeight) * 100.0f;
        tileGenConfig.stoneGrass9Weight = (tileGenConfig.stoneGrass9Weight / totalWeight) * 100.0f;
        tileGenConfig.stoneGrass10Weight = (tileGenConfig.stoneGrass10Weight / totalWeight) * 100.0f;
        tileGenConfig.stoneGrass11Weight = (tileGenConfig.stoneGrass11Weight / totalWeight) * 100.0f;
        tileGenConfig.stoneGrass12Weight = (tileGenConfig.stoneGrass12Weight / totalWeight) * 100.0f;
        tileGenConfig.stoneGrass13Weight = (tileGenConfig.stoneGrass13Weight / totalWeight) * 100.0f;
        tileGenConfig.stoneGrass14Weight = (tileGenConfig.stoneGrass14Weight / totalWeight) * 100.0f;
        tileGenConfig.stoneGrass15Weight = (tileGenConfig.stoneGrass15Weight / totalWeight) * 100.0f;
        tileGenConfig.stoneGrass16Weight = (tileGenConfig.stoneGrass16Weight / totalWeight) * 100.0f;
        tileGenConfig.stoneGrass17Weight = (tileGenConfig.stoneGrass17Weight / totalWeight) * 100.0f;
        tileGenConfig.stoneGrass18Weight = (tileGenConfig.stoneGrass18Weight / totalWeight) * 100.0f;
        tileGenConfig.stoneGrass19Weight = (tileGenConfig.stoneGrass19Weight / totalWeight) * 100.0f;
        
        std::cout << "Normalized weights - Stone: " << tileGenConfig.stoneWeight 
                  << "%, Grass: " << tileGenConfig.grassWeight 
                  << "%, StoneGrass variants: 40% total (2% each)" << std::endl;
    }
}

void World::printTileDistribution() {
    int grassCount = 0, stoneCount = 0;
    int stoneGrassCounts[19] = {0}; // Legacy, unused with new materials
    
    // Count tile types
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int tileId = tiles[y][x].id;
            switch (tileId) {
                case TILE_GRASS: grassCount++; break;
                case TILE_STONE: stoneCount++; break;
                default:
                    break;
            }
        }
    }
    
    int totalTiles = width * height;
    float grassPercent = (static_cast<float>(grassCount) / totalTiles) * 100.0f;
    float stonePercent = (static_cast<float>(stoneCount) / totalTiles) * 100.0f;
    
    std::cout << "Tile distribution:" << std::endl;
    std::cout << "  Stone: " << stoneCount << " (" << stonePercent << "%)" << std::endl;
    std::cout << "  Grass: " << grassCount << " (" << grassPercent << "%)" << std::endl;
    
    // Omit legacy stone_grass breakdown with new materials
}

// Fog of war and visibility methods
void World::updateVisibility(float playerX, float playerY) {
    if (!fogOfWarEnabled) {
        return;
    }
    
    // Clear current visibility
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            visibleTiles[y][x] = false;
        }
    }
    
    // Calculate visibility from player position
    calculateVisibility(playerX, playerY);
}

void World::calculateVisibility(float playerX, float playerY) {
    // Convert player position to tile coordinates
    int playerTileX = static_cast<int>(playerX / tileSize);
    int playerTileY = static_cast<int>(playerY / tileSize);
    
    // Clamp to world bounds
    playerTileX = std::max(0, std::min(width - 1, playerTileX));
    playerTileY = std::max(0, std::min(height - 1, playerTileY));
    
    // Mark the player's tile as visible and explored
    markTileVisible(playerTileX, playerTileY);
    markTileExplored(playerTileX, playerTileY);
    
    // Calculate visibility in a radius around the player - make it more generous
    for (int y = playerTileY - visibilityRadius; y <= playerTileY + visibilityRadius; y++) {
        for (int x = playerTileX - visibilityRadius; x <= playerTileX + visibilityRadius; x++) {
            // Check if tile is within world bounds
            if (x < 0 || x >= width || y < 0 || y >= height) {
                continue;
            }
            
            // Calculate distance from player
            int dx = x - playerTileX;
            int dy = y - playerTileY;
            float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy));
            
            // Check if tile is within visibility radius - be more generous
            if (distance <= visibilityRadius) {
                // Check line of sight - simplified to be less restrictive
                if (hasLineOfSight(playerTileX, playerTileY, x, y)) {
                    markTileVisible(x, y);
                    markTileExplored(x, y);
                }
            }
        }
    }
    
    // Debug output (only print once every 60 frames)
    static int visibilityDebugCount = 0;
    if (visibilityDebugCount++ % 60 == 0) {
        int visibleCount = 0;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (visibleTiles[y][x]) {
                    visibleCount++;
                }
            }
        }
        std::cout << "Visibility update - Player at tile (" << playerTileX << ", " << playerTileY 
                  << "), Visible tiles: " << visibleCount << " out of " << (width * height) << std::endl;
    }
}

bool World::hasLineOfSight(int startX, int startY, int endX, int endY) const {
    // Simplified line of sight - just check if the tiles are within a reasonable distance
    // This is less restrictive than the Bresenham algorithm
    int dx = std::abs(endX - startX);
    int dy = std::abs(endY - startY);
    
    // If the distance is very small, always allow line of sight
    if (dx <= 3 && dy <= 3) {
        return true;
    }
    
    // For longer distances, use a simple check
    float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy));
    
    // If distance is within a reasonable range, allow line of sight
    if (distance <= 15.0f) {
        return true;
    }
    
    // For very long distances, use the original Bresenham algorithm
    // Use Bresenham's line algorithm to check line of sight
    int sx = startX < endX ? 1 : -1;
    int sy = startY < endY ? 1 : -1;
    int err = dx - dy;
    
    int x = startX;
    int y = startY;
    
    while (true) {
        // Check if current tile blocks line of sight
        if (x >= 0 && x < width && y >= 0 && y < height) {
            if (!tiles[y][x].transparent) {
                return false; // Line of sight blocked
            }
        }
        
        if (x == endX && y == endY) {
            break;
        }
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
    
    return true;
}

bool World::isTileVisible(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return false;
    }
    return visibleTiles[y][x];
}

void World::markTileVisible(int x, int y) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        visibleTiles[y][x] = true;
    }
}

void World::markTileExplored(int x, int y) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        exploredTiles[y][x] = true;
    }
}

World::TileColor World::getTileColor(int tileId) const {
    switch (tileId) {
        case TILE_GRASS:          return TileColor(34, 139, 34, 255);
        case TILE_DIRT:           return TileColor(139, 69, 19, 255);
        case TILE_STONE:          return TileColor(128, 128, 128, 255);
        case TILE_ASPHALT:        return TileColor(60, 60, 60, 255);
        case TILE_CONCRETE:       return TileColor(120, 120, 120, 255);
        case TILE_SAND:           return TileColor(237, 201, 175, 255);
        case TILE_SNOW:           return TileColor(240, 248, 255, 255);
        case TILE_GRASSY_ASPHALT: return TileColor(80, 120, 80, 255);
        case TILE_GRASSY_CONCRETE:return TileColor(90, 140, 90, 255);
        case TILE_SANDY_DIRT:     return TileColor(210, 180, 140, 255);
        case TILE_SANDY_STONE:    return TileColor(200, 190, 160, 255);
        case TILE_SNOWY_STONE:    return TileColor(220, 230, 240, 255);
        case TILE_STONY_DIRT:     return TileColor(150, 120, 100, 255);
        case TILE_WET_DIRT:       return TileColor(110, 80, 60, 255);
        case TILE_WATER_SHALLOW:  return TileColor(64, 164, 223, 220);
        case TILE_WATER_DEEP:     return TileColor(0, 105, 148, 220);
        case TILE_LAVA:           return TileColor(255, 69, 0, 220);
        default:                  return TileColor(34, 139, 34, 255);
    }
}

// Chunk management functions
std::string World::getChunkKey(int chunkX, int chunkY) const {
    return std::to_string(chunkX) + "," + std::to_string(chunkY);
}

std::pair<int, int> World::worldToChunkCoords(int worldX, int worldY) const {
    const int s = tileGenConfig.chunkSize;
    auto floorDiv = [s](int v) -> int {
        // Integer floor division that works for negatives
        return (v >= 0) ? (v / s) : ((v + 1) / s - 1);
    };
    return {floorDiv(worldX), floorDiv(worldY)};
}

std::pair<int, int> World::chunkToWorldCoords(int chunkX, int chunkY) const {
    return {chunkX * tileGenConfig.chunkSize, chunkY * tileGenConfig.chunkSize};
}

Chunk* World::getChunk(int chunkX, int chunkY) {
    std::string key = getChunkKey(chunkX, chunkY);
    auto it = chunks.find(key);
    if (it != chunks.end()) {
        return it->second.get();
    }
    return nullptr;
}

void World::generateChunk(int chunkX, int chunkY) {
    std::string key = getChunkKey(chunkX, chunkY);
    
    // Check if chunk already exists
    if (chunks.find(key) != chunks.end()) {
        return;
    }
    
    // Create new chunk
    auto chunk = std::make_unique<Chunk>(chunkX, chunkY, tileGenConfig.chunkSize);
    generateChunkTiles(chunk.get());
    chunk->isGenerated = true;
    
    // Store chunk
    chunks[key] = std::move(chunk);
    
    std::cout << "Generated chunk (" << chunkX << ", " << chunkY << ")" << std::endl;
}

void World::generateChunkTiles(Chunk* chunk) {
    if (!chunk) return;
    
    int chunkSize = tileGenConfig.chunkSize;
    int worldStartX = chunk->chunkX * chunkSize;
    int worldStartY = chunk->chunkY * chunkSize;
    
    // Generate per-tile from biomes
    for (int y = 0; y < chunkSize; y++) {
        for (int x = 0; x < chunkSize; x++) {
            int wx = worldStartX + x;
            int wy = worldStartY + y;
            int biome = getBiomeType(wx, wy);
            int region = pickRegionGroupForBiome(wx, wy, biome);
            // Priority on a single base tile per region; variants only as accents later
            int mat = pickBaseMaterialForGroup(region, 0.5f);
            bool walk = !(mat == TILE_WATER_SHALLOW || mat == TILE_WATER_DEEP || mat == TILE_LAVA);
            chunk->tiles[y][x] = Tile(mat, walk, true);
        }
    }

    // Smooth large patches
    smoothRegions(chunk);
    applyTransitionBuffers(chunk);
    addAccents(chunk);
    // Water features
    placeWaterLakes(chunk);
    carveRivers(chunk);
    pruneRiverStubs(chunk);
}

// Classification helpers for color groups and smoothing
int World::getMaterialGroupId(int tileId) const {
    switch (tileId) {
        case TILE_GRASS:
        case TILE_GRASSY_ASPHALT:
        case TILE_GRASSY_CONCRETE: return 0; // green
        case TILE_DIRT:
        case TILE_STONY_DIRT:
        case TILE_WET_DIRT:        return 1; // earth
        case TILE_STONE:
        case TILE_CONCRETE:
        case TILE_ASPHALT:
        case TILE_SNOWY_STONE:     return 2; // stone/gray
        case TILE_SAND:
        case TILE_SANDY_DIRT:
        case TILE_SANDY_STONE:     return 3; // sand/yellow
        case TILE_SNOW:            return 4; // snow/white
        default:                   return 1;
    }
}

bool World::areMaterialsCloseInColor(int a, int b) const {
    int ga = getMaterialGroupId(a);
    int gb = getMaterialGroupId(b);
    if (ga == gb) return true;
    if ((ga == 1 && gb == 2) || (ga == 2 && gb == 1)) return true; // earth~stone
    if ((ga == 1 && gb == 3) || (ga == 3 && gb == 1)) return true; // earth~sand
    if ((ga == 2 && gb == 3) || (ga == 3 && gb == 2)) return true; // stone~sand
    return false;
}

int World::pickRegionGroupForBiome(int wx, int wy, int biomeType) const {
    // Use multi-octave low-frequency noise to create very large contiguous regions
    float base = tileGenConfig.regionNoiseScale;
    float f = (generateNoise(wx * base, wy * base) * 0.6f
             + generateNoise(wx * base * 0.5f, wy * base * 0.5f) * 0.3f
             + generateNoise(wx * base * 0.25f, wy * base * 0.25f) * 0.1f);
    f = (f + 1.0f) * 0.5f;
    switch (biomeType) {
        case 0: // plains
            if (f < 0.65f) return 0; // green
            if (f < 0.85f) return 1; // earth
            return 2;                // stone
        case 1: // hills
            if (f < 0.5f) return 2;
            if (f < 0.8f) return 1;
            return 0;
        case 2: // mountains
            if (f < 0.8f) return 2; return 1;
        case 3: // arid
            if (f < 0.7f) return 3;
            if (f < 0.9f) return 1;
            return 2;
        case 4: // tundra
            if (f < 0.7f) return 4; return 2;
        default:
            return 1;
    }
}

int World::pickBaseMaterialForGroup(int groupId, float noiseVal) const {
    switch (groupId) {
        // Use 06-08 biased materials by default; transitions are added later by applyTransitionBuffers
        case 0: return TILE_GRASS; // variants 06-08 are prioritized at load
        case 1: return TILE_DIRT;
        case 2: return TILE_STONE;
        case 3: return TILE_SAND;
        case 4: return TILE_SNOW;
        default: return TILE_DIRT;
    }
}

void World::smoothRegions(Chunk* chunk) {
    if (!chunk) return;
    const int s = tileGenConfig.chunkSize;
    for (int it = 0; it < tileGenConfig.regionSmoothingIterations; ++it) {
        auto copy = chunk->tiles;
        for (int y = 0; y < s; ++y) {
            for (int x = 0; x < s; ++x) {
                int counts[5] = {0};
                // Larger neighborhood to grow patches faster
                for (int dy = -2; dy <= 2; ++dy) {
                    for (int dx = -2; dx <= 2; ++dx) {
                        int nx = x + dx, ny = y + dy;
                        if (nx < 0 || ny < 0 || nx >= s || ny >= s) continue;
                        counts[getMaterialGroupId(copy[ny][nx].id)]++;
                    }
                }
                int best = 0, bestCount = -1;
                for (int g = 0; g < 5; ++g) {
                    if (counts[g] > bestCount) { bestCount = counts[g]; best = g; }
                }
                int desired = pickBaseMaterialForGroup(best, 0.5f);
                if (!areMaterialsCloseInColor(copy[y][x].id, desired)) {
                    chunk->tiles[y][x].id = desired;
                }
            }
        }
    }
}

void World::addAccents(Chunk* chunk) {
    if (!chunk) return;
    const int s = tileGenConfig.chunkSize;
    // Deterministic PRNG per chunk for stable accents
    std::mt19937 prng(static_cast<unsigned int>((chunk->chunkX * 2654435761u) ^ (chunk->chunkY * 97531u)));
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    auto groupAccent = [&](int groupId, int baseMat) -> int {
        // Map group to a rare accent within same color family
        switch (groupId) {
            case 0: return baseMat; // green stays uniform for readability
            case 1: return TILE_STONY_DIRT; // occasional stones in dirt
            case 2: return TILE_CONCRETE;   // occasional concrete in stone fields
            case 3: return TILE_SANDY_STONE; // rocky sand spot
            case 4: return TILE_SNOWY_STONE; // rocky snow spot
            default: return baseMat;
        }
    };

    for (int y = 0; y < s; ++y) {
        for (int x = 0; x < s; ++x) {
            int id = chunk->tiles[y][x].id;
            if (id == TILE_WATER_SHALLOW || id == TILE_WATER_DEEP || id == TILE_LAVA) continue;
            int g = getMaterialGroupId(id);
            float r = dist(prng);
            if (r < tileGenConfig.accentChance) {
                int accent = groupAccent(g, id);
                // Respect color compatibility strictly
                if (areMaterialsCloseInColor(id, accent)) {
                    chunk->tiles[y][x].id = accent;
                }
            }
        }
    }
}

int World::getPreferredVariantIndex(int tileType, int worldX, int worldY) const {
    // Prefer variants 06-08 using very low-frequency noise to form large blobs.
    float base = tileGenConfig.regionNoiseScale * 0.75f;
    float f = (generateNoise(worldX * base, worldY * base) + 1.0f) * 0.5f; // 0..1
    if (f < 0.66f) return 5; // -> index 5 (frame 06)
    if (f < 0.85f) return 6; // -> index 6 (frame 07)
    return 7;                // -> index 7 (frame 08)
}

void World::carveRivers(Chunk* chunk) {
    if (!chunk) return;
    const int s = tileGenConfig.chunkSize;
    const int worldStartX = chunk->chunkX * s;
    const int worldStartY = chunk->chunkY * s;

    auto inb = [&](int x, int y){ return x >= 0 && x < s && y >= 0 && y < s; };
    auto isDeep = [&](int x, int y){ return chunk->tiles[y][x].id == TILE_WATER_DEEP; };
    auto isShallow = [&](int x, int y){ return chunk->tiles[y][x].id == TILE_WATER_SHALLOW; };

    // Collect candidate lake-edge seeds (not adjacent to deep water) in grassy/stone biomes
    std::vector<std::pair<int,int>> seeds;
    for (int y = 1; y < s - 1; ++y) {
        for (int x = 1; x < s - 1; ++x) {
            if (!isShallow(x, y)) continue;
            int wx = worldStartX + x;
            int wy = worldStartY + y;
            int biome = getBiomeType(wx, wy);
            if (!(biome == 0 || biome == 1 || biome == 2)) continue;
            bool adjacentDeep = false, edge = false;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = x + dx, ny = y + dy;
                    if (!inb(nx, ny)) continue;
                    if (isDeep(nx, ny)) adjacentDeep = true;
                    if (!isShallow(nx, ny)) edge = true;
                }
            }
            // Ensure we start near the downhill/ocean-ward side of the lake by preferring higher ocean potential
            if (!adjacentDeep && edge) {
                float oceanP = (generateNoise(wx * tileGenConfig.waterNoiseScale, wy * tileGenConfig.waterNoiseScale) + 1.0f) * 0.5f;
                if (oceanP > tileGenConfig.shallowWaterThreshold) seeds.emplace_back(x, y);
            }
        }
    }

    // Deterministic PRNG per chunk to pick a couple of seeds
    std::mt19937 prng(static_cast<unsigned int>((chunk->chunkX * 2654435761u) ^ (chunk->chunkY * 97531u)));
    std::shuffle(seeds.begin(), seeds.end(), prng);
    int numRivers = std::min(3, static_cast<int>(seeds.size()));

    const float oceanScale = tileGenConfig.waterNoiseScale;          // pulls rivers toward oceans
    const float flowScale  = tileGenConfig.waterNoiseScale * 1.6f;   // adds windiness

    for (int i = 0; i < numRivers; ++i) {
        int x = seeds[i].first;
        int y = seeds[i].second;
        int prevX = x, prevY = y;
        const int maxSteps = s * 6; // longer reach

        for (int step = 0; step < maxSteps; ++step) {
            int wx = worldStartX + x;
            int wy = worldStartY + y;

            // Compute a meandering flow direction using angle noise
            float ang = (generateNoise(wx * flowScale, wy * flowScale) + 1.0f) * 3.14159265f; // 0..2pi
            float dirX = std::cos(ang);
            float dirY = std::sin(ang);

            // Pick the neighbor that maximizes ocean potential with a small alignment to flow direction
            int bestNx = x, bestNy = y; float bestScore = -1e9f;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = x + dx, ny = y + dy;
                    if (!inb(nx, ny)) continue;
                    if (nx == prevX && ny == prevY) continue; // avoid immediate backtrack
                    int nwx = worldStartX + nx;
                    int nwy = worldStartY + ny;
                    float oceanN = (generateNoise(nwx * oceanScale, nwy * oceanScale) + 1.0f) * 0.5f;
                    float align = (dx * dirX + dy * dirY) * 0.35f; // windiness
                    float score = oceanN + align;
                    if (score > bestScore) { bestScore = score; bestNx = nx; bestNy = ny; }
                }
            }

            if (bestNx == x && bestNy == y) break; // stuck
            prevX = x; prevY = y; x = bestNx; y = bestNy;

            // Stop if we reached deep water
            if (isDeep(x, y)) break;

            // Carve narrow river, discourage branching by only carving current cell
            chunk->tiles[y][x].id = TILE_WATER_SHALLOW;
            // Rarely widen, but do not create lateral offshoots
            if ((step % 13) == 0) {
                for (int wy2 = -1; wy2 <= 1; ++wy2) {
                    for (int wx2 = -1; wx2 <= 1; ++wx2) {
                        int nx = x + wx2, ny = y + wy2;
                        if (!inb(nx, ny)) continue;
                        if (std::abs(wx2) + std::abs(wy2) == 1 && (nx == prevX || ny == prevY)) { // widen in flow direction
                            if (chunk->tiles[ny][nx].id != TILE_WATER_DEEP) {
                                chunk->tiles[ny][nx].id = TILE_WATER_SHALLOW;
                            }
                        }
                    }
                }
            }
        }
    }
}

void World::pruneRiverStubs(Chunk* chunk) {
    if (!chunk) return;
    const int s = tileGenConfig.chunkSize;
    auto inb = [&](int x, int y){ return x >= 0 && x < s && y >= 0 && y < s; };

    // Two passes: remove shallow-water cells with <=1 shallow neighbors and not touching deep water
    for (int pass = 0; pass < 2; ++pass) {
        auto copy = chunk->tiles;
        for (int y = 0; y < s; ++y) {
            for (int x = 0; x < s; ++x) {
                if (copy[y][x].id != TILE_WATER_SHALLOW) continue;
                int shallowN = 0; bool touchesDeep = false;
                const int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
                for (auto &d : dirs) {
                    int nx = x + d[0], ny = y + d[1];
                    if (!inb(nx, ny)) continue;
                    int id = copy[ny][nx].id;
                    if (id == TILE_WATER_SHALLOW) shallowN++;
                    if (id == TILE_WATER_DEEP) touchesDeep = true;
                }
                if (!touchesDeep && shallowN <= 1) {
                    chunk->tiles[y][x].id = TILE_STONE; // revert stub to local neutral (stone/gray)
                }
            }
        }
    }
}

void World::applyTransitionBuffers(Chunk* chunk) {
    if (!chunk) return;
    const int s = tileGenConfig.chunkSize;
    auto inb = [&](int x, int y){ return x >= 0 && x < s && y >= 0 && y < s; };
    auto needsBuffer = [&](int a, int b) -> int {
        if (a == b) return -1;
        auto is = [&](int t, TileType tt){ return t == static_cast<int>(tt); };
        // Grass transitions
        if ((is(a, TILE_GRASS) && is(b, TILE_CONCRETE)) || (is(b, TILE_GRASS) && is(a, TILE_CONCRETE))) return TILE_GRASSY_CONCRETE;
        if ((is(a, TILE_GRASS) && is(b, TILE_ASPHALT))  || (is(b, TILE_GRASS) && is(a, TILE_ASPHALT)))  return TILE_GRASSY_ASPHALT;
        if ((is(a, TILE_GRASS) && is(b, TILE_DIRT))     || (is(b, TILE_GRASS) && is(a, TILE_DIRT)))     return TILE_STONY_DIRT; // grass↔dirt edge accent
        // Sand transitions
        if ((is(a, TILE_STONE) && is(b, TILE_SAND))     || (is(b, TILE_STONE) && is(a, TILE_SAND)))     return TILE_SANDY_STONE;
        if ((is(a, TILE_DIRT)  && is(b, TILE_SAND))     || (is(b, TILE_DIRT)  && is(a, TILE_SAND)))     return TILE_SANDY_DIRT;
        // Snow transitions
        if ((is(a, TILE_STONE) && is(b, TILE_SNOW))     || (is(b, TILE_STONE) && is(a, TILE_SNOW)))     return TILE_SNOWY_STONE;
        // Dirt/stone generic
        if ((is(a, TILE_DIRT)  && is(b, TILE_STONE))    || (is(b, TILE_DIRT)  && is(a, TILE_STONE)))    return TILE_STONY_DIRT;
        return -1;
    };

    // Copy tiles to examine neighbors without cascading effects
    auto tilesCopy = chunk->tiles;
    for (int y = 0; y < s; ++y) {
        for (int x = 0; x < s; ++x) {
            int a = tilesCopy[y][x].id;
            if (a == TILE_WATER_SHALLOW || a == TILE_WATER_DEEP) {
                // Wet dirt rim around water
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        if (dx == 0 && dy == 0) continue;
                        int nx = x + dx, ny = y + dy;
                        if (!inb(nx, ny)) continue;
                        int b = tilesCopy[ny][nx].id;
                        if (b == TILE_DIRT || b == TILE_GRASS || b == TILE_STONE || b == TILE_SAND) {
                            chunk->tiles[ny][nx].id = TILE_WET_DIRT;
                        }
                    }
                }
                continue;
            }

            const int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
            for (auto &d : dirs) {
                int nx = x + d[0], ny = y + d[1];
                if (!inb(nx, ny)) continue;
                int b = tilesCopy[ny][nx].id;
                int buf = needsBuffer(a, b);
                if (buf >= 0) {
                    chunk->tiles[y][x].id = buf;
                    break;
                }
            }
        }
    }
}

void World::carveRandomGrassPatches(Chunk* chunk) {
    const int s = tileGenConfig.chunkSize;
    // Deterministic seed per chunk for stability
    std::mt19937 prng(static_cast<unsigned int>((chunk->chunkX * 73856093) ^ (chunk->chunkY * 19349663)));
    std::uniform_int_distribution<int> patchCountDist(4, 12);
    std::uniform_int_distribution<int> sizeDist(1, 9);

    int patches = patchCountDist(prng);
    for (int i = 0; i < patches; ++i) {
        int size = std::min(sizeDist(prng), s);
        if (size <= 0) continue;
        std::uniform_int_distribution<int> xDist(0, std::max(0, s - size));
        std::uniform_int_distribution<int> yDist(0, std::max(0, s - size));
        int ox = xDist(prng);
        int oy = yDist(prng);
        for (int y = oy; y < oy + size; ++y) {
            for (int x = ox; x < ox + size; ++x) {
                chunk->tiles[y][x].id = TILE_GRASS;
            }
        }
    }
}

void World::placeWaterLakes(Chunk* chunk) {
    const int s = tileGenConfig.chunkSize;
    int worldStartX = chunk->chunkX * s;
    int worldStartY = chunk->chunkY * s;
    // Oceans from very low frequency field, large lakes from medium frequency only in certain biomes
    float oceanScale = tileGenConfig.waterNoiseScale;
    float macroLakeScale  = tileGenConfig.waterNoiseScale * 1.1f; // medium frequency, coherent
    float macroLakeThreshold = std::max(0.80f, tileGenConfig.shallowWaterThreshold + 0.02f);

    int shallowCount = 0;
    for (int y = 0; y < s; ++y) {
        for (int x = 0; x < s; ++x) {
            int wx = worldStartX + x;
            int wy = worldStartY + y;
            int biome = getBiomeType(wx, wy);
            float deepN = (generateNoise(wx * oceanScale, wy * oceanScale) + 1.0f) * 0.5f;
            if (deepN >= tileGenConfig.deepWaterThreshold) {
                chunk->tiles[y][x].id = TILE_WATER_DEEP;
                continue;
            }
            if (biome == 0 || biome == 1 || biome == 2) {
                float lm = (generateNoise(wx * macroLakeScale, wy * macroLakeScale) + 1.0f) * 0.5f;
                if (lm > macroLakeThreshold && lm < tileGenConfig.deepWaterThreshold) {
                    chunk->tiles[y][x].id = TILE_WATER_SHALLOW;
                    shallowCount++;
                }
            }
        }
    }

    // Extremely rare oasis in arid chunks
    int centerBiome = getBiomeType(worldStartX + s / 2, worldStartY + s / 2);
    // PRNG per chunk
    std::mt19937 prng(static_cast<unsigned int>((chunk->chunkX * 2654435761u) ^ (chunk->chunkY * 97531u)));

    if (centerBiome == 3) {
        std::uniform_real_distribution<float> r01(0.0f, 1.0f);
        if (r01(prng) < 0.005f) { // 0.5% per arid chunk
            std::uniform_int_distribution<int> rx(4, s - 5);
            std::uniform_int_distribution<int> ry(4, s - 5);
            std::uniform_int_distribution<int> rr(2, 4);
            int cx = rx(prng), cy = ry(prng), rad = rr(prng);
            for (int dy = -rad; dy <= rad; ++dy) {
                for (int dx = -rad; dx <= rad; ++dx) {
                    int nx = cx + dx, ny = cy + dy;
                    if (nx < 0 || ny < 0 || nx >= s || ny >= s) continue;
                    if (dx * dx + dy * dy <= rad * rad) {
                        chunk->tiles[ny][nx].id = TILE_WATER_SHALLOW;
                        shallowCount++;
                    }
                }
            }
        }
    }

    // Fallback: if no lakes were created in a grassy/stone chunk, create a small circular lake with low probability
    if (shallowCount < s) {
        int cb = getBiomeType(worldStartX + s / 2, worldStartY + s / 2);
        if (cb == 0 || cb == 1 || cb == 2) {
            std::uniform_real_distribution<float> r01(0.0f, 1.0f);
            if (r01(prng) < 0.06f) { // ~6% per eligible chunk
                std::uniform_int_distribution<int> rx(6, s - 7);
                std::uniform_int_distribution<int> ry(6, s - 7);
                std::uniform_int_distribution<int> rr(3, 6);
                int cx = rx(prng), cy = ry(prng), rad = rr(prng);
                for (int dy = -rad; dy <= rad; ++dy) {
                    for (int dx = -rad; dx <= rad; ++dx) {
                        int nx = cx + dx, ny = cy + dy;
                        if (nx < 0 || ny < 0 || nx >= s || ny >= s) continue;
                        if (dx * dx + dy * dy <= rad * rad) {
                            chunk->tiles[ny][nx].id = TILE_WATER_SHALLOW;
                        }
                    }
                }
            }
        }
    }
}

void World::updateVisibleChunks(float playerX, float playerY) {
    // Convert player pixel position to tile, then to chunk coordinates
    int playerTileX = static_cast<int>(std::floor(playerX / static_cast<float>(tileSize)));
    int playerTileY = static_cast<int>(std::floor(playerY / static_cast<float>(tileSize)));
    auto [playerChunkX, playerChunkY] = worldToChunkCoords(playerTileX, playerTileY);
    
    // Clear previous visible chunks
    visibleChunks.clear();
    
    // Generate and mark chunks within render distance as visible (smooth roaming)
    int renderDistance = tileGenConfig.renderDistance;
    for (int cy = playerChunkY - renderDistance; cy <= playerChunkY + renderDistance; cy++) {
        for (int cx = playerChunkX - renderDistance; cx <= playerChunkX + renderDistance; cx++) {
            generateChunk(cx, cy);
            Chunk* c = getChunk(cx, cy);
            if (c) {
                c->isVisible = true;
                visibleChunks.push_back(c);
            }
        }
    }
    
    // Hide chunks outside render distance
    for (auto& [key, chunk] : chunks) {
        int dx = std::abs(chunk->chunkX - playerChunkX);
        int dy = std::abs(chunk->chunkY - playerChunkY);
        if (dx > renderDistance || dy > renderDistance) {
            chunk->isVisible = false;
        }
    }
}

// Biome system
int World::getBiomeType(int x, int y) const {
    float n1 = generateNoise(x * tileGenConfig.biomeScale, y * tileGenConfig.biomeScale);
    float n2 = generateNoise((x + 10000) * tileGenConfig.biomeScale * 0.8f, (y - 10000) * tileGenConfig.biomeScale * 0.8f);
    float a = (n1 + 1.0f) * 0.5f;
    float b = (n2 + 1.0f) * 0.5f;
    if (a < 0.25f) return 0;      // Plains
    if (a < 0.5f)  return 1;      // Hills
    if (a < 0.7f)  return 2;      // Mountains
    if (b < 0.5f)  return 3;      // Arid
    return 4;                     // Tundra
}

int World::generateBiomeBasedTile(int x, int y, int biomeType) const {
    float n = (generateNoise(x * tileGenConfig.noiseScale, y * tileGenConfig.noiseScale) + 1.0f) * 0.5f;
    switch (biomeType) {
        case 0: // Plains
            if (n < 0.75f) return TILE_GRASS;
            if (n < 0.90f) return TILE_DIRT;
            return TILE_STONE;
        case 1: // Hills
            if (n < 0.5f) return TILE_STONE;
            if (n < 0.75f) return TILE_STONY_DIRT;
            return TILE_GRASS;
        case 2: // Mountains
            if (n < 0.8f) return TILE_STONE;
            return TILE_STONY_DIRT;
        case 3: // Arid
            if (n < 0.7f) return TILE_SAND;
            if (n < 0.85f) return TILE_SANDY_DIRT;
            return TILE_SANDY_STONE;
        case 4: // Tundra
            if (n < 0.7f) return TILE_SNOW;
            return TILE_SNOWY_STONE;
        default:
            return TILE_GRASS;
    }
}
