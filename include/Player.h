#pragma once

#include <SDL.h>
#include <memory>
#include <vector>
#include <string>
#include <array>
#include <unordered_map>

// Forward declarations
class Projectile;
class Enemy;
class SpellSystem;

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
    ATTACK_END,
    DASHING,
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
    // Computes hit damage with crit and applies durability loss
    int rollMeleeDamageForHit();
    int getFireDamageForHit();
    int getFireShieldDamage();
    SDL_Rect getMeleeHitbox() const; // returns {0,0,0,0} if not attacking
    bool isMeleeHitActive() const;    // true only during the active frames window
    bool consumeMeleeHitIfActive();   // returns true once per swing when entering active window
    
    // Movement
    void handleInput(const InputManager* inputManager);
    void move(float deltaTime);
    
    // Combat
    void performMeleeAttack();
    void performRangedAttack();
    void performDash();
    bool isDashing() const { return currentState == PlayerState::DASHING; }
    float getDashCooldownRemaining() const { return dashCooldownTimer; }
    float getDashCooldownMax() const { return DASH_COOLDOWN_SECONDS; }
    void startShield();
    void stopShield();
    bool hasFireWeapon() const { const auto& sw = equipment[static_cast<size_t>(EquipmentSlot::SWORD)]; return sw.fire > 0; }
    void takeDamage(int damage);
    void takeDamageWithType(int damage, int fireDamage);
    bool isShieldActive() const { return shieldActive; }
    // Fire shield now depends on belt (waist) enchant with fire
    bool hasFireShield() const { return equipment[static_cast<size_t>(EquipmentSlot::WAIST)].fire > 0; }
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
    Direction getDeathDirection() const { return deathDirectionAtDeath; }
    
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

    // Equipment and upgrades
    enum class EquipmentSlot { RING, HELM, NECKLACE, SWORD, CHEST, SHIELD, GLOVE, WAIST, FEET, COUNT };
    struct EquipmentItem {
        std::string name;
        int plusLevel = 0;        // +0..+N
        int basePower = 0;        // contributes to stats
        // Simple elemental modifiers (placeholder values)
        int fire = 0;
        int ice = 0;
        int lightning = 0;
        int poison = 0;
        int resistFire = 0;
        int resistIce = 0;
        int resistLightning = 0;
        int resistPoison = 0;
        // Sword-specific combat stats (used primarily for SWORD slot)
        int attack = 0;                 // base attack power contribution
        float attackSpeedMultiplier = 1.0f; // scales melee attack rate (cooldown divided by this)
        float critChancePercent = 0.0f; // 0..100
        int durability = 0;             // current durability
        int maxDurability = 0;          // cap for durability
    };
    const EquipmentItem& getEquipment(EquipmentSlot slot) const { return equipment[static_cast<int>(slot)]; }
    EquipmentItem& getEquipmentMutable(EquipmentSlot slot) { return equipment[static_cast<int>(slot)]; }
    void upgradeEquipment(EquipmentSlot slot, int deltaPlus);
    void upgradeSpecificItem(class Item* item, int deltaPlus);
    void enchantEquipment(EquipmentSlot slot, const std::string& element, int amount);
    void enchantSpecificItem(class Item* item, const std::string& element, int amount);
    void clearEquipmentSlot(EquipmentSlot slot);
    void syncEquipmentFromItem(EquipmentSlot slot, const class Item* item);

    // Scrolls inventory
    int getUpgradeScrolls() const { return upgradeScrolls; }
    int getElementScrolls(const std::string& element) const;
    void addUpgradeScrolls(int count) { upgradeScrolls += std::max(0, count); addItemToInventory("upgrade_scroll", count); }
    void addElementScrolls(const std::string& element, int count);
    bool consumeUpgradeScroll();
    bool consumeElementScroll(const std::string& element);

    // Enhanced item system
    class ItemSystem* getItemSystem() const { return itemSystem.get(); }
    void addItemToInventoryWithRarity(const std::string& itemId, int amount, int rarity = 0);
    
    // Spell system
    SpellSystem* getSpellSystem() { return spellSystem.get(); }
    const SpellSystem* getSpellSystem() const { return spellSystem.get(); }
    Direction getFacingDirection() const { return currentDirection; }
    
    // Legacy inventory support (for compatibility)
    void addItemToInventory(const std::string& key, int amount);
    int getInventoryCount(const std::string& key) const;
    const std::array<std::unordered_map<std::string,int>,2>& getBags() const { return bags; }

    // Collision
    SDL_Rect getCollisionRect() const;
    
    // Spell helper
    bool isSpellSlotReady(int slot) const;

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
    Direction deathDirectionAtDeath = Direction::DOWN;
    
    // Animation
    SpriteSheet* currentSpriteSheet;
    int currentFrame;
    float frameTimer;
    float frameDuration;
    bool deathAnimationFinished = false;
    // Dash state
    float dashRemainingDistance = 0.0f;
    float dashVelocityX = 0.0f;
    float dashVelocityY = 0.0f;
    float dashFrameDuration = 0.06f; // 8 frames ≈ 0.48s
    float dashCooldownTimer = 0.0f;
    
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

    // Enhanced item system
    std::unique_ptr<class ItemSystem> itemSystem;
    
    // Spell system
    std::unique_ptr<SpellSystem> spellSystem;
    
    // Legacy equipment and upgrade resources (for compatibility)
    std::array<EquipmentItem, static_cast<size_t>(EquipmentSlot::COUNT)> equipment;
    int upgradeScrolls = 0; // "Blessed upgrade scroll"
    std::unordered_map<std::string,int> elementScrolls; // e.g., fire, ice, lightning, poison, resist_fire, etc.
    // Two bags (0 and 1)
    std::array<std::unordered_map<std::string,int>,2> bags;

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
    // Bow-specific idle/run and attack
    SpriteSheet* bowIdleLeftSpriteSheet = nullptr;
    SpriteSheet* bowIdleRightSpriteSheet = nullptr;
    SpriteSheet* bowIdleUpSpriteSheet = nullptr;
    SpriteSheet* bowIdleDownSpriteSheet = nullptr;
    SpriteSheet* bowRunLeftSpriteSheet = nullptr;
    SpriteSheet* bowRunRightSpriteSheet = nullptr;
    SpriteSheet* bowRunUpSpriteSheet = nullptr;
    SpriteSheet* bowRunDownSpriteSheet = nullptr;
    SpriteSheet* bowAttackLeftSpriteSheet = nullptr;
    SpriteSheet* bowAttackRightSpriteSheet = nullptr;
    SpriteSheet* bowAttackUpSpriteSheet = nullptr;
    SpriteSheet* bowAttackDownSpriteSheet = nullptr;
    // Bow end (2 frames)
    SpriteSheet* bowEndLeftSpriteSheet = nullptr;
    SpriteSheet* bowEndRightSpriteSheet = nullptr;
    SpriteSheet* bowEndUpSpriteSheet = nullptr;
    SpriteSheet* bowEndDownSpriteSheet = nullptr;
    // 4-frame end-of-attack wind-down animation (plays after melee or ranged)
    SpriteSheet* attackEndLeftSpriteSheet;
    SpriteSheet* attackEndRightSpriteSheet;
    SpriteSheet* attackEndUpSpriteSheet;
    SpriteSheet* attackEndDownSpriteSheet;
    // Dash sheets (8 frames)
    SpriteSheet* dashLeftSpriteSheet;
    SpriteSheet* dashRightSpriteSheet;
    SpriteSheet* dashUpSpriteSheet;
    SpriteSheet* dashDownSpriteSheet;
    SpriteSheet* hurtLeftSpriteSheet;
    SpriteSheet* hurtRightSpriteSheet;
    SpriteSheet* hurtUpSpriteSheet;
    SpriteSheet* hurtDownSpriteSheet;
    SpriteSheet* deathLeftSpriteSheet;
    SpriteSheet* deathRightSpriteSheet;
    SpriteSheet* deathUpSpriteSheet;
    SpriteSheet* deathDownSpriteSheet;
    // Fire shield visuals
    SpriteSheet* fireShieldSpriteSheet = nullptr;
    int fireShieldFrame = 0;
    float fireShieldTimer = 0.0f;
    float fireShieldFrameDuration = 0.08f;
    bool shieldActive = false;
    // Fire shield gameplay
    float fireShieldTickTimer = 0.0f; // AoE damage tick
    static constexpr float FIRE_SHIELD_TICK_SECONDS = 0.5f;
    static constexpr int FIRE_SHIELD_DAMAGE = 20;
    // Mana drain while channeling
    float fireShieldManaAccumulator = 0.0f;
    static constexpr float FIRE_SHIELD_MANA_DRAIN_PER_SEC = 8.0f; // mana per second
    
    // Helper functions
    void loadSprites();
    SpriteSheet* getSpriteSheetForState(PlayerState state);
    
    // Projectile management
    std::vector<std::unique_ptr<Projectile>> projectiles;
    void updateAttackCooldowns(float deltaTime);
    bool canAttack() const;
    bool isAttackAnimationPlaying() const;
    bool canDash() const { return currentState != PlayerState::DEAD && currentState != PlayerState::DASHING && !isAttackAnimationPlaying() && dashCooldownTimer <= 0.0f; }
    // Weapon visual mode (last used skill): sword vs bow
    enum class WeaponVisual { SWORD, BOW };
    void setWeaponVisual(WeaponVisual w) { lastWeaponVisual = w; }
    WeaponVisual getWeaponVisual() const { return lastWeaponVisual; }
    void calculateExperienceToNext();
    
    // Constants
    static constexpr float DEFAULT_MOVE_SPEED = 150.0f;
    static constexpr float MELEE_ATTACK_COOLDOWN = 0.5f;
    static constexpr float RANGED_ATTACK_COOLDOWN = 1.0f;
    static constexpr float FRAME_DURATION = 0.2f;  // Slower animation for better visibility
    static constexpr float DASH_COOLDOWN_SECONDS = 10.0f;
    static constexpr int BASE_HEALTH = 100;
    static constexpr int BASE_MANA = 50;
    static constexpr int BASE_STRENGTH = 10;
    static constexpr int BASE_INTELLIGENCE = 15;
    static constexpr int POTION_MAX_CHARGES = 10;
    static constexpr int HEALTH_POTION_HEAL = 20;
    static constexpr int MANA_POTION_RESTORE = 20;

    // Melee swing state
    bool meleeHitConsumedThisSwing = false;
    // Schedule a second melee SFX after a delay (does not re-trigger animation)
    bool meleeSecondSfxPending = false;
    float meleeSecondSfxTimer = 0.0f;
    static constexpr float MELEE_SECOND_SFX_DELAY = 1.75f;

    // Footstep timing
    float footstepTimer = 0.0f;
    float footstepInterval = 0.55f; // seconds between steps while walking

    // Potion cooldowns (seconds)
    float healthPotionCooldown = 0.0f;
    float manaPotionCooldown = 0.0f;
    static constexpr float POTION_COOLDOWN_SECONDS = 5.0f;

    // Render scale for the player sprite (1.0 = original). Upscales visual size only.
    float renderScale = 1.75f;
    WeaponVisual lastWeaponVisual = WeaponVisual::SWORD;

    // Equipment helpers
    void updateSwordNameByPlus();
    void updateSwordStatsByPlus();
};
