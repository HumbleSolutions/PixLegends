#include "AssetManager.h"
#include <iostream>
#include <filesystem>

// Static member initialization
const std::string AssetManager::ASSETS_PATH = "assets/";
const std::string AssetManager::WIZARD_PATH = ASSETS_PATH + "Wizard/Sprites/";
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
    loadSpriteSheetAuto(WIZARD_PATH + "IDLE.png", 6, 6);
    loadSpriteSheetAuto(WIZARD_PATH + "WALK.png", 4, 4);
    loadSpriteSheetAuto(WIZARD_PATH + "MELEE ATTACK.png", 6, 6);
    loadSpriteSheetAuto(WIZARD_PATH + "RANGED ATTACK.png", 10, 10);
    loadSpriteSheetAuto(WIZARD_PATH + "Projectile.png", 5, 5);
    loadSpriteSheetAuto(WIZARD_PATH + "HURT.png", 4, 4);
    loadSpriteSheetAuto(WIZARD_PATH + "DEATH.png", 6, 6);

    // Load goblin minion sprites (optional preload; they will auto-load on demand too)
    std::cout << "Loading goblin sprites (minion)..." << std::endl;
    const std::string GOBLIN_BASE = "assets/Goblin/Sprites/";
    // Available files: ATTACK1.png, ATTACK2.png, DEATH.png, HURT.png, IDLE.png, RUN.png
    loadSpriteSheetAuto(GOBLIN_BASE + "IDLE.png", 3, 3);
    loadSpriteSheetAuto(GOBLIN_BASE + "RUN.png", 6, 6);
    loadSpriteSheetAuto(GOBLIN_BASE + "ATTACK1.png", 6, 6);
    loadSpriteSheetAuto(GOBLIN_BASE + "ATTACK2.png", 6, 6);
    loadSpriteSheetAuto(GOBLIN_BASE + "HURT.png", 3, 3);
    loadSpriteSheetAuto(GOBLIN_BASE + "DEATH.png", 10, 10);

    // Load new main character directional sheets
    // Files exist under assets/Main Character/{IDLE,RUN,TAKE_DMG,DEATH,ATTACK 1,ATTACK 2}/
    std::cout << "Loading main character sprites (player)..." << std::endl;
    // SWORD: IDLE/RUN (6 frames each, 128 or 384 px frames depending on sheet)
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "IDLE/Sword_Idle_Left.png", 6, 6);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "IDLE/Sword_Idle_Right.png", 6, 6);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "IDLE/Sword_Idle_Up.png", 6, 6);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "IDLE/Sword_Idle_Down.png", 6, 6);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "RUN/Sword_Run_Left.png", 6, 6);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "RUN/Sword_Run_Right.png", 6, 6);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "RUN/Sword_Run_Up.png", 6, 6);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "RUN/Sword_Run_Down.png", 6, 6);
    // BOW: IDLE/RUN (6 frames)
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "IDLE/Bow_Idle_Left.png", 6, 6);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "IDLE/Bow_Idle_Right.png", 6, 6);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "IDLE/Bow_Idle_Up.png", 6, 6);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "IDLE/Bow_Idle_Down.png", 6, 6);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "RUN/Bow_Run_Left.png", 6, 6);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "RUN/Bow_Run_Right.png", 6, 6);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "RUN/Bow_Run_Up.png", 6, 6);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "RUN/Bow_Run_Down.png", 6, 6);
    // TAKE DAMAGE (6 frames each, 128x128) - support both naming schemes
    // Newer set may be named Damage_*.png or hurt_*.png
    auto tryLoadHurt = [&](const std::string& a, const std::string& b){
        // try A then B; only loads if exists
        if (!loadSpriteSheetAuto(a, 6, 6)) {
            loadSpriteSheetAuto(b, 6, 6);
        }
    };
    // Load hurt sprites directly (Damage_*.png files don't exist)
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "TAKE_DMG/hurt_left.png", 6, 6);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "TAKE_DMG/hurt_right.png", 6, 6);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "TAKE_DMG/hurt_up.png", 6, 6);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "TAKE_DMG/hurt_down.png", 6, 6);
    // DEATH (12 frames each, 128x128)
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "DEATH/Death_Left.png", 12, 12);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "DEATH/Death_Right.png", 12, 12);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "DEATH/Death_Up.png", 12, 12);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "DEATH/Death_Down.png", 12, 12);
    // ATTACKS: Sword (melee) and Bow (ranged)
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "SWORD ATTACK 1/Sword_Attack_1_Left.png", 8, 8);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "SWORD ATTACK 1/Sword_Attack_1_Right.png", 8, 8);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "SWORD ATTACK 1/Sword_Attack_1_Up.png", 8, 8);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "SWORD ATTACK 1/Sword_Attack_1_Down.png", 8, 8);
    // (Sword_Attack_2 reserved for future ability)
    // Bow attack 1 (6 frames)
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "BOW ATTACK 1/Bow_Attack_1_Left.png", 6, 6);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "BOW ATTACK 1/Bow_Attack_1_Right.png", 6, 6);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "BOW ATTACK 1/Bow_Attack_1_Up.png", 6, 6);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "BOW ATTACK 1/Bow_Attack_1_Down.png", 6, 6);
    // End-of-attack wind-down (sword 4 frames)
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "END ANIMATION/Sword_Attack_End_Left.png", 4, 4);
    // Bow end attack (2 frames) - optional - DISABLED: Files not found
    // loadSpriteSheetAuto(MAIN_CHAR_PATH + "BOW ATTACK 1/Bow_Attack_End_Left.png", 2, 2);
    // loadSpriteSheetAuto(MAIN_CHAR_PATH + "BOW ATTACK 1/Bow_Attack_End_Right.png", 2, 2);
    // loadSpriteSheetAuto(MAIN_CHAR_PATH + "BOW ATTACK 1/Bow_Attack_End_Up.png", 2, 2);
    // loadSpriteSheetAuto(MAIN_CHAR_PATH + "BOW ATTACK 1/Bow_Attack_End_Down.png", 2, 2);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "END ANIMATION/Sword_Attack_End_Right.png", 4, 4);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "END ANIMATION/Sword_Attack_End_Up.png", 4, 4);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "END ANIMATION/Sword_Attack_End_Down.png", 4, 4);
    // DASH (8 frames)
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "DASH/Dash_Left.png", 8, 8);
    // Bow arrow sprite (single frame)
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "BOW ATTACK 1/arrow.png", 1, 1);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "DASH/Dash_Right.png", 8, 8);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "DASH/Dash_Up.png", 8, 8);
    loadSpriteSheetAuto(MAIN_CHAR_PATH + "DASH/Dash_Down.png", 8, 8);
    
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
    // Experience orbs
    loadTexture(OBJECTS_PATH + "exp_orb1.png");
    loadTexture(OBJECTS_PATH + "exp_orb2.png");
    loadTexture(OBJECTS_PATH + "exp_orb3.png");
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
    // Load Magic Anvil spritesheet with 39 frames
    loadSpriteSheetAuto(OBJECTS_PATH + "magic_anvil.png", 39, 0);
    // Load Underworld Magic Anvil spritesheet with 39 frames
    loadSpriteSheetAuto(OBJECTS_PATH + "underword_magic_anvil.png", 39, 0);
    
    // Preload primary UI/game font
    loadFont("assets/Fonts/retganon.ttf", 16);
    loadFont("assets/Fonts/retganon.ttf", 12);
    loadFont("assets/Fonts/retganon.ttf", 24);

  // UI textures (HP/MP/EXP bars) - DISABLED: Files not found, loaded on demand instead
  // loadTexture(UI_PATH + "hp_bar.png");
  // loadTexture(UI_PATH + "mana_bar.png");
  // loadTexture(UI_PATH + "exp_bar.png");
  // Optional: full composite UI sheet if we want to use it later
  // loadTexture(UI_PATH + "hp_mana_bar_ui.png");
  // Upgrade inventory panel
  // loadTexture(UI_PATH + "Item_inv.png");
  // Upgrade UI slots and indicators
  // loadTexture(UI_PATH + "ring_upgrade_slot.png");
  // loadTexture(UI_PATH + "helm_upgrade_slot.png");
  // loadTexture(UI_PATH + "necklace_upgrade_slot.png");
  // loadTexture(UI_PATH + "sword_upgrade_slot.png");
  // loadTexture(UI_PATH + "chest_upgrade_slot.png");
  // loadTexture(UI_PATH + "sheild_upgrade_slot.png"); // filename spelled as provided
  // loadTexture(UI_PATH + "gloves_upgrade_slot.png");
  // loadTexture(UI_PATH + "belt_upgrade_slot.png");
  // loadTexture(UI_PATH + "boots_upgrade_slot.png");
  // loadTexture(UI_PATH + "scrol slot.png");
  // loadTexture(UI_PATH + "upgrade_indicator.png");
  // loadTexture(UI_PATH + "upgrade_succeed.png");
  // loadTexture(UI_PATH + "upgrade_failed.png");
  // loadTexture(UI_PATH + "item_upgrade_slot.png");
  // loadTexture(UI_PATH + "scroll_slot.png");
  // loadTexture(UI_PATH + "upgrade_button.png");
  // Inventory panel (bag)
  // loadTexture(UI_PATH + "Inventory.png");
  
  // Skill icons - DISABLED: Files not found, loaded on demand instead
  // loadTexture(UI_PATH + "dash_ui.png");
  // loadTexture(UI_PATH + "fire_sheild_ui.png");
  // loadTexture(UI_PATH + "firebolt_ui.png");
  // loadTexture(UI_PATH + "flame_wave_ui.png");
  // loadTexture(UI_PATH + "meteor_strike_ui.png");
  // loadTexture(UI_PATH + "dragons_breath_ui.png");
  
  // Fire passive ability icons - DISABLED: Files not found
  // loadTexture(UI_PATH + "fire_mastery.png");
  // loadTexture(UI_PATH + "burning_aura.png");
  // loadTexture(UI_PATH + "inferno_lord.png");
  
  // Fire spell animations
  const std::string SPELLS_PATH = "assets/Textures/Spells/";
  loadSpriteSheetAuto(SPELLS_PATH + "firebolt.png", 4, 4);
  loadSpriteSheetAuto(SPELLS_PATH + "flame_wave_new.png", 27, 27);
  loadSpriteSheetAuto(SPELLS_PATH + "meteor shower-red.png", 24, 8); // 3 rows x 8 columns
  loadSpriteSheetAuto(SPELLS_PATH + "dragon_breath.png", 20, 20);
  loadSpriteSheetAuto(SPELLS_PATH + "fire_sheild.png", 8, 8); // 8-frame 48x48 shield animation
  loadSpriteSheetAuto(SPELLS_PATH + "explosion.png", 16, 16);
  loadSpriteSheetAuto(SPELLS_PATH + "explosion_02.png", 12, 12);
  loadSpriteSheetAuto(SPELLS_PATH + "explosion_03.png", 18, 9); // 2 rows x 9 columns
  loadSpriteSheetAuto(SPELLS_PATH + "fire_ground_impact.png", 6, 6);
  loadSpriteSheetAuto(SPELLS_PATH + "burning_ground.png", 5, 5);
  loadSpriteSheetAuto(SPELLS_PATH + "fire_partical.png", 16, 16);
  loadSpriteSheetAuto(SPELLS_PATH + "smoke cloud.png", 10, 10);

  // Item scrolls (for upgrades and enchants)
  loadTexture("assets/Textures/Items/upgrade_scroll.png");
  loadTexture("assets/Textures/Items/fire_dmg_scroll.png");
  loadTexture("assets/Textures/Items/water_damage_scroll.png");
  loadTexture("assets/Textures/Items/Poison_dmg_scroll.png");
  // Additional variants (not yet used directly in UI but available)
  loadTexture("assets/Textures/Items/armor_enchant_scroll.png");
  loadTexture("assets/Textures/Items/jewellery_upgrade_scroll.png");
  // Item icons used in anvil side item slot
  loadTexture("assets/Textures/Items/ring_01.png");
  loadTexture("assets/Textures/Items/helmet_01.png");
  loadTexture("assets/Textures/Items/necklace_01.png");
  loadTexture("assets/Textures/Items/sword_01.png");
  loadTexture("assets/Textures/Items/chestpeice_01.png");
  loadTexture("assets/Textures/Items/bow_01.png");
  loadTexture("assets/Textures/Items/gloves_01.png");
  loadTexture("assets/Textures/Items/waist_01.png");
  loadTexture("assets/Textures/Items/boots_01.png");
    
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
