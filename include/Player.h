#pragma once

#include <SDL2/SDL.h>
#include <memory>
#include <vector>
#include <string>

// Forward declarations
class Projectile;

// Forward declarations
class Game;
class Renderer;
class SpriteSheet;
class InputManager;
class Object;

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
    
    // Movement
    void handleInput(const InputManager* inputManager);
    void move(float deltaTime);
    
    // Combat
    void performMeleeAttack();
    void performRangedAttack();
    void takeDamage(int damage);
    
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

private:
    Game* game;
    
    // Position and size
    float x, y;
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
    
    // Loot notification
    std::string lastLootNotification;
    
    // Animation spritesheets
    SpriteSheet* idleLeftSpriteSheet;
    SpriteSheet* idleRightSpriteSheet;
    SpriteSheet* walkLeftSpriteSheet;
    SpriteSheet* walkRightSpriteSheet;
    SpriteSheet* meleeAttackLeftSpriteSheet;
    SpriteSheet* meleeAttackRightSpriteSheet;
    SpriteSheet* rangedAttackLeftSpriteSheet;
    SpriteSheet* rangedAttackRightSpriteSheet;
    SpriteSheet* hurtSpriteSheet;
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
};
