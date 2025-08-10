#include "AssetManager.h"
#include <iostream>
#include <filesystem>

// Static member initialization
const std::string AssetManager::ASSETS_PATH = "assets/";
const std::string AssetManager::WIZARD_PATH = ASSETS_PATH + "Wizard 2D Pixel Art v2.0/Sprites/without_outline/";
// New main character root (top folder contains subfolders per action and direction)
const std::string AssetManager::MAIN_CHAR_PATH = ASSETS_PATH + "Main Character/";
const std::string AssetManager::TILESET_PATH = ASSETS_PATH + "Textures/Tiles/";
const std::string AssetManager::UI_PATH = ASSETS_PATH + "UI/";
const std::string AssetManager::OBJECTS_PATH = ASSETS_PATH + "Textures/Objects/";

// Texture implementation
Texture::Texture(SDL_Texture* texture, int width, int height) 
    : texture(texture), width(width), height(height) {
}

Texture::~Texture() {
    if (texture) {
        SDL_DestroyTexture(texture);
    }
}

Texture::Texture(Texture&& other) noexcept 
    : texture(other.texture), width(other.width), height(other.height) {
    other.texture = nullptr;
    other.width = 0;
    other.height = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        if (texture) {
            SDL_DestroyTexture(texture);
        }
        texture = other.texture;
        width = other.width;
        height = other.height;
        other.texture = nullptr;
        other.width = 0;
        other.height = 0;
    }
    return *this;
}

// SpriteSheet implementation
SpriteSheet::SpriteSheet(std::unique_ptr<Texture> texture, int frameWidth, int frameHeight, int framesPerRow, int totalFrames)
    : texture(std::move(texture)), frameWidth(frameWidth), frameHeight(frameHeight), framesPerRow(framesPerRow), totalFrames(totalFrames) {
    
    if (this->texture) {
        if (framesPerRow == 0) {
            // Auto-calculate frames per row
            this->framesPerRow = this->texture->getWidth() / frameWidth;
        }
        if (totalFrames == 0) {
            // Auto-calculate total frames if not specified
            this->totalFrames = this->framesPerRow;
        }
    }
}

SDL_Rect SpriteSheet::getFrameRect(int frameIndex) const {
    if (frameIndex >= totalFrames || frameIndex < 0) {
        return {0, 0, 0, 0};
    }
    
    int row = frameIndex / framesPerRow;
    int col = frameIndex % framesPerRow;
    
    return {
        col * frameWidth,
        row * frameHeight,
        frameWidth,
        frameHeight
    };
}

// AssetManager implementation
AssetManager::AssetManager(SDL_Renderer* renderer) : renderer(renderer) {
    if (!renderer) {
        throw std::runtime_error("AssetManager requires a valid SDL_Renderer");
    }
}

AssetManager::~AssetManager() {
    clearCache();
}

Texture* AssetManager::loadTexture(const std::string& path) {
    // Check if already loaded
    auto it = textureCache.find(path);
    if (it != textureCache.end()) {
        return it->second.get();
    }
    
    std::string fullPath = getFullPath(path);
    if (!fileExists(fullPath)) {
        std::cerr << "Texture file not found: " << fullPath << std::endl;
        return nullptr;
    }
    
    SDL_Surface* surface = IMG_Load(fullPath.c_str());
    if (!surface) {
        std::cerr << "Failed to load texture surface: " << IMG_GetError() << std::endl;
        return nullptr;
    }
    
    // Print surface details for debugging
    std::cout << "Loaded surface: " << path << " (" << surface->w << "x" << surface->h << ", format: " << surface->format->format << ")" << std::endl;
    
    // Convert surface to the correct format if needed
    SDL_Surface* convertedSurface = nullptr;
    SDL_PixelFormat* format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888);
    if (!format) {
        std::cerr << "Failed to allocate pixel format" << std::endl;
        SDL_FreeSurface(surface);
        return nullptr;
    }
    
    // Always convert to RGBA8888 for consistency
    convertedSurface = SDL_ConvertSurface(surface, format, 0);
    SDL_FreeFormat(format);
    SDL_FreeSurface(surface);
    
    if (!convertedSurface) {
        std::cerr << "Failed to convert surface: " << SDL_GetError() << std::endl;
        return nullptr;
    }
    
    // Create texture with proper blend mode
    SDL_Texture* sdlTexture = SDL_CreateTextureFromSurface(renderer, convertedSurface);
    SDL_FreeSurface(convertedSurface);
    
    if (!sdlTexture) {
        std::cerr << "Failed to create texture from surface: " << SDL_GetError() << std::endl;
        return nullptr;
    }
    
    // Set proper blend mode for the texture
    SDL_SetTextureBlendMode(sdlTexture, SDL_BLENDMODE_BLEND);
    
    int width, height;
    SDL_QueryTexture(sdlTexture, nullptr, nullptr, &width, &height);
    
    auto texture = std::make_unique<Texture>(sdlTexture, width, height);
    Texture* result = texture.get();
    textureCache[path] = std::move(texture);
    
    std::cout << "Loaded texture: " << path << " (" << width << "x" << height << ")" << std::endl;
    return result;
}

