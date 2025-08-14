#include "World.h"
#include "Renderer.h"
#include "AssetManager.h"
#include "Object.h"
#include "Game.h"
#include "Enemy.h"
#include "Boss.h"
#include <iostream>
#include <random>
#include <cmath> // Required for sin and cos
#include <algorithm> // Required for std::max
#include <cstdio>
#include <cstdlib> // Required for std::abs
#include <set> // Required for std::set

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
    
    // Update boss
    if (currentBoss && !currentBoss->isDead()) {
        currentBoss->update(deltaTime, playerX, playerY);
    } else if (currentBoss && currentBoss->isDead()) {
        std::cout << currentBoss->getBossName() << " has been defeated!" << std::endl;
        // Boss drops enhanced loot
        // TODO: Implement boss loot dropping
        currentBoss.reset();
    }
    
    // Check for boss spawn conditions
    checkBossSpawn(playerX, playerY);
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
    
    // TMX render path when a fixed map is loaded
    if (usePrebakedChunks && !tmxLayers.empty() && tmxWidth>0 && tmxHeight>0) {
        int cameraX, cameraY; renderer->getCamera(cameraX, cameraY);
        float z = renderer->getZoom();
        auto toScreen = [cameraX, cameraY, z](int wx, int wy) -> SDL_Rect {
            float x1 = (wx - cameraX) * z;
            float y1 = (wy - cameraY) * z;
            float x2 = (wx + 32 - cameraX) * z;
            float y2 = (wy + 32 - cameraY) * z;
            return SDL_Rect{ static_cast<int>(std::floor(x1)), static_cast<int>(std::floor(y1)),
                             std::max(1, static_cast<int>(std::floor(x2)-std::floor(x1))),
                             std::max(1, static_cast<int>(std::floor(y2)-std::floor(y1))) };
        };
        // Draw layers in order
        for (const auto& layer : tmxLayers) {
            for (int y=0; y<tmxHeight; ++y) {
                for (int x=0; x<tmxWidth; ++x) {
                    int gid = layer[y*tmxWidth + x]; if (gid<=0) continue;
                    // Find tileset
                    const TmxTilesetInfo* used = nullptr; const TmxTilesetInfo* best=nullptr;
                    for (const auto& ts : tmxTilesets) {
                        if (gid >= ts.firstGid) { if (!best || ts.firstGid > best->firstGid) best = &ts; }
                    }
                    used = best; if (!used || !used->texture || used->columns<=0) continue;
                    int localId = gid - used->firstGid; if (localId < 0) continue;
                    int sx = (localId % used->columns) * used->tileWidth;
                    int sy = (localId / used->columns) * used->tileHeight;
                    SDL_Rect src{ sx, sy, used->tileWidth, used->tileHeight };
                    SDL_Rect dst = toScreen(x*tileSize, y*tileSize);
                    SDL_RenderCopy(renderer->getSDLRenderer(), used->texture->getTexture(), &src, &dst);
                }
            }
        }
        // Also render objects/enemies as below
        // fall-through to object/enemy render; skip procedural tile pass
    } else {
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
                    if (!underworldVisuals && tileId == TILE_WATER_DEEP && deepWaterSpriteSheet && deepWaterSpriteSheet->getTexture()) {
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
                    SDL_Texture* sdlTex = nullptr;
                    if (underworldVisuals && (underworldAtlasPlatform1 || underworldAtlasPlatform2)) {
                        // Choose atlas per-neighborhood: if tile neighbors any lava, use platform2 (glow), else platform1
                        auto inb = [&](int tx, int ty){ return tx>=0 && tx<width && ty>=0 && ty<height; };
                        bool nearLava = false;
                        for (int dy=-1; dy<=1 && !nearLava; ++dy) {
                            for (int dx=-1; dx<=1; ++dx) {
                                if (dx==0 && dy==0) continue;
                                int nx = worldX + dx;
                                int ny = worldY + dy;
                                if (!inb(nx, ny)) continue;
                                if (tiles[ny][nx].id == TILE_LAVA) { nearLava = true; break; }
                            }
                        }
                        Texture* atlasTex = nearLava && underworldAtlasPlatform2 ? underworldAtlasPlatform2 : (underworldAtlasPlatform1 ? underworldAtlasPlatform1 : underworldAtlasPlatform2);
                        SDL_Rect src{0,0,32,32};
                        // Small variety using world coords
                        int idx = ((worldX * 13 + worldY * 7) & 7);
                        src.x = (idx % std::max(1, underworldAtlasCols)) * 32;
                        src.y = ((idx / std::max(1, underworldAtlasCols)) % std::max(1, underworldAtlasRows)) * 32;
                        sdlTex = atlasTex ? atlasTex->getTexture() : nullptr;
                        if (sdlTex) { SDL_RenderCopy(renderer->getSDLRenderer(), sdlTex, &src, &destRect); continue; }
                    } else {
                    if (tileId >= 0 && tileId < static_cast<int>(tileVariantTextures.size()) && !tileVariantTextures[tileId].empty()) {
                        size_t idx = static_cast<size_t>(getPreferredVariantIndex(tileId, worldX, worldY));
                        idx = std::min(idx, tileVariantTextures[tileId].size() - 1);
                        chosen = tileVariantTextures[tileId][idx];
                    } else if (tileId >= 0 && tileId < static_cast<int>(tileTextures.size())) {
                        chosen = tileTextures[tileId];
                    }
                        sdlTex = chosen ? chosen->getTexture() : nullptr;
                    }
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
            // Magnetic pickup behavior for EXP orbs
            if (object->getType() == ObjectType::EXP_ORB1 || object->getType() == ObjectType::EXP_ORB2 || object->getType() == ObjectType::EXP_ORB3) {
                // Player proximity magnetic pull
                if (renderer) {
                    // We do not have direct player pointer here; use last known camera center as a proxy for nearby pull
                }
            }
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
    
    // Render boss (screen-space culling)
    if (currentBoss && !currentBoss->isDead()) {
        float z = renderer->getZoom();
        int screenX = static_cast<int>((static_cast<int>(currentBoss->getX()) - cameraX) * z);
        int screenY = static_cast<int>((static_cast<int>(currentBoss->getY()) - cameraY) * z);
        int cullW = static_cast<int>(512 * z);
        int cullH = static_cast<int>(512 * z);
        if (screenX + cullW > -200 && screenX < 1920 + 200 &&
            screenY + cullH > -200 && screenY < 1080 + 200) {
            currentBoss->render(renderer);
            currentBoss->renderProjectiles(renderer);
        }
    }
}

void World::renderMinimap(Renderer* renderer, int x, int y, int panelWidth, int panelHeight, float playerX, float playerY) const {
    if (!renderer || panelWidth <= 0 || panelHeight <= 0) return;
    SDL_Renderer* sdl = renderer->getSDLRenderer();
    if (!sdl) return;

    SDL_SetRenderDrawBlendMode(sdl, SDL_BLENDMODE_BLEND);
    // No external border; minimap will be embedded within HUD frame
    // Inner drawing area (fit inside border)
    const int margin = 8;
    int ix = x + margin, iy = y + margin;
    int iw = std::max(1, panelWidth - margin * 2);
    int ih = std::max(1, panelHeight - margin * 2);
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
    lavaSpriteSheet = assetManager->loadSpriteSheet("assets/Underworld Tilemap/Tilesets/lava-16frames.png", 32, 32, 16, 16);
}

// (legacy isGrassAt / buildMaskFromNeighbors removed)

// (legacy getEdgeTextureForMask removed)

void World::loadTilemap(const std::string& filename) {
    // Lightweight TMX CSV loader: reads layers named "plat", "plat2" as solid ground
    // and any layer with name starting with "lava" as TILE_LAVA (non-walkable).
    // Other layers are ignored for collision but render via our normal materials.
    std::cout << "Loading TMX tilemap: " << filename << std::endl;

    // Read file into memory
    FILE* f = fopen(filename.c_str(), "rb");
    if (!f) { std::cerr << "Failed to open TMX: " << filename << std::endl; return; }
    fseek(f, 0, SEEK_END); long len = ftell(f); fseek(f, 0, SEEK_SET);
    std::string xml; xml.resize(static_cast<size_t>(len));
    size_t rd = fread(xml.data(), 1, static_cast<size_t>(len), f); (void)rd; fclose(f);

    // Crude parsing: extract width/height and tilesets/layers
    auto findAttr = [](const std::string& s, const std::string& key, size_t pos)->int{
        size_t k = s.find(key + "=\"", pos); if (k==std::string::npos) return -1; k += key.size()+2; size_t e = s.find('"', k); if (e==std::string::npos) return -1; return std::atoi(s.substr(k, e-k).c_str()); };

    // Initialize baseline grid from first matching layer dimensions
    int mapW = 0, mapH = 0;
    // Parse tilesets (supports external TSX)
    tmxTilesets.clear(); tmxLayers.clear(); tmxWidth = tmxHeight = 0;
    auto getAttr = [&](const std::string& h, const std::string& key)->std::string{
        size_t k = h.find(key+"=\""); if (k==std::string::npos) return std::string(); k += key.size()+2; size_t e = h.find('"', k); if (e==std::string::npos) return std::string(); return h.substr(k, e-k);
    };
    auto readWholeFile = [&](const std::string& path) -> std::string {
        FILE* tf = fopen(path.c_str(), "rb"); if (!tf) return std::string();
        fseek(tf, 0, SEEK_END); long L = ftell(tf); fseek(tf, 0, SEEK_SET);
        std::string out; out.resize(static_cast<size_t>(std::max<long>(L,0)));
        size_t n = fread(out.data(), 1, static_cast<size_t>(std::max<long>(L,0)), tf); (void)n; fclose(tf);
        return out;
    };
    auto joinPath = [](const std::string& base, const std::string& rel) -> std::string {
        if (rel.empty()) return rel;
        if (rel.find(":") != std::string::npos) return rel; // absolute (Windows)
        if (!rel.empty() && (rel[0]=='/' || rel[0]=='\\')) return rel;
        if (base.empty()) return rel;
        size_t slash = base.find_last_of("/\\");
        std::string dir = (slash==std::string::npos) ? std::string() : base.substr(0, slash+1);
        return dir + rel;
    };
    size_t tsPos = 0;
    while (true) {
        size_t tsi = xml.find("<tileset", tsPos); if (tsi==std::string::npos) break;
        size_t tsiEnd = xml.find('>', tsi); if (tsiEnd==std::string::npos) break;
        std::string tsHeader = xml.substr(tsi, tsiEnd - tsi + 1);
        int firstGid = std::max(0, std::atoi(getAttr(tsHeader, "firstgid").c_str()));
        std::string sourceTsx = getAttr(tsHeader, "source");
        if (!sourceTsx.empty()) {
            std::string tsxPath = joinPath(filename, sourceTsx);
            std::string tsx = readWholeFile(tsxPath);
            if (!tsx.empty()) {
                // Parse TSX header for tile size, columns, image source
                size_t hdrStart = tsx.find("<tileset"); size_t hdrEnd = tsx.find('>', hdrStart);
                std::string h = (hdrStart!=std::string::npos && hdrEnd!=std::string::npos) ? tsx.substr(hdrStart, hdrEnd-hdrStart+1) : std::string();
                int tw = std::max(1, std::atoi(getAttr(h, "tilewidth").c_str()));
                int th = std::max(1, std::atoi(getAttr(h, "tileheight").c_str()));
                int cols = std::max(0, std::atoi(getAttr(h, "columns").c_str()));
                std::string name = getAttr(h, "name");
                size_t imgS = tsx.find("<image"); size_t imgE = tsx.find('>', imgS);
                std::string ih = (imgS!=std::string::npos && imgE!=std::string::npos) ? tsx.substr(imgS, imgE-imgS+1) : std::string();
                std::string imgRel = getAttr(ih, "source");
                std::string imgPath = joinPath(tsxPath, imgRel);
                Texture* tex = nullptr;
                if (assetManager) {
                    tex = assetManager->getTexture(imgPath);
                    if (!tex) tex = assetManager->getTexture(imgRel);
                }
                TmxTilesetInfo info; info.firstGid = firstGid; info.texture = tex; info.tileWidth = tw; info.tileHeight = th; info.name = name; info.imagePath = imgRel;
                if (tex) {
                    info.columns = (cols>0? cols : std::max(1, tex->getWidth() / std::max(1, tw)));
                } else {
                    info.columns = (cols>0? cols : 0);
                }
                tmxTilesets.push_back(info);
            }
        } else {
            // Inline tileset with <image>
            size_t imgTagStart = xml.find("<image", tsiEnd);
            size_t nextTileOrTs = std::min(xml.find("<tileset", tsiEnd), xml.find("<layer", tsiEnd));
            if (imgTagStart!=std::string::npos && (nextTileOrTs==std::string::npos || imgTagStart < nextTileOrTs)) {
                size_t imgEnd = xml.find('>', imgTagStart);
                if (imgEnd!=std::string::npos) {
                    std::string imgHeader = xml.substr(imgTagStart, imgEnd - imgTagStart + 1);
                    std::string src = getAttr(imgHeader, "source");
                    TmxTilesetInfo info; info.firstGid = firstGid; info.imagePath = src; info.tileWidth = 32; info.tileHeight = 32; info.columns = 0;
                    if (assetManager) {
                        std::string fullRel = joinPath(filename, src);
                        Texture* tex = assetManager->getTexture(fullRel);
                        if (!tex) tex = assetManager->getTexture(src);
                        info.texture = tex; if (tex) info.columns = std::max(1, tex->getWidth() / info.tileWidth);
                    }
                    tmxTilesets.push_back(info);
                }
            }
        }
        tsPos = tsiEnd + 1;
    }

    // Basic CSV gather per layer (all, to honor map order)
    struct LayerData { std::string name; std::vector<int> gids; int w=0; int h=0; };
    std::vector<LayerData> layers;

    size_t pos = 0;
    while (true) {
        size_t li = xml.find("<layer", pos); if (li == std::string::npos) break;
        size_t liEnd = xml.find('>', li); if (liEnd == std::string::npos) break;
        std::string header = xml.substr(li, liEnd - li + 1);
        // name
        size_t nk = header.find("name=\""); if (nk == std::string::npos) { pos = liEnd + 1; continue; }
        nk += 6; size_t nkE = header.find('"', nk); if (nkE == std::string::npos) { pos = liEnd + 1; continue; }
        std::string lname = header.substr(nk, nkE - nk);
        int lw = findAttr(header, "width", 0);
        int lh = findAttr(header, "height", 0);
        size_t dataStart = xml.find("<data", liEnd); if (dataStart == std::string::npos) { pos = liEnd + 1; continue; }
        size_t dataTagEnd = xml.find('>', dataStart); if (dataTagEnd == std::string::npos) { pos = liEnd + 1; continue; }
        size_t dataClose = xml.find("</data>", dataTagEnd); if (dataClose == std::string::npos) { pos = liEnd + 1; continue; }
        std::string csv = xml.substr(dataTagEnd + 1, dataClose - dataTagEnd - 1);
        // Sanitize
        for (char& c : csv) { if (c=='\n' || c=='\r' || c=='\t') c = ' '; }
        // Parse CSV ints
        std::vector<int> gids; gids.reserve(std::max(0, lw*lh));
        int val = 0; bool inNum = false; bool neg = false; 
        auto flush = [&](){ if (inNum){ gids.push_back(neg?-val:val); val=0; neg=false; inNum=false; } };
        for (size_t i = 0; i < csv.size(); ++i) {
            char ch = csv[i];
            if ((ch >= '0' && ch <= '9')) { inNum = true; val = val*10 + (ch - '0'); }
            else if (ch == '-') { inNum = true; neg = true; }
            else if (ch == ',' || ch == ' ') { flush(); }
        }
        flush();

        LayerData ld; ld.name = lname; ld.gids = std::move(gids); ld.w = lw; ld.h = lh;
        layers.push_back(std::move(ld));
        if (mapW == 0 && lw > 0) { mapW = lw; mapH = lh; }
        pos = dataClose + 7;
    }

    if (mapW <= 0 || mapH <= 0) { std::cerr << "TMX parse failed: width/height not found" << std::endl; return; }

    // Switch world to prebaked grid sized to TMX
    width = mapW; height = mapH; tileSize = 32; // TMX is 32px tiles in our assets
    tiles.assign(height, std::vector<Tile>(width, Tile(TILE_STONE, true, true)));
    visibleTiles.assign(height, std::vector<bool>(width, true));
    exploredTiles.assign(height, std::vector<bool>(width, true));
    usePrebakedChunks = true; visibleChunks.clear(); chunks.clear();
    mapChunkCols = (width + tileGenConfig.chunkSize - 1) / tileGenConfig.chunkSize;
    mapChunkRows = (height + tileGenConfig.chunkSize - 1) / tileGenConfig.chunkSize;

    auto applyLayer = [&](const LayerData& L){
        auto lower = L.name; for (auto& c: lower) c = static_cast<char>(::tolower(c));
        bool isLava = (lower == "lava" || lower == "lava river"); // Only actual lava, not "lava passage" which might be decorative
        bool isGround = (lower == "plat" || lower == "plat2" || lower.find("ground") != std::string::npos);
        bool isStairs = (lower.find("stairs") != std::string::npos);
        bool isPlatform = (lower == "plat" || lower == "plat2" || lower == "platform1" || lower == "platform2");
        
        // Process all relevant layers
        if (!isLava && !isGround && !isStairs && !isPlatform) return;
        if (static_cast<int>(L.gids.size()) != L.w * L.h) return;
        
        for (int y = 0; y < L.h; ++y) {
            for (int x = 0; x < L.w; ++x) {
                int gid = L.gids[y * L.w + x];
                if (gid == 0) continue; // empty cell
                if (isLava) {
                    tiles[y][x] = Tile(TILE_LAVA, false, true);
                } else if (isGround || isStairs || isPlatform) {
                    // Default underworld ground: stone (walkable)
                    tiles[y][x] = Tile(TILE_STONE, true, true);
                }
            }
        }
    };
    for (const auto& L : layers) applyLayer(L);

    // Store TMX layers and size for exact draw
    tmxWidth = width; tmxHeight = height;
    tmxLayers.clear();
    for (const auto& L : layers) {
        tmxLayers.push_back(L.gids);
    }
    // Build platform/stairs masks
    platformMask.assign(height, std::vector<bool>(width,false));
    platform1Mask.assign(height, std::vector<bool>(width,false));
    platform2Mask.assign(height, std::vector<bool>(width,false));
    stairsMask.assign(height, std::vector<bool>(width,false));
    edgeMask.assign(height, std::vector<bool>(width,false));
    lavaMask.assign(height, std::vector<bool>(width,false));
    for (const auto& L : layers) {
        auto lower = L.name; for (auto& c: lower) c = static_cast<char>(::tolower(c));
        std::cout << "Processing layer: " << L.name << " (lowercase: " << lower << ")" << std::endl;
        bool isPlat1 = (lower == "plat" || lower == "platform1");
        bool isPlat2 = (lower == "plat2" || lower == "platform2");
        bool isPlat = isPlat1 || isPlat2;
        bool isStairs = (lower.find("stairs") != std::string::npos);
        bool isEdge = (lower.find("floating land") != std::string::npos || lower.find("floating_land") != std::string::npos || lower.find("floating") != std::string::npos);
        bool isLava = (lower == "lava" || lower == "lava river"); // Be specific about lava layers
        if (isStairs) std::cout << "  -> Identified as stairs layer!" << std::endl;
        if (!(isPlat || isStairs || isEdge || isLava)) continue;
        if (static_cast<int>(L.gids.size()) != L.w*L.h) continue;
        for (int y=0; y<L.h; ++y) {
            for (int x=0; x<L.w; ++x) {
                int gid = L.gids[y*L.w + x];
                if (gid == 0) continue;
                if (isPlat) platformMask[y][x] = true;
                if (isPlat1) platform1Mask[y][x] = true;
                if (isPlat2) platform2Mask[y][x] = true;
                if (isStairs) stairsMask[y][x] = true;
                if (isEdge) edgeMask[y][x] = true;
                if (isLava) lavaMask[y][x] = true;
            }
        }
    }
    // First, set walkability based on tile type
    // IMPORTANT: We need to distinguish between actual lava tiles and other tiles
    // The lava layer should only mark real lava, not platforms or other tiles
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Only mark as lava if it's in the lava layer AND not a platform/stairs tile
            if (lavaMask[y][x] && !platformMask[y][x] && !stairsMask[y][x]) {
                // This is actual lava - mark as hazard
                tiles[y][x].walkable = false;
                tiles[y][x].id = TILE_LAVA;
            } else {
                // All other tiles are walkable by default (will be modified by edge detection)
                tiles[y][x].walkable = true;
                // Keep the original tile type, don't override it
            }
        }
    }

    // Now block specific edge tiles that are visual ledge faces
    // Keep this list minimal to avoid invisible walls. Only include the
    // obvious rocky faces that are never part of flat walkable top surfaces.
    std::set<int> edgeTileGids = {
        32, 36,  // vertical faces (left/right)
        48, 52,  // vertical faces (inner variants / with shadows)
        65, 67   // a couple of bottom-row rocky face variants
    };
    
    // Block tiles that are specifically marked as edges or visual ledge faces
    // Only check platform layers for edge tiles
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Skip if already non-walkable (like lava)
            if (!tiles[y][x].walkable) continue;
            
            // Only check platform tiles for edges
            if (!platformMask[y][x]) continue;
            
            // Heuristic 1: adjacency-based south cliff face detection.
            // If this platform tile has platform directly above but NOT below,
            // it visually represents a vertical face we should not stand on.
            // We detect this separately for platform1 and platform2 so each
            // level's ledges are blocked correctly.
            // IMPORTANT: never block tiles that are part of or immediately adjacent
            // to a staircase. Those tiles must remain walkable so players can
            // step on/off the stairs cleanly at the top and bottom.
            bool nearStairs = stairsMask[y][x]
                || (y > 0 && stairsMask[y - 1][x])
                || (y + 1 < height && stairsMask[y + 1][x])
                || (x > 0 && stairsMask[y][x - 1])
                || (x + 1 < width && stairsMask[y][x + 1]);
            bool hasAbove = (y > 0) && platformMask[y - 1][x];
            bool hasBelow = (y + 1 < height) && platformMask[y + 1][x];
            bool hasLeft  = (x > 0) && platformMask[y][x - 1];
            bool hasRight = (x + 1 < width) && platformMask[y][x + 1];
            bool hasAboveP1 = (y > 0) && platform1Mask[y - 1][x];
            bool hasBelowP1 = (y + 1 < height) && platform1Mask[y + 1][x];
            bool hasLeftP1  = (x > 0) && platform1Mask[y][x - 1];
            bool hasRightP1 = (x + 1 < width) && platform1Mask[y][x + 1];
            bool hasAboveP2 = (y > 0) && platform2Mask[y - 1][x];
            bool hasBelowP2 = (y + 1 < height) && platform2Mask[y + 1][x];
            bool hasLeftP2  = (x > 0) && platform2Mask[y][x - 1];
            bool hasRightP2 = (x + 1 < width) && platform2Mask[y][x + 1];
			// Heuristic 1a: south cliff face via adjacency.
			// If there is platform directly above but NOT below, this tile visually
			// represents the front face of a ledge. Block it (except near stairs).
			if (!nearStairs && ((hasAbove && !hasBelow) || (hasAboveP1 && !hasBelowP1) || (hasAboveP2 && !hasBelowP2))) {
				tiles[y][x].walkable = false;
				edgeMask[y][x] = true;
				continue; // already blocked by adjacency rule
			}

            // Heuristic 1b: left/right vertical face detection.
            // A tile that has platform above, and platform on one horizontal side but not the other,
            // is part of a vertical cliff face. Do not allow standing on these.
            if (!nearStairs) {
                bool sideFaceAny = (hasAbove && ((hasRight && !hasLeft) || (hasLeft && !hasRight)));
                bool sideFaceP1  = (hasAboveP1 && ((hasRightP1 && !hasLeftP1) || (hasLeftP1 && !hasRightP1)));
                bool sideFaceP2  = (hasAboveP2 && ((hasRightP2 && !hasLeftP2) || (hasLeftP2 && !hasRightP2)));
                if (sideFaceAny || sideFaceP1 || sideFaceP2) {
                    tiles[y][x].walkable = false;
                    edgeMask[y][x] = true;
                    continue;
                }
            }

            // Heuristic 2: explicit GID-based detection for special side pieces.
            // Get the GID for this position from platform layers only
            int layerIndex = 0;
            for (const auto& L : layers) {
                auto lower = L.name; 
                for (auto& c: lower) c = static_cast<char>(::tolower(c));
                bool isPlatformLayer = (lower == "plat" || lower == "plat2" || 
                                      lower == "platform" || lower == "platform1" || 
                                      lower == "platform2");
                
                if (isPlatformLayer && layerIndex < tmxLayers.size()) {
                    int gid = tmxLayers[layerIndex][y * tmxWidth + x];
                    if (gid > 0) {
                        // Map GID -> local tileset ID using the tileset with the
                        // largest firstGid that is <= gid. This avoids accidental
                        // matches from earlier tilesets and makes localGid stable.
                        int localGid = -1;
                        int chosenFirst = -1;
                        for (const auto& ts : tmxTilesets) {
                            if (gid >= ts.firstGid && ts.firstGid > chosenFirst) {
                                chosenFirst = ts.firstGid;
                            }
                        }
                        if (chosenFirst >= 0) {
                            localGid = gid - chosenFirst;
                        }
                        
                        if (localGid >= 0 && edgeTileGids.count(localGid)) {
                            // Only treat as a true edge when adjacency also suggests
                            // a vertical face and it's not near stairs. Avoid marking
                            // flat top surfaces as edges.
                            bool adjacencyLooksLikeFace = (hasAbove && !hasBelow)
                                || (hasAbove && ((hasRight && !hasLeft) || (hasLeft && !hasRight)))
                                || (hasAboveP1 && ((hasRightP1 && !hasLeftP1) || (hasLeftP1 && !hasRightP1)))
                                || (hasAboveP2 && ((hasRightP2 && !hasLeftP2) || (hasLeftP2 && !hasRightP2)));
                            if (!nearStairs && adjacencyLooksLikeFace) {
                                tiles[y][x].walkable = false;
                                edgeMask[y][x] = true;
                            }
                        }
                    }
                }
                layerIndex++;
            }
        }
    }
    
    // Note: We're not blocking tiles adjacent to lava anymore - let players get close to danger!

    // Debug: count walkable tiles and show breakdown
    int walkableCount = 0;
    int lavaCount = 0;
    int edgeBlockedCount = 0;
    int floatingLandCount = 0;
    int platformCount = 0;
    int stairsCount = 0;
    int emptyCount = 0;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (platformMask[y][x]) platformCount++;
            if (stairsMask[y][x]) stairsCount++;
            
            if (tiles[y][x].walkable) {
                walkableCount++;
            } else {
                if (tiles[y][x].id == TILE_LAVA) lavaCount++;
                else if (edgeMask[y][x]) floatingLandCount++;
                // Count other non-walkable tiles
                else edgeBlockedCount++;
            }
        }
    }
    std::cout << "Underworld walkability: " << walkableCount << " walkable, " 
              << lavaCount << " lava, " << edgeBlockedCount << " edge-blocked, " 
              << floatingLandCount << " floating-land, " << emptyCount << " empty out of " << (width * height) << " total" << std::endl;
    std::cout << "Platform tiles: " << platformCount << ", Stairs tiles: " << stairsCount << std::endl;
    
    // Debug: print first few stairs locations
    if (stairsCount > 0) {
        std::cout << "First few stairs locations: ";
        int printCount = 0;
        for (int y = 0; y < height && printCount < 10; ++y) {
            for (int x = 0; x < width && printCount < 10; ++x) {
                if (stairsMask[y][x]) {
                    std::cout << "(" << x << "," << y << ") ";
                    printCount++;
                }
            }
        }
        std::cout << std::endl;
    }

    // Switch to underworld visual set using atlas
    underworldVisuals = true;
    underworldAtlasPlatform1 = nullptr;
    underworldAtlasPlatform2 = nullptr;
    underworldAtlasCols = underworldAtlasRows = 0;
    if (assetManager) {
        // Load both platform atlases (same layout; platform2 includes glow-integration)
        underworldAtlasPlatform1 = assetManager->getTexture("assets/Underworld Tilemap/Tilesets/platform1.png");
        underworldAtlasPlatform2 = assetManager->getTexture("assets/Underworld Tilemap/Tilesets/platform2.png");
        Texture* anyAtlas = underworldAtlasPlatform1 ? underworldAtlasPlatform1 : underworldAtlasPlatform2;
        if (!anyAtlas) anyAtlas = assetManager->getTexture("assets/Underworld Tilemap/Tilesets/tileset.png");
        if (anyAtlas) { underworldAtlasCols = std::max(1, anyAtlas->getWidth() / 32); underworldAtlasRows = std::max(1, anyAtlas->getHeight() / 32); }
        // Swap lava sheet to the underworld version if available
        lavaSpriteSheet = assetManager->loadSpriteSheet("assets/Underworld Tilemap/Tilesets/lava-16frames.png", 32, 32, 16, 16);
    }
    std::cout << "TMX loaded: " << width << "x" << height << " tiles. Lava tiles and ground applied (Underworld visuals)." << std::endl;
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

