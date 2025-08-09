#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

// Forward declarations
struct SDL_Texture;
struct SDL_Surface;

class Texture {
public:
    Texture(SDL_Texture* texture, int width, int height);
    ~Texture();
    
    SDL_Texture* getTexture() const { return texture; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    
    // Disable copying
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    
    // Allow moving
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

private:
    SDL_Texture* texture;
    int width;
    int height;
};

class SpriteSheet {
public:
    SpriteSheet(std::unique_ptr<Texture> texture, int frameWidth, int frameHeight, int framesPerRow, int totalFrames = 0);
    
    Texture* getTexture() const { return texture.get(); }
    int getFrameWidth() const { return frameWidth; }
    int getFrameHeight() const { return frameHeight; }
    int getFramesPerRow() const { return framesPerRow; }
    int getTotalFrames() const { return totalFrames; }
    
    // Get source rectangle for a specific frame
    SDL_Rect getFrameRect(int frameIndex) const;

private:
    std::unique_ptr<Texture> texture;
    int frameWidth;
    int frameHeight;
    int framesPerRow;
    int totalFrames;
};

class AssetManager {
public:
    explicit AssetManager(SDL_Renderer* renderer);
    ~AssetManager();

    // Texture loading
    Texture* loadTexture(const std::string& path);
    Texture* getTexture(const std::string& path);
    
    // Sprite sheet loading
    SpriteSheet* loadSpriteSheet(const std::string& path, int frameWidth, int frameHeight, int framesPerRow = 0, int totalFrames = 0);
    SpriteSheet* getSpriteSheet(const std::string& path);
    // Helper: auto-calc frame size from image dimensions and totalFrames (optionally framesPerRow)
    SpriteSheet* loadSpriteSheetAuto(const std::string& path, int totalFrames, int framesPerRow = 0);
    
    // Font loading (for UI)
    TTF_Font* loadFont(const std::string& path, int size);
    TTF_Font* getFont(const std::string& path, int size);
    
    // Utility functions
    void preloadAssets();
    void clearCache();
    
    // Asset paths
    static const std::string ASSETS_PATH;
    static const std::string WIZARD_PATH;
    static const std::string TILESET_PATH;
    static const std::string UI_PATH;
    static const std::string OBJECTS_PATH;
    // Optional: base path for enemies
    static constexpr const char* DEMON_BOSS_PATH = "assets/Demon Boss 2D Pixel Art/Sprites/without_outline/";

private:
    SDL_Renderer* renderer;
    
    // Asset caches
    std::unordered_map<std::string, std::unique_ptr<Texture>> textureCache;
    std::unordered_map<std::string, std::unique_ptr<SpriteSheet>> spriteSheetCache;
    std::unordered_map<std::string, TTF_Font*> fontCache;
    
    // Helper functions
    std::string getFullPath(const std::string& relativePath) const;
    bool fileExists(const std::string& path) const;
};
