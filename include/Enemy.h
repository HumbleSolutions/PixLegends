#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <memory>
#include <vector>
#include "Projectile.h"

// Forward declarations
class SpriteSheet;
class AssetManager;
class Renderer;

enum class EnemyState {
    IDLE,
    FLYING,
    ATTACKING,
    HURT,
    DEAD
};

enum class EnemyDirection {
    LEFT,
    RIGHT
};

enum class EnemyKind {
    Demon,
    Wizard,
    Goblin
};

enum class PackRarity {
    Trash,   // grey
    Common,  // white
    Magic,   // blue
    Elite    // gold
};

class Enemy {
public:
    Enemy(float spawnX, float spawnY, AssetManager* assetManager, EnemyKind kind);
    ~Enemy() = default;

    void update(float deltaTime, float playerX, float playerY);
    void render(Renderer* renderer) const;
    void renderProjectiles(Renderer* renderer) const;

    float getX() const { return x; }
    float getY() const { return y; }
    int getWidth() const { return currentSpriteSheet ? currentSpriteSheetFrameWidth : width; }
    int getHeight() const { return currentSpriteSheet ? currentSpriteSheetFrameHeight : height; }

    bool isDead() const { return currentState == EnemyState::DEAD; }

    // Combat
    void takeDamage(int amount);
    int getHealth() const { return health; }
    int getMaxHealth() const { return maxHealth; }
    SDL_Rect getCollisionRect() const;
    bool isWithinAttackRange(float playerX, float playerY) const;
    bool isAttackReady() const { return attackCooldownTimer <= 0.0f; }
    void consumeAttackCooldown() { attackCooldownTimer = attackCooldownSeconds; }
    int getContactDamage() const { return contactDamage; }
    bool getIsAggroed() const { return isAggroed; }
    const char* getDisplayName() const {
        switch (kind) {
            case EnemyKind::Demon: return "Demon";
            case EnemyKind::Wizard: return "Wizard";
            case EnemyKind::Goblin: return "Goblin";
        }
        return "Enemy";
    }
    PackRarity getPackRarity() const { return packRarity; }
    void setPackRarity(PackRarity r) { packRarity = r; }
    float getRenderScale() const { return renderScale; }
    void setRenderScale(float s) { renderScale = s; }
    EnemyKind getKind() const { return kind; }
    // Loot drop bookkeeping
    bool isLootDropped() const { return lootDropped; }
    void markLootDropped() { lootDropped = true; }
    // Despawn timing
    Uint32 getDeathTicksMs() const { return deathTicksMs; }
    bool isDespawnReady(Uint32 nowTicks, Uint32 ttlMs = 60000) const {
        return currentState == EnemyState::DEAD && deathTicksMs > 0 && (nowTicks - deathTicksMs) >= ttlMs;
    }

    // Expose basic dimension for simple collision if needed
    int getRawWidth() const { return width; }
    int getRawHeight() const { return height; }

    // Spawn/reset
    void resetToSpawn();
    const std::vector<std::unique_ptr<Projectile>>& getProjectiles() const { return projectiles; }

protected:
    // Core
    float x;
    float y;
    int width;
    int height;
    float moveSpeed;
    int health;
    int maxHealth;
    AssetManager* assets = nullptr;

    // State
    EnemyState currentState;
    EnemyDirection currentDirection;

    // Animation
    SpriteSheet* currentSpriteSheet;
    int currentFrame;
    float frameTimer;
    float frameDuration;
    int currentSpriteSheetFrameWidth;
    int currentSpriteSheetFrameHeight;

    // Sprites
    SpriteSheet* idleLeftSpriteSheet;
    SpriteSheet* idleRightSpriteSheet;
    SpriteSheet* flyingLeftSpriteSheet;
    SpriteSheet* flyingRightSpriteSheet;
    SpriteSheet* attackLeftSpriteSheet;
    SpriteSheet* attackRightSpriteSheet;
    SpriteSheet* hurtLeftSpriteSheet;
    SpriteSheet* hurtRightSpriteSheet;
    SpriteSheet* deathSpriteSheet;

    // Helpers
    void loadSprites(AssetManager* assetManager);
    SpriteSheet* pickSpriteSheetForState(EnemyState state) const;
    void setState(EnemyState newState);
    void setDirection(EnemyDirection newDirection);
    void updateAnimation(float deltaTime);
    void updateProjectiles(float deltaTime);
    void fireProjectileTowards(float targetX, float targetY, AssetManager* assetManager);

    // Combat/behavior
    float aggroRadius;   // pixels
    float attackRange;   // pixels
    bool isAggroed = false;

    // Attack control
    float attackCooldownSeconds;
    float attackCooldownTimer;
    int contactDamage;

    // Spawn position
    float spawnX;
    float spawnY;

    // Type/behavior
    EnemyKind kind;
    PackRarity packRarity = PackRarity::Common;
    float renderScale = 2.0f;
    // Ranged attack (wizard)
    float rangedCooldownSeconds = 1.2f;
    float rangedCooldownTimer = 0.0f;
    float rangedRange = 600.0f;
    std::vector<std::unique_ptr<Projectile>> projectiles;

    bool lootDropped = false;
    Uint32 deathTicksMs = 0; // time of death for corpse despawn
};