bool World::isLedgeBlockedVertical(int fromTileX, int fromTileY, int toTileX, int toTileY) const {
    // Only for TMX underworld with platform/stairs masks
    if (!usePrebakedChunks || platformMask.empty()) return false;
    // Vertical move only
    if (fromTileX != toTileX || std::abs(toTileY - fromTileY) != 1) return false;
    // If either origin or destination is out of bounds, do not restrict
    if (fromTileY < 0 || fromTileY >= static_cast<int>(platformMask.size()) || fromTileX < 0 || fromTileX >= static_cast<int>(platformMask[0].size())) return false;
    if (toTileY < 0 || toTileY >= static_cast<int>(platformMask.size()) || toTileX < 0 || toTileX >= static_cast<int>(platformMask[0].size())) return false;
    
    bool fromPlat = platformMask[fromTileY][fromTileX];
    bool toPlat = platformMask[toTileY][toTileX];
    
    // Check if this is a ledge crossing that should be blocked
    // A ledge crossing happens when moving between different platform heights
    
    // First, always block movement into non-walkable tiles
    if (!tiles[toTileY][toTileX].walkable) {
        return true;
    }
    
    // Check if either position has stairs - if so, always allow movement
    bool stairAtOrigin = stairsMask[fromTileY][fromTileX];
    bool stairAtDest = stairsMask[toTileY][toTileX];
    if (stairAtOrigin || stairAtDest) {
        return false; // Always allow movement on/to/from stairs
    }
    
    // Only block actual elevation changes (non-platform -> platform when going up,
    // platform -> non-platform when going down). Allow movement within the same
    // platform area so players can move freely on flat surfaces.
    // Additionally, if trying to step onto a tile we have marked as an edge face
    // by adjacency or GID analysis, block it.
    
    // Moving up (negative Y direction)
    if (toTileY < fromTileY) {
        if (!fromPlat && toPlat) {
            return true; // climbing up a ledge without stairs
        }
    }
    
    // Moving down (positive Y direction)
    if (toTileY > fromTileY) {
        if (fromPlat && !toPlat) {
            return true; // dropping down a ledge without stairs
        }
    }

    // Final guard: if destination tile is explicitly flagged as an edge, block.
    if (edgeMask[toTileY][toTileX]) return true;

    // Note: Do NOT hard-block vertical transitions between platform1 and platform2
    // here. Treat platform1/2 differences as cosmetic within the top surface.
    // True elevation changes are already caught above (non-platform <-> platform)
    // and by ledge-crossing logic which consults edge markers and stairs.
    
    return false; // Allow horizontal movements on same level
}

