#include "World.h"
#include "Renderer.h"
#include "AssetManager.h"
#include "Object.h"
#include <iostream>
#include <random>
#include <cmath> // Required for sin and cos
#include <algorithm> // Required for std::max
#include <cstdlib> // Required for std::abs

World::World() : width(50), height(50), tileSize(32), tilesetTexture(nullptr), assetManager(nullptr), rng(std::random_device{}()), visibilityRadius(50), fogOfWarEnabled(true) {
    // Initialize default tile generation config
    tileGenConfig.worldWidth = width;
    tileGenConfig.worldHeight = height;
    initializeDefaultWorld();
}

World::World(AssetManager* assetManager) : width(50), height(50), tileSize(32), tilesetTexture(nullptr), assetManager(assetManager), rng(std::random_device{}()), visibilityRadius(50), fogOfWarEnabled(false) {
    // Initialize default tile generation config
    tileGenConfig.worldWidth = width;
    tileGenConfig.worldHeight = height;
    loadTileTextures();
    initializeDefaultWorld();
}

void World::initializeDefaultWorld() {
    // Initialize tile grid
    tiles.resize(height, std::vector<Tile>(width));
    
    // Initialize visibility arrays
    visibleTiles.resize(height, std::vector<bool>(width, false));
    exploredTiles.resize(height, std::vector<bool>(width, false));
    
    // Use the new tilemap generation system with default configuration
    TileGenerationConfig defaultConfig;
    defaultConfig.worldWidth = width;
    defaultConfig.worldHeight = height;
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
    defaultConfig.useNoiseDistribution = false;
    defaultConfig.enableStoneClustering = true;
    
    generateTilemap(defaultConfig);
    
    // Validate that all tiles have valid IDs after generation
    std::cout << "Validating tile array after generation..." << std::endl;
    int invalidTileCount = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (tiles[y][x].id < 0 || tiles[y][x].id > TILE_WATER) {
                invalidTileCount++;
                std::cout << "ERROR: Invalid tile ID " << tiles[y][x].id << " at position (" << x << ", " << y << ") after generation!" << std::endl;
                // Fix invalid tiles immediately
                tiles[y][x].id = TILE_GRASS;
            }
        }
    }
    
    if (invalidTileCount > 0) {
        std::cout << "WARNING: Fixed " << invalidTileCount << " invalid tile IDs during initialization!" << std::endl;
    } else {
        std::cout << "All tiles have valid IDs after generation." << std::endl;
    }
    
    std::cout << "World initialized: " << width << "x" << height << " tiles with weighted tile generation" << std::endl;
    std::cout << "Fog of war enabled with visibility radius: " << visibilityRadius << " tiles" << std::endl;
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
    
    // Get camera position from renderer
    int cameraX, cameraY;
    renderer->getCamera(cameraX, cameraY);
    
    // OPTIMIZATION: Only render tiles that are visible on screen
    // Calculate visible tile range based on camera position - be very generous to prevent black areas
    int startX = std::max(0, cameraX / tileSize - 5);
    int endX = std::min(width - 1, (cameraX + 1920) / tileSize + 5);
    int startY = std::max(0, cameraY / tileSize - 5);
    int endY = std::min(height - 1, (cameraY + 1080) / tileSize + 5);
    
    // Debug: Count tile types (only once every 300 frames to reduce spam)
    static int debugFrameCount = 0;
    if (debugFrameCount++ % 300 == 0) {
        std::cout << "Rendering tiles in range (" << startX << "," << startY << ") to (" << endX << "," << endY << ")" << std::endl;
        
        // Debug: Count tile types being rendered
        int grassCount = 0, stoneCount = 0, stoneGrassTotalCount = 0, invalidCount = 0;
        int stoneGrassCounts[19] = {0}; // Count all 19 stone_grass variants
        
        for (int y = startY; y <= endY; y++) {
            for (int x = startX; x <= endX; x++) {
                int tileId = tiles[y][x].id;
                if (tileId == TILE_GRASS) {
                    grassCount++;
                } else if (tileId == TILE_STONE) {
                    stoneCount++;
                } else if (tileId >= TILE_STONE_GRASS && tileId <= TILE_STONE_GRASS_19) {
                    stoneGrassTotalCount++;
                    int variantIndex = tileId - TILE_STONE_GRASS;
                    if (variantIndex >= 0 && variantIndex < 19) {
                        stoneGrassCounts[variantIndex]++;
                    }
                } else {
                    invalidCount++;
                }
            }
        }
        
        std::cout << "Tile types in render range - Grass: " << grassCount << ", Stone: " << stoneCount 
                  << ", StoneGrass Total: " << stoneGrassTotalCount << ", Invalid: " << invalidCount << std::endl;
        
        // Show individual stone_grass variant counts
        for (int i = 0; i < 19; i++) {
            if (stoneGrassCounts[i] > 0) {
                std::cout << "  StoneGrass" << (i + 1) << ": " << stoneGrassCounts[i] << std::endl;
            }
        }
    }
    
    // Render only visible tiles
    int renderedTiles = 0;
    int visibleTilesCount = 0;
    
    for (int y = startY; y <= endY; y++) {
        for (int x = startX; x <= endX; x++) {
            int tileId = tiles[y][x].id;
            
            // Validate tile ID to catch any invalid values
            if (tileId < 0 || tileId > TILE_WATER) {
                std::cout << "ERROR: Invalid tile ID " << tileId << " at position (" << x << ", " << y << "). Setting to grass." << std::endl;
                tileId = TILE_GRASS;
                tiles[y][x].id = TILE_GRASS; // Fix the invalid tile
            }
            
            // Check visibility - render all tiles, but apply fog of war effect if enabled
            bool isVisible = true;
            bool isExplored = true;
            
            if (fogOfWarEnabled) {
                isVisible = visibleTiles[y][x];
                isExplored = exploredTiles[y][x];
            }
            
            // Calculate world position
            int worldX = x * tileSize;
            int worldY = y * tileSize;
            
            // Calculate screen position (with camera offset)
            int screenX = worldX - cameraX;
            int screenY = worldY - cameraY;
            
            // Check if the tile is visible on screen (with very generous margin to prevent black areas)
            if (screenX + tileSize > -200 && screenX < 1920 + 200 && 
                screenY + tileSize > -200 && screenY < 1080 + 200) {
                
                renderedTiles++;
                if (isVisible) {
                    visibleTilesCount++;
                }
                
                // Check if we have a texture for this tile type
                if (tileId >= 0 && tileId < tileTextures.size() && tileTextures[tileId]) {
                    // Render using texture
                    Texture* texture = tileTextures[tileId];
                    if (texture && texture->getTexture()) {
                        SDL_Rect destRect = {
                            screenX,
                            screenY,
                            tileSize,
                            tileSize
                        };
                        
                        // Set the blend mode to ensure proper rendering
                        SDL_SetTextureBlendMode(texture->getTexture(), SDL_BLENDMODE_BLEND);
                        
                        // Apply fog of war effect - darken explored but not visible tiles
                        if (fogOfWarEnabled && isExplored && !isVisible) {
                            // Darken the texture for explored but not visible tiles
                            SDL_SetTextureColorMod(texture->getTexture(), 100, 100, 100);
                        } else if (fogOfWarEnabled && !isExplored) {
                            // Very dark for unexplored tiles
                            SDL_SetTextureColorMod(texture->getTexture(), 50, 50, 50);
                        } else {
                            // Reset color modulation for visible tiles
                            SDL_SetTextureColorMod(texture->getTexture(), 255, 255, 255);
                        }
                        
                        // Render the texture
                        int result = SDL_RenderCopy(renderer->getSDLRenderer(), texture->getTexture(), nullptr, &destRect);
                        if (result != 0) {
                            // If rendering failed, fall back to colored rectangle
                            TileColor tileColor = getTileColor(tileId);
                            SDL_Color color = {tileColor.r, tileColor.g, tileColor.b, tileColor.a};
                            
                            // Apply fog of war effect to colored rectangles too
                            if (fogOfWarEnabled && isExplored && !isVisible) {
                                color.r = color.r / 2;
                                color.g = color.g / 2;
                                color.b = color.b / 2;
                            } else if (fogOfWarEnabled && !isExplored) {
                                color.r = color.r / 4;
                                color.g = color.g / 4;
                                color.b = color.b / 4;
                            }
                            
                            renderer->renderRect(screenX, screenY, tileSize, tileSize, color, true);
                        }
                    } else {
                        // Fallback to colored rectangles if texture is invalid
                        TileColor tileColor = getTileColor(tileId);
                        SDL_Color color = {tileColor.r, tileColor.g, tileColor.b, tileColor.a};
                        
                        // Apply fog of war effect to colored rectangles
                        if (fogOfWarEnabled && isExplored && !isVisible) {
                            color.r = color.r / 2;
                            color.g = color.g / 2;
                            color.b = color.b / 2;
                        } else if (fogOfWarEnabled && !isExplored) {
                            color.r = color.r / 4;
                            color.g = color.g / 4;
                            color.b = color.b / 4;
                        }
                        
                        renderer->renderRect(screenX, screenY, tileSize, tileSize, color, true);
                    }
                } else {
                    // Fallback to colored rectangles if texture not available
                    // Additional safety check for tile ID bounds
                    if (tileId < 0 || tileId >= tileTextures.size()) {
                        std::cout << "ERROR: Tile ID " << tileId << " is out of bounds! tileTextures.size() = " << tileTextures.size() << std::endl;
                        tileId = TILE_GRASS; // Use grass as fallback
                    }
                    
                    TileColor tileColor = getTileColor(tileId);
                    SDL_Color color = {tileColor.r, tileColor.g, tileColor.b, tileColor.a};
                    
                    // Apply fog of war effect - darken explored but not visible tiles
                    if (fogOfWarEnabled && isExplored && !isVisible) {
                        color.r = color.r / 2;
                        color.g = color.g / 2;
                        color.b = color.b / 2;
                    } else if (fogOfWarEnabled && !isExplored) {
                        color.r = color.r / 4;
                        color.g = color.g / 4;
                        color.b = color.b / 4;
                    }
                    
                    renderer->renderRect(screenX, screenY, tileSize, tileSize, color, true);
                }
            }
        }
    }
    
    // Debug output (only print once every 300 frames to avoid spam)
    if (debugFrameCount % 300 == 0) {
        std::cout << "Rendered tiles: " << renderedTiles << " out of " << ((endX - startX + 1) * (endY - startY + 1)) << " visible tiles" << std::endl;
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
        
        // Check if object is visible on screen (with very generous margin)
        if (objScreenX + tileSize > -200 && objScreenX < 1920 + 200 && 
            objScreenY + tileSize > -200 && objScreenY < 1080 + 200) {
            object->render(renderer->getSDLRenderer(), cameraX, cameraY, tileSize);
        }
    }
}

