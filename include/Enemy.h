#pragma once

#include <SDL.h>
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
    DEAD,
    TRANSFORMING,
    JUMPING,
    DASHING,
    SUPER_ATTACKING
};

enum class EnemyDirection {
    LEFT,
    RIGHT
};

enum class EnemyKind {
    // Currently implemented
    Demon,
    Wizard,
    Goblin,
    Skeleton,
    
    // Trash tier (Grey)
    Imp,
    FlyingEye,
    PoisonSkull,
    
    // Common tier (White)
    Lizardman,
    DwarfWarrior,
    Harpy,
    
    // Magic tier (Blue)
    SkeletonMage,
    Pyromancer,
    Witch,
    
    // Elite tier (Gold)
    Dragon,
    Minotaur,
    StoneGolem,
    HugeKnight,
    Centaur,
    HeadlessHorseman,
    Cyclops,
    Medusa,
    Cerberus,
    Gryphon,
    Gargoyle,
    Werewolf,
    Mimic,
    MaskedOrc,
    KoboldWarrior,
    SatyrArcher,
    BabyDragon
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
    void update(float deltaTime, float playerX, float playerY, const std::vector<std::unique_ptr<Enemy>>& otherEnemies);
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
            // Currently implemented
            case EnemyKind::Demon: return "Demon";
            case EnemyKind::Wizard: return "Wizard";
            case EnemyKind::Goblin: return "Goblin";
            case EnemyKind::Skeleton: return "Skeleton";
            
            // Trash tier
            case EnemyKind::Imp: return "Imp";
            case EnemyKind::FlyingEye: return "Flying Eye";
            case EnemyKind::PoisonSkull: return "Poison Skull";
            
            // Common tier
            case EnemyKind::Lizardman: return "Lizardman";
            case EnemyKind::DwarfWarrior: return "Dwarf Warrior";
            case EnemyKind::Harpy: return "Harpy";
            
            // Magic tier
            case EnemyKind::SkeletonMage: return "Skeleton Mage";
            case EnemyKind::Pyromancer: return "Pyromancer";
            case EnemyKind::Witch: return "Witch";
            
            // Elite tier
            case EnemyKind::Dragon: return "Dragon";
            case EnemyKind::Minotaur: return "Minotaur";
            case EnemyKind::StoneGolem: return "Stone Golem";
            case EnemyKind::HugeKnight: return "Huge Knight";
            case EnemyKind::Centaur: return "Centaur";
            case EnemyKind::HeadlessHorseman: return "Headless Horseman";
            case EnemyKind::Cyclops: return "Cyclops";
            case EnemyKind::Medusa: return "Medusa";
            case EnemyKind::Cerberus: return "Cerberus";
            case EnemyKind::Gryphon: return "Gryphon";
            case EnemyKind::Gargoyle: return "Gargoyle";
            case EnemyKind::Werewolf: return "Werewolf";
            case EnemyKind::Mimic: return "Mimic";
            case EnemyKind::MaskedOrc: return "Masked Orc";
            case EnemyKind::KoboldWarrior: return "Kobold Warrior";
            case EnemyKind::SatyrArcher: return "Satyr Archer";
            case EnemyKind::BabyDragon: return "Baby Dragon";
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
    
    // Dual attack support (for Dragon)
    SpriteSheet* attack2LeftSpriteSheet = nullptr;
    SpriteSheet* attack2RightSpriteSheet = nullptr;
    bool hasDualAttacks = false;
    bool useAttack2 = false;  // Toggle between attack types
    
    // Transformation support (for Werewolf)
    SpriteSheet* transformationSpriteSheet = nullptr;
    SpriteSheet* humanIdleLeftSpriteSheet = nullptr;
    SpriteSheet* humanIdleRightSpriteSheet = nullptr;
    bool isTransformed = false;
    bool hasTransformationAbility = false;
    
    // Advanced combat support (for Kobold Warrior)
    SpriteSheet* attack3LeftSpriteSheet = nullptr;
    SpriteSheet* attack3RightSpriteSheet = nullptr;
    SpriteSheet* superAttackLeftSpriteSheet = nullptr;
    SpriteSheet* superAttackRightSpriteSheet = nullptr;
    SpriteSheet* jumpLeftSpriteSheet = nullptr;
    SpriteSheet* jumpRightSpriteSheet = nullptr;
    SpriteSheet* dashLeftSpriteSheet = nullptr;
    SpriteSheet* dashRightSpriteSheet = nullptr;
    bool hasAdvancedAbilities = false;
    int currentAttackType = 1;  // 1, 2, 3, or 4 (super)
    bool isJumping = false;
    bool isDashing = false;
    float dashCooldown = 0.0f;
    float jumpCooldown = 0.0f;
    float superAttackCooldown = 0.0f;
    float dashTargetX = 0.0f;
    float dashTargetY = 0.0f;
    
    // For enemies with only one direction sprites
    bool usesSpriteFlipping = false;
    bool baseSpriteFacesLeft = false;  // true if base sprite faces left, false if faces right

    // Helpers
    void loadSprites(AssetManager* assetManager);
    SpriteSheet* pickSpriteSheetForState(EnemyState state) const;
    void setState(EnemyState newState);
    void setDirection(EnemyDirection newDirection);
    void updateAnimation(float deltaTime);
    void updateProjectiles(float deltaTime);
    void fireProjectileTowards(float targetX, float targetY, AssetManager* assetManager, const std::string& projectileSprite = "", int frames = 0, bool rotateByDirection = false);
    void triggerTransformation();  // For werewolf transformation

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