bool World::isLedgeCrossingBlocked(int fromTileX, int fromTileY, int toTileX, int toTileY) const {
    if (!usePrebakedChunks || platformMask.empty()) return false;
    // Any move that changes platformMask state must go through a stairs cell
    auto inb = [&](int x, int y){ return x>=0 && x<width && y>=0 && y<height; };
    if (!inb(fromTileX,fromTileY) || !inb(toTileX,toTileY)) return false;
    if (fromTileX == toTileX && fromTileY == toTileY) return false;
    bool fromPlat = platformMask[fromTileY][fromTileX];
    bool toPlat = platformMask[toTileY][toTileX];
    bool stairHere = stairsMask[fromTileY][fromTileX] || stairsMask[toTileY][toTileX];

    // Helper: when making a HORIZONTAL step at the top or bottom of a staircase,
    // allow the transition if the destination or origin has a staircase directly
    // above or below it. This lets you step off/on the staircase onto the platform.
    bool isHorizontal = (fromTileY == toTileY) && (std::abs(toTileX - fromTileX) == 1);
    bool isVertical   = (fromTileX == toTileX) && (std::abs(toTileY - fromTileY) == 1);
    bool stairAdjFrom = false, stairAdjTo = false;
    if (isHorizontal) {
        if (inb(fromTileX, fromTileY - 1)) stairAdjFrom |= stairsMask[fromTileY - 1][fromTileX];
        if (inb(fromTileX, fromTileY + 1)) stairAdjFrom |= stairsMask[fromTileY + 1][fromTileX];
        if (inb(toTileX, toTileY - 1))     stairAdjTo   |= stairsMask[toTileY - 1][toTileX];
        if (inb(toTileX, toTileY + 1))     stairAdjTo   |= stairsMask[toTileY + 1][toTileX];
    }
    // Do not relax vertical ledge transitions via side-adjacent stairs here.
    // Vertical movement is handled by isLedgeBlockedVertical (with edge guard).

    // Case 1: Crossing between non-platform and platform (any direction)
    auto nearStairsAt = [&](int tx, int ty){
        if (!inb(tx,ty)) return false;
        if (stairsMask[ty][tx]) return true;
        if (inb(tx,ty-1) && stairsMask[ty-1][tx]) return true;
        if (inb(tx,ty+1) && stairsMask[ty+1][tx]) return true;
        if (inb(tx-1,ty) && stairsMask[ty][tx-1]) return true;
        if (inb(tx+1,ty) && stairsMask[ty][tx+1]) return true;
        return false;
    };
    bool edgeFrom = edgeMask[fromTileY][fromTileX] && !nearStairsAt(fromTileX, fromTileY);
    bool edgeTo   = edgeMask[toTileY][toTileX]     && !nearStairsAt(toTileX, toTileY);
    bool edgeHere = edgeFrom || edgeTo;
    if (fromPlat != toPlat) {
        if (stairHere) return false;
        if ((isHorizontal || isVertical) && (stairAdjFrom || stairAdjTo)) return false;
        if (!edgeHere) return false;
        return true;
    }

    // Case 2: Crossing between different platform levels (platform1 <-> platform2)
    // Even though both are platform, treat them as different elevations that require stairs
    bool fromP1 = platform1Mask[fromTileY][fromTileX];
    bool toP1   = platform1Mask[toTileY][toTileX];
    bool fromP2 = platform2Mask[fromTileY][fromTileX];
    bool toP2   = platform2Mask[toTileY][toTileX];
    bool crossingBetweenLevels = (fromP1 && toP2) || (fromP2 && toP1);
    if (crossingBetweenLevels) {
        if (stairHere) return false;
        if ((isHorizontal || isVertical) && (stairAdjFrom || stairAdjTo)) return false;
        if (!edgeHere) return false;
        return true;
    }

    return false;
}