void World::loadTileTextures() {
    if (!assetManager) {
        std::cout << "AssetManager not available, skipping tile texture loading" << std::endl;
        return;
    }
    
    // Initialize tile textures vector
    tileTextures.resize(22, nullptr); // 22 tile types: grass, stone, 19 stone_grass variants, water
    
    // Load tile textures using AssetManager with correct paths
    std::cout << "Loading tile textures..." << std::endl;
    
    // Use the correct AssetManager paths - these should match how they're preloaded in AssetManager::preloadAssets()
    // The AssetManager uses TILESET_PATH + filename, where TILESET_PATH = "assets/FULL_Fantasy Forest/Tiles/"
    tileTextures[TILE_GRASS] = assetManager->getTexture("assets/FULL_Fantasy Forest/Tiles/grass_tile.png");
    if (!tileTextures[TILE_GRASS]) {
        std::cout << "ERROR: Failed to load grass tile texture! Will use colored rectangle fallback." << std::endl;
    } else {
        std::cout << "SUCCESS: Grass tile texture loaded successfully" << std::endl;
    }
    
    tileTextures[TILE_STONE] = assetManager->getTexture("assets/FULL_Fantasy Forest/Tiles/stone_tile.png");
    if (!tileTextures[TILE_STONE]) {
        std::cout << "ERROR: Failed to load stone tile texture! Will use colored rectangle fallback." << std::endl;
    } else {
        std::cout << "SUCCESS: Stone tile texture loaded successfully" << std::endl;
    }
    
    // Load all stone_grass_tile variants
    tileTextures[TILE_STONE_GRASS] = assetManager->getTexture("assets/FULL_Fantasy Forest/Tiles/stone_grass_tile.png");
    tileTextures[TILE_STONE_GRASS_2] = assetManager->getTexture("assets/FULL_Fantasy Forest/Tiles/stone_grass_tile_2.png");
    tileTextures[TILE_STONE_GRASS_3] = assetManager->getTexture("assets/FULL_Fantasy Forest/Tiles/stone_grass_tile_3.png");
    tileTextures[TILE_STONE_GRASS_4] = assetManager->getTexture("assets/FULL_Fantasy Forest/Tiles/stone_grass_tile_4.png");
    tileTextures[TILE_STONE_GRASS_5] = assetManager->getTexture("assets/FULL_Fantasy Forest/Tiles/stone_grass_tile_5.png");
    tileTextures[TILE_STONE_GRASS_6] = assetManager->getTexture("assets/FULL_Fantasy Forest/Tiles/stone_grass_tile_6.png");
    tileTextures[TILE_STONE_GRASS_7] = assetManager->getTexture("assets/FULL_Fantasy Forest/Tiles/stone_grass_tile_7.png");
    tileTextures[TILE_STONE_GRASS_8] = assetManager->getTexture("assets/FULL_Fantasy Forest/Tiles/stone_grass_tile_8.png");
    tileTextures[TILE_STONE_GRASS_9] = assetManager->getTexture("assets/FULL_Fantasy Forest/Tiles/stone_grass_tile_9.png");
    tileTextures[TILE_STONE_GRASS_10] = assetManager->getTexture("assets/FULL_Fantasy Forest/Tiles/stone_grass_tile_10.png");
    tileTextures[TILE_STONE_GRASS_11] = assetManager->getTexture("assets/FULL_Fantasy Forest/Tiles/stone_grass_tile_11.png");
    tileTextures[TILE_STONE_GRASS_12] = assetManager->getTexture("assets/FULL_Fantasy Forest/Tiles/stone_grass_tile_12.png");
    tileTextures[TILE_STONE_GRASS_13] = assetManager->getTexture("assets/FULL_Fantasy Forest/Tiles/stone_grass_tile_13.png");
    tileTextures[TILE_STONE_GRASS_14] = assetManager->getTexture("assets/FULL_Fantasy Forest/Tiles/stone_grass_tile_14.png");
    tileTextures[TILE_STONE_GRASS_15] = assetManager->getTexture("assets/FULL_Fantasy Forest/Tiles/stone_grass_tile_15.png");
    tileTextures[TILE_STONE_GRASS_16] = assetManager->getTexture("assets/FULL_Fantasy Forest/Tiles/stone_grass_tile_16.png");
    tileTextures[TILE_STONE_GRASS_17] = assetManager->getTexture("assets/FULL_Fantasy Forest/Tiles/stone_grass_tile_17.png");
    tileTextures[TILE_STONE_GRASS_18] = assetManager->getTexture("assets/FULL_Fantasy Forest/Tiles/stone_grass_tile_18.png");
    tileTextures[TILE_STONE_GRASS_19] = assetManager->getTexture("assets/FULL_Fantasy Forest/Tiles/stone_grass_tile_19.png");
    
    tileTextures[TILE_WATER] = nullptr; // Water not implemented yet
    
    // Debug output
    std::cout << "Tile textures loaded:" << std::endl;
    std::cout << "  Grass tile: " << (tileTextures[TILE_GRASS] ? "Loaded" : "Failed - using colored rectangle fallback") << std::endl;
    std::cout << "  Stone tile: " << (tileTextures[TILE_STONE] ? "Loaded" : "Failed - using colored rectangle fallback") << std::endl;
    
    // Check stone_grass tile loading
    int stoneGrassLoaded = 0;
    for (int i = TILE_STONE_GRASS; i <= TILE_STONE_GRASS_19; i++) {
        if (tileTextures[i]) {
            stoneGrassLoaded++;
        }
    }
    std::cout << "  Stone grass tiles: " << stoneGrassLoaded << "/19 loaded successfully" << std::endl;
    
    // Check if any textures failed to load
    int loadedCount = 0;
    for (int i = 0; i < tileTextures.size(); i++) {
        if (tileTextures[i]) {
            loadedCount++;
            // Print texture details
            std::cout << "  Texture " << i << " dimensions: " << tileTextures[i]->getWidth() << "x" << tileTextures[i]->getHeight() << std::endl;
            
            // Test if the texture is valid
            if (tileTextures[i]->getTexture()) {
                std::cout << "  Texture " << i << " SDL_Texture is valid" << std::endl;
            } else {
                std::cout << "  Texture " << i << " SDL_Texture is NULL!" << std::endl;
            }
        } else {
            std::cout << "  Texture " << i << " failed to load! Will use colored rectangle fallback." << std::endl;
        }
    }
    std::cout << "Successfully loaded " << loadedCount << " out of " << tileTextures.size() << " tile textures" << std::endl;
    
    // Additional debugging: Check what happens when we try to get textures by path
    std::cout << "Testing texture retrieval by path..." << std::endl;
    Texture* testGrass = assetManager->getTexture("assets/FULL_Fantasy Forest/Tiles/grass_tile.png");
    std::cout << "  Direct path test - grass: " << (testGrass ? "SUCCESS" : "FAILED") << std::endl;
    
    // Check if the issue is with the AssetManager paths
    std::cout << "AssetManager TILESET_PATH: " << AssetManager::TILESET_PATH << std::endl;
}

