#include "AssetManager.h"
#include <iostream>
#include <filesystem>

// Static member initialization
const std::string AssetManager::ASSETS_PATH = "assets/";
const std::string AssetManager::WIZARD_PATH = ASSETS_PATH + "Wizard 2D Pixel Art v2.0/Sprites/without_outline/";
const std::string AssetManager::TILESET_PATH = ASSETS_PATH + "FULL_Fantasy Forest/Tiles/";
const std::string AssetManager::UI_PATH = ASSETS_PATH + "UI/";
const std::string AssetManager::OBJECTS_PATH = ASSETS_PATH + "FULL_Fantasy Forest/Objects/";

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
    
    SDL_Texture* sdlTexture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    
    if (!sdlTexture) {
        std::cerr << "Failed to create texture from surface: " << SDL_GetError() << std::endl;
        return nullptr;
    }
    
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
    std::cout << "Preloading assets..." << std::endl;
    
    // Clear all caches first
    textureCache.clear();
    spriteSheetCache.clear();
    
    // Load wizard sprites
    // The PNG files have padding on the sides - each frame is 128x78 but character content is 64x78 centered
    loadSpriteSheet(WIZARD_PATH + "IDLE_LEFT.png", 128, 78, 6, 6);   // 128x78 frames, 6 frames per row, 6 total frames
    loadSpriteSheet(WIZARD_PATH + "IDLE_RIGHT.png", 128, 78, 6, 6);   // 128x78 frames, 6 frames per row, 6 total frames
    loadSpriteSheet(WIZARD_PATH + "WALK_LEFT.png", 128, 78, 4, 4);   // 128x78 frames, 4 frames per row, 4 total frames
    loadSpriteSheet(WIZARD_PATH + "WALK_RIGHT.png", 128, 78, 4, 4);   // 128x78 frames, 4 frames per row, 4 total frames
    loadSpriteSheet(WIZARD_PATH + "MELEE ATTACK_LEFT.png", 128, 78, 6, 6);   // 128x78 frames, 6 frames per row, 6 total frames
    loadSpriteSheet(WIZARD_PATH + "MELEE ATTACK_RIGHT.png", 128, 78, 6, 6);   // 128x78 frames, 6 frames per row, 6 total frames
    loadSpriteSheet(WIZARD_PATH + "RANGED ATTACK_LEFT.png", 128, 78, 10, 10); // 128x78 frames, 10 frames per row, 10 total frames
    loadSpriteSheet(WIZARD_PATH + "RANGED ATTACK_RIGHT.png", 128, 78, 10, 10); // 128x78 frames, 10 frames per row, 10 total frames
    loadSpriteSheet(WIZARD_PATH + "Projectile.png", 32, 32, 5, 5); // 32x32 frames, 5 frames per row, 5 total frames
    loadSpriteSheet(WIZARD_PATH + "HURT.png", 128, 78, 8, 8);   // 128x78 frames, 8 frames per row, 8 total frames
    loadSpriteSheet(WIZARD_PATH + "DEATH.png", 128, 78, 12, 12); // 128x78 frames, 12 frames per row, 12 total frames
    
    // Load tilesets
    loadTexture(TILESET_PATH + "Tileset Inside.png");
    loadTexture(TILESET_PATH + "Tileset Outside.png");
    
    // Load backgrounds
    loadTexture(ASSETS_PATH + "FULL_Fantasy Forest/Backgrounds/Sky.png");
    loadTexture(ASSETS_PATH + "FULL_Fantasy Forest/Backgrounds/Grass Mountains.png");
    
    // Load object textures
    loadTexture(OBJECTS_PATH + "chest_opened.png");
    loadTexture(OBJECTS_PATH + "chest_unopened.png");
    loadTexture(OBJECTS_PATH + "clay_pot.png");
    loadTexture(OBJECTS_PATH + "flag.png");
    loadTexture(OBJECTS_PATH + "wood_crate.png");
    loadTexture(OBJECTS_PATH + "steel_crate.png");
    loadTexture(OBJECTS_PATH + "wood_fence.png");
    loadTexture(OBJECTS_PATH + "wood_fence_broken.png");
    loadTexture(OBJECTS_PATH + "wood_sign.png");
    
    // Load bonfire as spritesheet with 6 frames
    // The bonfire texture is likely taller than 32 pixels, so using 48 pixels height
    loadSpriteSheet(OBJECTS_PATH + "Bonfire.png", 32, 48, 6, 6);
    
    std::cout << "Asset preloading complete!" << std::endl;
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