Texture* AssetManager::getTexture(const std::string& path) {
    auto it = textureCache.find(path);
    if (it != textureCache.end()) {
        return it->second.get();
    }
    return loadTexture(path);
}

SpriteSheet* AssetManager::loadSpriteSheet(const std::string& path, int frameWidth, int frameHeight, int framesPerRow, int totalFrames) {
    // Check if already loaded
    auto it = spriteSheetCache.find(path);
    if (it != spriteSheetCache.end()) {
        return it->second.get();
    }
    
    // Load the texture directly for the sprite sheet
    std::string fullPath = getFullPath(path);
    if (!fileExists(fullPath)) {
        std::cerr << "Sprite sheet file not found: " << fullPath << std::endl;
        return nullptr;
    }
    
    SDL_Surface* surface = IMG_Load(fullPath.c_str());
    if (!surface) {
        std::cerr << "Failed to load sprite sheet surface: " << IMG_GetError() << std::endl;
        return nullptr;
    }
    
    SDL_Texture* sdlTexture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    
    if (!sdlTexture) {
        std::cerr << "Failed to create sprite sheet texture: " << SDL_GetError() << std::endl;
        return nullptr;
    }
    
    int width, height;
    SDL_QueryTexture(sdlTexture, nullptr, nullptr, &width, &height);
    
    auto spriteSheet = std::make_unique<SpriteSheet>(
        std::make_unique<Texture>(sdlTexture, width, height), frameWidth, frameHeight, framesPerRow, totalFrames);
    
    SpriteSheet* result = spriteSheet.get();
    spriteSheetCache[path] = std::move(spriteSheet);
    
    std::cout << "Loaded sprite sheet: " << path << " (" << frameWidth << "x" << frameHeight << " frames, total: " << result->getTotalFrames() << ")" << std::endl;
    return result;
}

SpriteSheet* AssetManager::getSpriteSheet(const std::string& path) {
    auto it = spriteSheetCache.find(path);
    if (it != spriteSheetCache.end()) {
        return it->second.get();
    }
    return nullptr;
}

SpriteSheet* AssetManager::loadSpriteSheetAuto(const std::string& path, int totalFrames, int framesPerRow) {
    // If already loaded, return it
    auto it = spriteSheetCache.find(path);
    if (it != spriteSheetCache.end()) {
        return it->second.get();
    }

    std::string fullPath = getFullPath(path);
    if (!fileExists(fullPath)) {
        std::cerr << "Sprite sheet file not found: " << fullPath << std::endl;
        return nullptr;
    }

    SDL_Surface* surface = IMG_Load(fullPath.c_str());
    if (!surface) {
        std::cerr << "Failed to load sprite sheet surface: " << IMG_GetError() << std::endl;
        return nullptr;
    }

    const int imgW = surface->w;
    const int imgH = surface->h;

    int cols = 0;
    int rows = 0;

    // 1) Honor explicit framesPerRow if provided
    if (framesPerRow > 0 && totalFrames % framesPerRow == 0) {
        int tryCols = framesPerRow;
        int tryRows = totalFrames / framesPerRow;
        if (tryCols > 0 && tryRows > 0 && imgW % tryCols == 0 && imgH % tryRows == 0) {
            cols = tryCols;
            rows = tryRows;
        }
    }

    // 2) Prefer single-row layout when possible
    if (cols == 0) {
        if (imgW % totalFrames == 0 && imgH % 1 == 0) {
            cols = totalFrames;
            rows = 1;
        }
    }

    // 3) Otherwise, search divisors descending to minimize number of rows
    if (cols == 0) {
        for (int tryCols = totalFrames; tryCols >= 1; --tryCols) {
            if (totalFrames % tryCols != 0) continue;
            int tryRows = totalFrames / tryCols;
            if (imgW % tryCols == 0 && imgH % tryRows == 0) {
                cols = tryCols;
                rows = tryRows;
                break;
            }
        }
    }

    // 4) Absolute fallback - treat as single row slices even if height not divisible
    if (cols == 0 || rows == 0) {
        cols = totalFrames;
        rows = 1;
    }

    int frameWidth = imgW / cols;
    int frameHeight = imgH / rows;

    SDL_Texture* sdlTexture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!sdlTexture) {
        std::cerr << "Failed to create sprite sheet texture: " << SDL_GetError() << std::endl;
        return nullptr;
    }

    SDL_SetTextureBlendMode(sdlTexture, SDL_BLENDMODE_BLEND);
    int width, height;
    SDL_QueryTexture(sdlTexture, nullptr, nullptr, &width, &height);

    auto spriteSheet = std::make_unique<SpriteSheet>(
        std::make_unique<Texture>(sdlTexture, width, height), frameWidth, frameHeight, cols, totalFrames);

    SpriteSheet* result = spriteSheet.get();
    spriteSheetCache[path] = std::move(spriteSheet);

    std::cout << "Auto-loaded sprite sheet: " << path
              << " (img=" << imgW << "x" << imgH
              << ", frame=" << frameWidth << "x" << frameHeight
              << ", cols=" << cols << ", rows=" << rows
              << ", total=" << totalFrames << ")" << std::endl;
    return result;
}