void World::loadTilemap(const std::string& filename) {
    // TODO: Implement tilemap loading from file
    std::cout << "Loading tilemap: " << filename << " (not implemented yet)" << std::endl;
}

void World::setTile(int x, int y, int tileId) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        // Validate tile ID to ensure it's within valid range
        if (tileId < 0 || tileId > TILE_WATER) {
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

int World::getPrioritizedTileType(int x, int y) {
    // New priority system with stone as highest priority:
    // 1. Stone (40% chance) - highest priority (stone_tile.png)
    // 2. Grass (20% chance) - lower priority (grass_tile.png)
    // 3. Stone with grass variants (40% total, ~2.1% each) - distributed evenly
    
    // Use position-based noise for more natural distribution
    float noise = (sin(x * 0.1f) + cos(y * 0.1f) + sin((x + y) * 0.05f)) / 3.0f;
    noise = (noise + 1.0f) / 2.0f; // Normalize to 0-1
    
    // Combine noise with random value for more natural patterns
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float randomValue = dist(rng);
    float combinedValue = (noise * 0.3f + randomValue * 0.7f);
    
    // Base probabilities - stone is now highest priority
    float stoneProb = 0.40f;
    float grassProb = 0.20f;
    float stoneGrassTotalProb = 0.40f; // Total for all stone_grass variants
    
    // Add position-based variations for more interesting patterns
    // Create some stone clusters in certain areas
    float stoneClusterNoise = sin(x * 0.05f) * cos(y * 0.05f);
    if (stoneClusterNoise > 0.7f) {
        stoneProb += 0.20f;
        grassProb -= 0.10f;
        stoneGrassTotalProb -= 0.10f;
    }
    
    // Create some stone grass transition areas
    float transitionNoise = sin((x + y) * 0.08f);
    if (transitionNoise > 0.6f && transitionNoise < 0.8f) {
        stoneGrassTotalProb += 0.15f;
        stoneProb -= 0.10f;
        grassProb -= 0.05f;
    }
    
    // Ensure probabilities stay positive and normalized
    stoneProb = std::max(0.0f, stoneProb);
    grassProb = std::max(0.0f, grassProb);
    stoneGrassTotalProb = std::max(0.0f, stoneGrassTotalProb);
    
    // Normalize probabilities
    float total = stoneProb + grassProb + stoneGrassTotalProb;
    if (total > 0.0f) {
        stoneProb /= total;
        grassProb /= total;
        stoneGrassTotalProb /= total;
    } else {
        // Fallback to default distribution
        stoneProb = 0.40f;
        grassProb = 0.20f;
        stoneGrassTotalProb = 0.40f;
    }
    
    // Apply probabilities
    if (combinedValue < stoneProb) {
        return TILE_STONE;
    } else if (combinedValue < stoneProb + grassProb) {
        return TILE_GRASS;
    } else {
        // Distribute stone_grass variants evenly
        float stoneGrassValue = (combinedValue - (stoneProb + grassProb)) / stoneGrassTotalProb;
        int stoneGrassIndex = static_cast<int>(stoneGrassValue * 19); // 19 variants
        stoneGrassIndex = std::min(stoneGrassIndex, 18); // Ensure we don't exceed array bounds
        return TILE_STONE_GRASS + stoneGrassIndex;
    }
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
                if (tileId == TILE_STONE || 
                    (tileId >= TILE_STONE_GRASS && tileId <= TILE_STONE_GRASS_19)) {
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
                if (tileId == TILE_STONE || 
                    (tileId >= TILE_STONE_GRASS && tileId <= TILE_STONE_GRASS_19)) {
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
    std::cout << "Tile weights - Stone: " << tileGenConfig.stoneWeight << "%, Grass: " << tileGenConfig.grassWeight 
              << "%, StoneGrass variants: 40% total (2% each)" << std::endl;
    
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
            if (tileType < 0 || tileType > TILE_WATER) {
                std::cout << "ERROR: Invalid tile type " << tileType << " generated at position (" << x << ", " << y << "). Setting to grass." << std::endl;
                tileType = TILE_GRASS;
            }
            
            switch (tileType) {
                case TILE_GRASS:
                    walkable = true;
                    transparent = true;
                    break;
                case TILE_STONE:
                    walkable = true;
                    transparent = true; // Stone tiles are transparent for line of sight
                    break;
                case TILE_STONE_GRASS:
                case TILE_STONE_GRASS_2:
                case TILE_STONE_GRASS_3:
                case TILE_STONE_GRASS_4:
                case TILE_STONE_GRASS_5:
                case TILE_STONE_GRASS_6:
                case TILE_STONE_GRASS_7:
                case TILE_STONE_GRASS_8:
                case TILE_STONE_GRASS_9:
                case TILE_STONE_GRASS_10:
                case TILE_STONE_GRASS_11:
                case TILE_STONE_GRASS_12:
                case TILE_STONE_GRASS_13:
                case TILE_STONE_GRASS_14:
                case TILE_STONE_GRASS_15:
                case TILE_STONE_GRASS_16:
                case TILE_STONE_GRASS_17:
                case TILE_STONE_GRASS_18:
                case TILE_STONE_GRASS_19:
                    walkable = true;
                    transparent = true;
                    break;
                case TILE_WATER:
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
    
    // Apply stone clustering if enabled
    if (tileGenConfig.enableStoneClustering) {
        applyStoneClustering();
    }
    
    // Print tile distribution for debugging
    printTileDistribution();
    
    // Additional debugging: Check for any invalid tile types
    int invalidTileCount = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (tiles[y][x].id < 0 || tiles[y][x].id > TILE_WATER) {
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
    float stoneGrass7Weight = tileGenConfig.stoneGrass7Weight;
    float stoneGrass8Weight = tileGenConfig.stoneGrass8Weight;
    float stoneGrass9Weight = tileGenConfig.stoneGrass9Weight;
    float stoneGrass10Weight = tileGenConfig.stoneGrass10Weight;
    float stoneGrass11Weight = tileGenConfig.stoneGrass11Weight;
    float stoneGrass12Weight = tileGenConfig.stoneGrass12Weight;
    float stoneGrass13Weight = tileGenConfig.stoneGrass13Weight;
    float stoneGrass14Weight = tileGenConfig.stoneGrass14Weight;
    float stoneGrass15Weight = tileGenConfig.stoneGrass15Weight;
    float stoneGrass16Weight = tileGenConfig.stoneGrass16Weight;
    float stoneGrass17Weight = tileGenConfig.stoneGrass17Weight;
    float stoneGrass18Weight = tileGenConfig.stoneGrass18Weight;
    float stoneGrass19Weight = tileGenConfig.stoneGrass19Weight;
    
    // Check each stone_grass variant in order
    cumulativeWeight += stoneGrassWeight;
    if (randomValue < cumulativeWeight) {
        return TILE_STONE_GRASS;
    }
    
    cumulativeWeight += stoneGrass2Weight;
    if (randomValue < cumulativeWeight) {
        return TILE_STONE_GRASS_2;
    }
    
    cumulativeWeight += stoneGrass3Weight;
    if (randomValue < cumulativeWeight) {
        return TILE_STONE_GRASS_3;
    }
    
    cumulativeWeight += stoneGrass4Weight;
    if (randomValue < cumulativeWeight) {
        return TILE_STONE_GRASS_4;
    }
    
    cumulativeWeight += stoneGrass5Weight;
    if (randomValue < cumulativeWeight) {
        return TILE_STONE_GRASS_5;
    }
    
    cumulativeWeight += stoneGrass6Weight;
    if (randomValue < cumulativeWeight) {
        return TILE_STONE_GRASS_6;
    }
    
    cumulativeWeight += stoneGrass7Weight;
    if (randomValue < cumulativeWeight) {
        return TILE_STONE_GRASS_7;
    }
    
    cumulativeWeight += stoneGrass8Weight;
    if (randomValue < cumulativeWeight) {
        return TILE_STONE_GRASS_8;
    }
    
    cumulativeWeight += stoneGrass9Weight;
    if (randomValue < cumulativeWeight) {
        return TILE_STONE_GRASS_9;
    }
    
    cumulativeWeight += stoneGrass10Weight;
    if (randomValue < cumulativeWeight) {
        return TILE_STONE_GRASS_10;
    }
    
    cumulativeWeight += stoneGrass11Weight;
    if (randomValue < cumulativeWeight) {
        return TILE_STONE_GRASS_11;
    }
    
    cumulativeWeight += stoneGrass12Weight;
    if (randomValue < cumulativeWeight) {
        return TILE_STONE_GRASS_12;
    }
    
    cumulativeWeight += stoneGrass13Weight;
    if (randomValue < cumulativeWeight) {
        return TILE_STONE_GRASS_13;
    }
    
    cumulativeWeight += stoneGrass14Weight;
    if (randomValue < cumulativeWeight) {
        return TILE_STONE_GRASS_14;
    }
    
    cumulativeWeight += stoneGrass15Weight;
    if (randomValue < cumulativeWeight) {
        return TILE_STONE_GRASS_15;
    }
    
    cumulativeWeight += stoneGrass16Weight;
    if (randomValue < cumulativeWeight) {
        return TILE_STONE_GRASS_16;
    }
    
    cumulativeWeight += stoneGrass17Weight;
    if (randomValue < cumulativeWeight) {
        return TILE_STONE_GRASS_17;
    }
    
    cumulativeWeight += stoneGrass18Weight;
    if (randomValue < cumulativeWeight) {
        return TILE_STONE_GRASS_18;
    }
    
    cumulativeWeight += stoneGrass19Weight;
    if (randomValue < cumulativeWeight) {
        return TILE_STONE_GRASS_19;
    }
    
    // Fallback to stone if something goes wrong (stone is highest priority)
    return TILE_STONE;
}

int World::generateNoiseBasedTileType(int x, int y) {
    // Generate noise value for this position
    float noiseValue = generateNoise(x * tileGenConfig.noiseScale, y * tileGenConfig.noiseScale);
    
    // Use noise to determine tile type with weighted distribution
    float normalizedNoise = (noiseValue + 1.0f) / 2.0f; // Normalize to 0-1
    
    // Create weighted thresholds based on tile weights - stone is now highest priority
    float stoneThreshold = tileGenConfig.stoneWeight / 100.0f;
    float grassThreshold = stoneThreshold + (tileGenConfig.grassWeight / 100.0f);
    float stoneGrassThreshold = grassThreshold + (tileGenConfig.stoneGrassWeight / 100.0f);
    float stoneGrass2Threshold = stoneGrassThreshold + (tileGenConfig.stoneGrass2Weight / 100.0f);
    float stoneGrass3Threshold = stoneGrass2Threshold + (tileGenConfig.stoneGrass3Weight / 100.0f);
    float stoneGrass4Threshold = stoneGrass3Threshold + (tileGenConfig.stoneGrass4Weight / 100.0f);
    float stoneGrass5Threshold = stoneGrass4Threshold + (tileGenConfig.stoneGrass5Weight / 100.0f);
    float stoneGrass6Threshold = stoneGrass5Threshold + (tileGenConfig.stoneGrass6Weight / 100.0f);
    float stoneGrass7Threshold = stoneGrass6Threshold + (tileGenConfig.stoneGrass7Weight / 100.0f);
    float stoneGrass8Threshold = stoneGrass7Threshold + (tileGenConfig.stoneGrass8Weight / 100.0f);
    float stoneGrass9Threshold = stoneGrass8Threshold + (tileGenConfig.stoneGrass9Weight / 100.0f);
    float stoneGrass10Threshold = stoneGrass9Threshold + (tileGenConfig.stoneGrass10Weight / 100.0f);
    float stoneGrass11Threshold = stoneGrass10Threshold + (tileGenConfig.stoneGrass11Weight / 100.0f);
    float stoneGrass12Threshold = stoneGrass11Threshold + (tileGenConfig.stoneGrass12Weight / 100.0f);
    float stoneGrass13Threshold = stoneGrass12Threshold + (tileGenConfig.stoneGrass13Weight / 100.0f);
    float stoneGrass14Threshold = stoneGrass13Threshold + (tileGenConfig.stoneGrass14Weight / 100.0f);
    float stoneGrass15Threshold = stoneGrass14Threshold + (tileGenConfig.stoneGrass15Weight / 100.0f);
    float stoneGrass16Threshold = stoneGrass15Threshold + (tileGenConfig.stoneGrass16Weight / 100.0f);
    float stoneGrass17Threshold = stoneGrass16Threshold + (tileGenConfig.stoneGrass17Weight / 100.0f);
    float stoneGrass18Threshold = stoneGrass17Threshold + (tileGenConfig.stoneGrass18Weight / 100.0f);
    float stoneGrass19Threshold = stoneGrass18Threshold + (tileGenConfig.stoneGrass19Weight / 100.0f);
    
    // Determine tile type based on noise value
    if (normalizedNoise < stoneThreshold) {
        return TILE_STONE;
    } else if (normalizedNoise < grassThreshold) {
        return TILE_GRASS;
    } else if (normalizedNoise < stoneGrassThreshold) {
        return TILE_STONE_GRASS;
    } else if (normalizedNoise < stoneGrass2Threshold) {
        return TILE_STONE_GRASS_2;
    } else if (normalizedNoise < stoneGrass3Threshold) {
        return TILE_STONE_GRASS_3;
    } else if (normalizedNoise < stoneGrass4Threshold) {
        return TILE_STONE_GRASS_4;
    } else if (normalizedNoise < stoneGrass5Threshold) {
        return TILE_STONE_GRASS_5;
    } else if (normalizedNoise < stoneGrass6Threshold) {
        return TILE_STONE_GRASS_6;
    } else if (normalizedNoise < stoneGrass7Threshold) {
        return TILE_STONE_GRASS_7;
    } else if (normalizedNoise < stoneGrass8Threshold) {
        return TILE_STONE_GRASS_8;
    } else if (normalizedNoise < stoneGrass9Threshold) {
        return TILE_STONE_GRASS_9;
    } else if (normalizedNoise < stoneGrass10Threshold) {
        return TILE_STONE_GRASS_10;
    } else if (normalizedNoise < stoneGrass11Threshold) {
        return TILE_STONE_GRASS_11;
    } else if (normalizedNoise < stoneGrass12Threshold) {
        return TILE_STONE_GRASS_12;
    } else if (normalizedNoise < stoneGrass13Threshold) {
        return TILE_STONE_GRASS_13;
    } else if (normalizedNoise < stoneGrass14Threshold) {
        return TILE_STONE_GRASS_14;
    } else if (normalizedNoise < stoneGrass15Threshold) {
        return TILE_STONE_GRASS_15;
    } else if (normalizedNoise < stoneGrass16Threshold) {
        return TILE_STONE_GRASS_16;
    } else if (normalizedNoise < stoneGrass17Threshold) {
        return TILE_STONE_GRASS_17;
    } else if (normalizedNoise < stoneGrass18Threshold) {
        return TILE_STONE_GRASS_18;
    } else if (normalizedNoise < stoneGrass19Threshold) {
        return TILE_STONE_GRASS_19;
    } else {
        // Fallback to stone if something goes wrong (stone is highest priority)
        return TILE_STONE;
    }
}

float World::generateNoise(float x, float y) {
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
                        // Convert some nearby grass tiles to stone or stone grass
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
                                            tiles[ny][nx].id = TILE_STONE_GRASS;
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
    int stoneGrassCounts[19] = {0}; // Array to count all stone_grass variants
    
    // Count tile types
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int tileId = tiles[y][x].id;
            switch (tileId) {
                case TILE_GRASS: grassCount++; break;
                case TILE_STONE: stoneCount++; break;
                default:
                    if (tileId >= TILE_STONE_GRASS && tileId <= TILE_STONE_GRASS_19) {
                        stoneGrassCounts[tileId - TILE_STONE_GRASS]++;
                    }
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
    
    // Display stone_grass variant counts
    int totalStoneGrass = 0;
    for (int i = 0; i < 19; i++) {
        totalStoneGrass += stoneGrassCounts[i];
        float variantPercent = (static_cast<float>(stoneGrassCounts[i]) / totalTiles) * 100.0f;
        std::cout << "  StoneGrass" << (i + 1) << ": " << stoneGrassCounts[i] << " (" << variantPercent << "%)" << std::endl;
    }
    
    float totalStoneGrassPercent = (static_cast<float>(totalStoneGrass) / totalTiles) * 100.0f;
    std::cout << "  Total StoneGrass variants: " << totalStoneGrass << " (" << totalStoneGrassPercent << "%)" << std::endl;
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
        case TILE_GRASS:
            return TileColor(34, 139, 34, 255); // Forest green
        case TILE_STONE:
            return TileColor(128, 128, 128, 255); // Gray
        case TILE_STONE_GRASS:
        case TILE_STONE_GRASS_2:
        case TILE_STONE_GRASS_3:
        case TILE_STONE_GRASS_4:
        case TILE_STONE_GRASS_5:
        case TILE_STONE_GRASS_6:
        case TILE_STONE_GRASS_7:
        case TILE_STONE_GRASS_8:
        case TILE_STONE_GRASS_9:
        case TILE_STONE_GRASS_10:
        case TILE_STONE_GRASS_11:
        case TILE_STONE_GRASS_12:
        case TILE_STONE_GRASS_13:
        case TILE_STONE_GRASS_14:
        case TILE_STONE_GRASS_15:
        case TILE_STONE_GRASS_16:
        case TILE_STONE_GRASS_17:
        case TILE_STONE_GRASS_18:
        case TILE_STONE_GRASS_19:
            return TileColor(100, 150, 100, 255); // Darker green
        case TILE_WATER:
            return TileColor(0, 191, 255, 255); // Deep sky blue
        default:
            return TileColor(34, 139, 34, 255); // Default to grass
    }
}
