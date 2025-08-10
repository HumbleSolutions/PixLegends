#pragma once

#include <SDL2/SDL.h>
#include <memory>
#include <vector>
#include <string>

// Forward declarations
class Projectile;
class Enemy;

// Forward declarations
class Game;
class Renderer;
class SpriteSheet;
class InputManager;
class Object;
struct PlayerSave; // from Database.h

enum class PlayerState {
    IDLE,
    WALKING,
    ATTACKING_MELEE,
    ATTACKING_RANGED,
    HURT,
    DEAD
};

enum class Direction {
    DOWN,
    UP,
    LEFT,
    RIGHT
};

class Player {
public:
    explicit Player(Game* game);
    ~Player() = default;

    // Core update and render
    void update(float deltaTime);
    void render(Renderer* renderer);
    
    // Projectile management
    void updateProjectiles(float deltaTime);
    void renderProjectiles(Renderer* renderer);
    std::vector<std::unique_ptr<Projectile>>& getProjectiles() { return projectiles; }
    const std::vector<std::unique_ptr<Projectile>>& getProjectiles() const { return projectiles; }

    // Melee combat helpers
    bool isMeleeAttacking() const { return currentState == PlayerState::ATTACKING_MELEE; }
    int getMeleeDamage() const { return meleeDamage; }
    SDL_Rect getMeleeHitbox() const; // returns {0,0,0,0} if not attacking
    bool isMeleeHitActive() const;    // true only during the active frames window
    bool consumeMeleeHitIfActive();   // returns true once per swing when entering active window
    
    // Movement
    void handleInput(const InputManager* inputManager);
    void move(float deltaTime);
    
    // Combat
    void performMeleeAttack();
    void performRangedAttack();
    void takeDamage(int damage);
    bool isDead() const { return currentState == PlayerState::DEAD; }
    void respawn(float respawnX, float respawnY);
    void setSpawnPoint(float sx, float sy) { spawnX = sx; spawnY = sy; }
    float getSpawnX() const { return spawnX; }
    float getSpawnY() const { return spawnY; }
    bool isDeathAnimationFinished() const { return deathAnimationFinished; }
    
    // Interaction
    void interact();
    Object* getNearbyInteractableObject() const;
    std::string getCurrentInteractionPrompt() const;
    std::string getLastLootNotification() const { return lastLootNotification; }
    void clearLootNotification() { lastLootNotification.clear(); }
    
    // Animation
    void updateAnimation(float deltaTime);
    void setState(PlayerState newState);
    void setDirection(Direction newDirection);
    
    // Stats and progression
    void gainExperience(int xp);
    void levelUp();
    void heal(int amount);
    void useMana(int amount);
    void addGold(int amount);
    void spendGold(int amount);

    // Persistence helpers
    void applySaveState(const PlayerSave& state);
    PlayerSave makeSaveState() const;

    // Potions
    bool consumeHealthPotion();
    bool consumeManaPotion();
    void addHealthPotionCharges(int charges);
    void addManaPotionCharges(int charges);
    int getHealthPotionCharges() const { return healthPotionCharges; }
    int getManaPotionCharges() const { return manaPotionCharges; }
    int getMaxPotionCharges() const { return POTION_MAX_CHARGES; }
    // Effective charges for UI when cheats are enabled
    int getEffectiveHealthPotionCharges() const;
    int getEffectiveManaPotionCharges() const;
    