TTF_Font* AssetManager::loadFont(const std::string& path, int size) {
    std::string key = path + "_" + std::to_string(size);
    
    auto it = fontCache.find(key);
    if (it != fontCache.end()) {
        return it->second;
    }
    
    std::string fullPath = getFullPath(path);
    if (!fileExists(fullPath)) {
        std::cerr << "Font file not found: " << fullPath << std::endl;
        return nullptr;
    }
    
    TTF_Font* font = TTF_OpenFont(fullPath.c_str(), size);
    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        return nullptr;
    }
    
    fontCache[key] = font;
    std::cout << "Loaded font: " << path << " (size " << size << ")" << std::endl;
    return font;
}

TTF_Font* AssetManager::getFont(const std::string& path, int size) {
    std::string key = path + "_" + std::to_string(size);
    auto it = fontCache.find(key);
    if (it != fontCache.end()) {
        return it->second;
    }
    return loadFont(path, size);
}

void AssetManager::preloadAssets() {
    std::cout << "Preloading assets with performance optimizations..." << std::endl;
    
    // Clear all caches first
    textureCache.clear();
    spriteSheetCache.clear();
    
    // OPTIMIZATION: Load assets in batches to improve performance
    // Load wizard sprites for enemy AI
    std::cout << "Loading wizard sprites (enemy)..." << std::endl;
    loadSpriteSheet(WIZARD_PATH + "IDLE_LEFT.png", 128, 78, 6, 6);
    loadSpriteSheet(WIZARD_PATH + "IDLE_RIGHT.png", 128, 78, 6, 6);
    loadSpriteSheet(WIZARD_PATH + "WALK_LEFT.png", 128, 78, 4, 4);
    loadSpriteSheet(WIZARD_PATH + "WALK_RIGHT.png", 128, 78, 4, 4);
    loadSpriteSheet(WIZARD_PATH + "MELEE ATTACK_LEFT.png", 128, 78, 6, 6);
    loadSpriteSheet(WIZARD_PATH + "MELEE ATTACK_RIGHT.png", 128, 78, 6, 6);
    loadSpriteSheet(WIZARD_PATH + "RANGED ATTACK_LEFT.png", 128, 78, 10, 10);
    loadSpriteSheet(WIZARD_PATH + "RANGED ATTACK_RIGHT.png", 128, 78, 10, 10);
    loadSpriteSheet(WIZARD_PATH + "Projectile.png", 32, 32, 5, 5);
    loadSpriteSheet(WIZARD_PATH + "HURT_LEFT.png", 128, 78, 4, 4);
    loadSpriteSheet(WIZARD_PATH + "HURT_RIGHT.png", 128, 78, 4, 4);
    loadSpriteSheet(WIZARD_PATH + "DEATH.png", 128, 78, 6, 6);

    // Load new main character 8-frame directional sheets
    // Files exist under assets/Main Character/{IDLE,RUN,ATTACK 1,ATTACK 2}/{idle_*.png, run_*.png, attack*.png}
    std::cout << "Loading main character sprites (player)..." << std::endl;
    // IDLE (8 frames each)
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "IDLE/idle_left.png", 8, 8);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "IDLE/idle_right.png", 8, 8);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "IDLE/idle_up.png", 8, 8);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "IDLE/idle_down.png", 8, 8);
    // RUN (8 frames each)
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "RUN/run_left.png", 8, 8);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "RUN/run_right.png", 8, 8);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "RUN/run_up.png", 8, 8);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "RUN/run_down.png", 8, 8);
    // ATTACK 1 (melee) and ATTACK 2 (alt) - use 8 frames
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "ATTACK 1/attack1_left.png", 8, 8);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "ATTACK 1/attack1_right.png", 8, 8);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "ATTACK 1/attack1_up.png", 8, 8);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "ATTACK 1/attack1_down.png", 8, 8);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "ATTACK 2/attack2_left.png", 8, 8);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "ATTACK 2/attack2_right.png", 8, 8);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "ATTACK 2/attack2_up.png", 8, 8);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "ATTACK 2/attack2_down.png", 8, 8);
    
    // Load new tile textures by folder groups (variants are loaded lazily by World)
    std::cout << "Loading tile textures..." << std::endl;
    auto load8 = [&](const std::string& dir, const std::string& prefix){
        for (int i = 1; i <= 8; ++i) {
            char name[64];
            std::snprintf(name, sizeof(name), "%s_%02d.png", prefix.c_str(), i);
            loadTexture(TILESET_PATH + dir + "/" + name);
        }
    };
    load8("Grass", "Grass");
    load8("Dirt", "Dirt");
    load8("Stone", "Stone");
    load8("Asphalt", "Asphalt");
    load8("Concrete", "Concrete");
    load8("Sand", "Sand");
    load8("Snow", "Snow");
    load8("Grassy Asphalt", "GrassyAsphalt");
    load8("Grassy Concrete", "GrassyConcrete");
    load8("Sandy Dirt", "SandyDirt");
    load8("Sandy Stone", "SandyStone");
    load8("Snowy Stone", "SnowyStone");
    load8("Stony Dirt", "StonyDirt");
    load8("Wet Dirt", "WetDirt");

    // Water/lava
    loadTexture(TILESET_PATH + "Water/water_shallow.png");
    // Deep water: auto-detect frames per row; total frames fixed at 4
    loadSpriteSheet(TILESET_PATH + "Water/water_deep_01.png", 32, 32, 0, 4);
    loadSpriteSheet(TILESET_PATH + "Lava/lava.png", 32, 32, 9, 9);
    
    // Load object textures (only the ones we actually use)
    std::cout << "Loading object textures..." << std::endl;
    loadTexture(OBJECTS_PATH + "chest_unopened.png");
    loadTexture(OBJECTS_PATH + "clay_pot.png");
    loadTexture(OBJECTS_PATH + "flag.png");
    loadTexture(OBJECTS_PATH + "wood_crate.png");
    loadTexture(OBJECTS_PATH + "steel_crate.png");
    loadTexture(OBJECTS_PATH + "wood_sign.png");
    // Load potion icons/sprite sheets for HUD
    // 8-frame sprite sheets for when potions have charges
    loadSpriteSheetAuto("assets/Textures/All Potions/HP potions/full_hp_potion.png", 8, 8);
    loadSpriteSheetAuto("assets/Textures/All Potions/HP potions/half_hp_potion.png", 8, 8);
    // Note: low_hp_potion has 9 frames
    loadSpriteSheetAuto("assets/Textures/All Potions/HP potions/low_hp_potion.png", 9, 9);
    loadSpriteSheetAuto("assets/Textures/All Potions/Mana potion/full_mana_potion.png", 8, 8);
    loadSpriteSheetAuto("assets/Textures/All Potions/Mana potion/half_mana_potion.png", 8, 8);
    loadSpriteSheetAuto("assets/Textures/All Potions/Mana potion/low_mana_potion.png", 8, 8);
    // Single-frame empty icons
    loadTexture("assets/Textures/All Potions/HP potions/empty.png");
    loadTexture("assets/Textures/All Potions/Mana potion/empty.png");
    
    // Load bonfire as spritesheet with 6 frames
    loadSpriteSheet(OBJECTS_PATH + "Bonfire.png", 32, 48, 6, 6);
    
    // Preload primary UI/game font
    loadFont("assets/Fonts/retganon.ttf", 16);
    loadFont("assets/Fonts/retganon.ttf", 12);
    loadFont("assets/Fonts/retganon.ttf", 24);
    
    std::cout << "Asset preloading complete! Loaded " << textureCache.size() << " textures and " << spriteSheetCache.size() << " sprite sheets." << std::endl;
}

void AssetManager::clearCache() {
    textureCache.clear();
    spriteSheetCache.clear();
    
    for (auto& font : fontCache) {
        TTF_CloseFont(font.second);
    }
    fontCache.clear();
}

std::string AssetManager::getFullPath(const std::string& relativePath) const {
    return relativePath;
}

bool AssetManager::fileExists(const std::string& path) const {
    return std::filesystem::exists(path);
}