void World::addObject(std::unique_ptr<Object> object) {
    if (object) {
        objects.push_back(std::move(object));
    }
}

void World::clearObjects() {
    objects.clear();
}

void World::addEnemy(std::unique_ptr<Enemy> enemy) {
    if (enemy) {
        enemies.push_back(std::move(enemy));
    }
}

void World::removeObject(int x, int y) {
    auto it = std::remove_if(objects.begin(), objects.end(),
        [x, y](const std::unique_ptr<Object>& obj) {
            if (!obj) return false;
            // Magic Anvil is indestructible
            if (obj->getType() == ObjectType::MAGIC_ANVIL) return false;
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

// (legacy shouldPlaceStone / shouldPlaceStoneGrass removed)

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
    
    // (legacy weight validation removed)
    
    // Initialize tile grid
    tiles.resize(height, std::vector<Tile>(width));
    
    std::cout << "Generating tilemap: " << width << "x" << height << " tiles" << std::endl;
    std::cout << "Tile weights configured; using biome-based generation with transition buffers." << std::endl;
    
    // Generate tiles based on configuration
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int tileType;
            // Always use noise-based generation (legacy weighted path removed)
            tileType = generateNoiseBasedTileType(x, y);
            
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

// (legacy generateWeightedTileType removed)

int World::generateNoiseBasedTileType(int x, int y) {
    // Generate noise value for this position
    float noiseValue = generateNoise(x * tileGenConfig.noiseScale, y * tileGenConfig.noiseScale);
    
    // Use noise to determine tile type with weighted distribution
    float normalizedNoise = (noiseValue + 1.0f) / 2.0f; // Normalize to 0-1
    
    // Fixed thresholds (legacy weight-based system removed)
    const float stoneThreshold = 0.4f;
    const float grassThreshold = 0.7f;
    
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
    
    return static_cast<float>(noise / (1.0 - std::pow(persistence, static_cast<double>(octaves))));
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

// (legacy validateTileWeights removed)

void World::printTileDistribution() {
    int grassCount = 0, stoneCount = 0;
    // (legacy stoneGrassCounts removed)
    
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
    
    if (usePrebakedChunks) {
        // Copy tiles directly from the loaded TMX grid; out-of-bounds fill with stone
        for (int y = 0; y < chunkSize; y++) {
            for (int x = 0; x < chunkSize; x++) {
                int wx = worldStartX + x;
                int wy = worldStartY + y;
                if (wx >= 0 && wx < width && wy >= 0 && wy < height) {
                    chunk->tiles[y][x] = tiles[wy][wx];
                } else {
                    chunk->tiles[y][x] = Tile(TILE_STONE, true, true);
                }
            }
        }
        return;
    }

    // Procedural generation per tile from biomes
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

int World::pickBaseMaterialForGroup(int groupId, float /*noiseVal*/) const {
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

int World::getPreferredVariantIndex(int /*tileType*/, int worldX, int worldY) const {
    // Prefer variants 06-08 using very low-frequency noise to form large blobs.
    float base = tileGenConfig.regionNoiseScale * 0.75f;
    float f = (generateNoise(static_cast<float>(worldX) * base, static_cast<float>(worldY) * base) + 1.0f) * 0.5f; // 0..1
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

// (legacy carveRandomGrassPatches removed)

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
    if (usePrebakedChunks) {
        // For prebaked maps, synthesize flat chunks that simply reference the global tiles
        visibleChunks.clear();
        for (int cy = playerChunkY - renderDistance; cy <= playerChunkY + renderDistance; cy++) {
            for (int cx = playerChunkX - renderDistance; cx <= playerChunkX + renderDistance; cx++) {
                // Clamp to map chunk grid
                Chunk* c = nullptr;
                if (cx >= 0 && cy >= 0 && cx < mapChunkCols && cy < mapChunkRows) {
                    generateChunk(cx, cy);
                    c = getChunk(cx, cy);
                } else {
                    generateChunk(cx, cy);
                    c = getChunk(cx, cy);
                }
                if (c) { c->isVisible = true; visibleChunks.push_back(c); }
            }
        }
    } else {
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

// (unused generateBiomeBasedTile removed)

// Boss management methods
void World::spawnBoss(BossType bossType, float x, float y) {
    if (currentBoss) {
        std::cout << "Warning: Attempting to spawn boss while another boss exists!" << std::endl;
        return;
    }
    
    currentBoss = std::make_unique<Boss>(x, y, assetManager, bossType);
    bossSpawned = true;
    
    std::cout << "Boss spawned: " << currentBoss->getBossName() << " at (" << x << ", " << y << ")" << std::endl;
}

void World::checkBossSpawn(float playerX, float playerY) {
    // Only spawn boss once per session for now
    if (bossSpawned) return;
    
    // Simple boss spawn trigger: when player reaches certain areas or after killing enough enemies
    int playerTileX = static_cast<int>(playerX / tileSize);
    int playerTileY = static_cast<int>(playerY / tileSize);
    
    // Example: Spawn boss when player reaches coordinates (30, 30) or similar trigger
    if ((playerTileX > 25 && playerTileX < 35 && playerTileY > 25 && playerTileY < 35) ||
        (enemies.empty() && rand() % 1000 < 2)) { // 0.2% chance per frame when no enemies
        
        // Choose random boss type
        BossType bossTypes[] = {BossType::DEMON_LORD, BossType::ANCIENT_WIZARD, BossType::GOBLIN_KING};
        BossType chosenType = bossTypes[rand() % 3];
        
        // Spawn boss at a safe distance from player
        float bossX = playerX + ((rand() % 2 == 0) ? 400 : -400);
        float bossY = playerY + ((rand() % 2 == 0) ? 400 : -400);
        
        // Ensure boss spawns on walkable terrain
        int bossTileX = static_cast<int>(bossX / tileSize);
        int bossTileY = static_cast<int>(bossY / tileSize);
        
        if (isWalkable(bossTileX, bossTileY)) {
            spawnBoss(chosenType, bossX, bossY);
        }
    }
}