    // Getters
    float getX() const { return x; }
    float getY() const { return y; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    PlayerState getState() const { return currentState; }
    Direction getDirection() const { return currentDirection; }
    
    // Stats
    int getHealth() const { return health; }
    int getMaxHealth() const { return maxHealth; }
    int getMana() const { return mana; }
    int getMaxMana() const { return maxMana; }
    int getLevel() const { return level; }
    int getExperience() const { return experience; }
    int getExperienceToNext() const { return experienceToNext; }
    int getStrength() const { return strength; }
    int getIntelligence() const { return intelligence; }
    int getGold() const { return gold; }

    // Collision
    SDL_Rect getCollisionRect() const;

private:
    Game* game;
    
    // Position and size
    float x, y;
    float spawnX, spawnY;
    int width, height;
    float moveSpeed;
    
    // State
    PlayerState currentState;
    Direction currentDirection;
    
    // Animation
    SpriteSheet* currentSpriteSheet;
    int currentFrame;
    float frameTimer;
    float frameDuration;
    bool deathAnimationFinished = false;
    
    // Combat
    float meleeAttackCooldown;
    float rangedAttackCooldown;
    float meleeAttackTimer;
    float rangedAttackTimer;
    int meleeDamage;
    int rangedDamage;
    
    // Stats
    int health, maxHealth;
    int mana, maxMana;
    int level;
    int experience, experienceToNext;
    int strength, intelligence;
    int gold;

    // Potions
    int healthPotionCharges;
    int manaPotionCharges;
    
    // Loot notification
    std::string lastLootNotification;
    
    // Animation spritesheets
    SpriteSheet* idleLeftSpriteSheet;
    SpriteSheet* idleRightSpriteSheet;
    SpriteSheet* idleUpSpriteSheet;
    SpriteSheet* idleDownSpriteSheet;
    SpriteSheet* walkLeftSpriteSheet;
    SpriteSheet* walkRightSpriteSheet;
    SpriteSheet* walkUpSpriteSheet;
    SpriteSheet* walkDownSpriteSheet;
    SpriteSheet* meleeAttackLeftSpriteSheet;
    SpriteSheet* meleeAttackRightSpriteSheet;
    SpriteSheet* meleeAttackUpSpriteSheet;
    SpriteSheet* meleeAttackDownSpriteSheet;
    SpriteSheet* rangedAttackLeftSpriteSheet;
    SpriteSheet* rangedAttackRightSpriteSheet;
    SpriteSheet* rangedAttackUpSpriteSheet;
    SpriteSheet* rangedAttackDownSpriteSheet;
    SpriteSheet* hurtLeftSpriteSheet;
    SpriteSheet* hurtRightSpriteSheet;
    SpriteSheet* deathSpriteSheet;
    
    // Helper functions
    void loadSprites();
    SpriteSheet* getSpriteSheetForState(PlayerState state);
    
    // Projectile management
    std::vector<std::unique_ptr<Projectile>> projectiles;
    void updateAttackCooldowns(float deltaTime);
    bool canAttack() const;
    void calculateExperienceToNext();
    
    // Constants
    static constexpr float DEFAULT_MOVE_SPEED = 150.0f;
    static constexpr float MELEE_ATTACK_COOLDOWN = 0.5f;
    static constexpr float RANGED_ATTACK_COOLDOWN = 1.0f;
    static constexpr float FRAME_DURATION = 0.2f;  // Slower animation for better visibility
    static constexpr int BASE_HEALTH = 100;
    static constexpr int BASE_MANA = 50;
    static constexpr int BASE_STRENGTH = 10;
    static constexpr int BASE_INTELLIGENCE = 15;
    static constexpr int POTION_MAX_CHARGES = 10;
    static constexpr int HEALTH_POTION_HEAL = 20;
    static constexpr int MANA_POTION_RESTORE = 20;

    // Melee swing state
    bool meleeHitConsumedThisSwing = false;

    // Footstep timing
    float footstepTimer = 0.0f;
    float footstepInterval = 0.55f; // seconds between steps while walking

    // Potion cooldowns (seconds)
    float healthPotionCooldown = 0.0f;
    float manaPotionCooldown = 0.0f;
    static constexpr float POTION_COOLDOWN_SECONDS = 5.0f;

    // Render scale for the player sprite (1.0 = original). Upscales visual size only.
    float renderScale = 1.0f;
};
