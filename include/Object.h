#pragma once

#include <string>
#include <vector>
#include <SDL2/SDL.h>

// Forward declarations
class Texture;
class SpriteSheet;
class AssetManager;

enum class LootType {
    GOLD,
    EXPERIENCE,
    HEALTH_POTION,
    MANA_POTION
};

struct Loot {
    LootType type;
    int amount;
    std::string name;
    
    Loot(LootType lootType, int lootAmount, const std::string& lootName = "")
        : type(lootType), amount(lootAmount), name(lootName) {}
};

enum class ObjectType {
    CHEST_OPENED,
    CHEST_UNOPENED,
    CLAY_POT,
    FLAG,
    WOOD_CRATE,
    STEEL_CRATE,
    WOOD_FENCE,
    WOOD_FENCE_BROKEN,
    WOOD_SIGN,
    BONFIRE,
    // Interactable upgrade station for equipment items
    MAGIC_ANVIL,
    // Experience orbs
    EXP_ORB1,
    EXP_ORB2,
    EXP_ORB3
};

class Object {
public:
    Object(ObjectType type, int x, int y, const std::string& texturePath);
    ~Object() = default;

    // Core functions
    void update(float deltaTime);
    void render(SDL_Renderer* renderer, int cameraX, int cameraY, int tileSize, float zoom);
    void setPositionPixels(float px, float py) { pixelX = px; pixelY = py; hasPixelPos = true; }
    float getPixelX() const { return hasPixelPos ? pixelX : static_cast<float>(x *  tileSizeHint); }
    float getPixelY() const { return hasPixelPos ? pixelY : static_cast<float>(y * tileSizeHint); }
    void setTileSizeHint(int ts) { tileSizeHint = ts; }
    bool isCollectible() const { return collectible; }
    void setCollectible(bool c) { collectible = c; }
    void setSpawnTicks(Uint32 t) { spawnTicksMs = t; }
    Uint32 getSpawnTicks() const { return spawnTicksMs; }
    void setMagnetDelaySeconds(float s) { magnetDelaySeconds = s; }
    float getMagnetDelaySeconds() const { return magnetDelaySeconds; }
    
    // Getters
    int getX() const { return x; }
    int getY() const { return y; }
    ObjectType getType() const { return type; }
    bool isInteractable() const { return interactable; }
    bool isWalkable() const { return walkable; }
    bool isVisible() const { return visible; }
    
    // Setters
    void setPosition(int xPos, int yPos) { this->x = xPos; this->y = yPos; }
    void setInteractable(bool isInteractable) { this->interactable = isInteractable; }
    void setWalkable(bool isWalkable) { this->walkable = isWalkable; }
    void setVisible(bool isVisible) { this->visible = isVisible; }
    
    // Interaction
    virtual void interact();
    bool isInInteractionRange(int playerX, int playerY, int tileSize) const;
    std::string getInteractionPrompt() const;
    
    // Loot system
    void addLoot(const Loot& loot);
    std::vector<Loot> getLoot();
    void clearLoot();
    bool hasLoot() const { return !loot.empty(); }
    
    // Texture management
    void setTexture(Texture* objTexture) { this->texture = objTexture; }
    Texture* getTexture() const { return texture; }
    void setSpriteSheet(SpriteSheet* objSpriteSheet) { this->spriteSheet = objSpriteSheet; }
    SpriteSheet* getSpriteSheet() const { return spriteSheet; }
    void changeTexture(const std::string& newTexturePath);
    void setAssetManager(AssetManager* manager) { assetManager = manager; }

    // Global pulse to play Magic Anvil animation once when upgrades happen
    static void setMagicAnvilPulse(float seconds);

protected:
    ObjectType type;
    int x, y;  // World coordinates
    std::string texturePath;
    Texture* texture;
    SpriteSheet* spriteSheet;  // For animated objects
    AssetManager* assetManager;  // Reference to asset manager for texture changes
    // Optional pixel-precise placement for small orbs
    bool hasPixelPos = false;
    float pixelX = 0.0f;
    float pixelY = 0.0f;
    int tileSizeHint = 32;
    
    // Properties
    bool interactable;
    bool walkable;
    bool visible;
    bool collectible = false; // auto-pickup on touch
    Uint32 spawnTicksMs = 0;   // creation time (SDL ticks)
    float magnetDelaySeconds = 0.0f; // delay before magnet can activate
    
    // Animation (for future use)
    float animationTime;
    int currentFrame;
    int totalFrames;
    float frameDuration;
    
    // Loot
    std::vector<Loot> loot;
};
