#pragma once

#include "Enemy.h"
#include <string>
#include <vector>

enum class BossType {
    DEMON_LORD,     // Fire-based boss with charge attacks
    ANCIENT_WIZARD, // Magic boss with teleportation and projectile barrages
    GOBLIN_KING     // Fast boss with summon minions ability
};

enum class BossPhase {
    PHASE_1,        // Normal behavior
    PHASE_2,        // Enhanced behavior at 50% health
    PHASE_3         // Desperate phase at 25% health
};

struct BossAbility {
    std::string name;
    float cooldown;
    float lastUsed;
    bool isReady() const { return (static_cast<float>(SDL_GetTicks()) - lastUsed) / 1000.0f >= cooldown; }
    void use() { lastUsed = static_cast<float>(SDL_GetTicks()); }
};

class Boss : public Enemy {
public:
    Boss(float spawnX, float spawnY, AssetManager* assetManager, BossType type);
    ~Boss() = default;

    // Boss-specific behavior (not overriding since Enemy methods aren't virtual)
    void update(float deltaTime, float playerX, float playerY);
    void render(Renderer* renderer) const;
    void takeDamage(int amount);

    // Boss-specific features
    BossType getBossType() const { return bossType; }
    BossPhase getCurrentPhase() const { return currentPhase; }
    bool isBoss() const { return true; }
    
    // Boss health bar display
    std::string getBossName() const;
    float getHealthPercentage() const { return static_cast<float>(getHealth()) / getMaxHealth(); }
    
    // Boss behavior states
    bool isEnraged() const { return currentPhase >= BossPhase::PHASE_2; }
    bool isDesperate() const { return currentPhase == BossPhase::PHASE_3; }
    
    // Minion management (for Goblin King)
    void spawnMinion(float playerX, float playerY);
    const std::vector<std::unique_ptr<Enemy>>& getMinions() const { return minions; }
    void updateMinions(float deltaTime, float playerX, float playerY);
    void renderMinions(Renderer* renderer) const;

private:
    BossType bossType;
    BossPhase currentPhase;
    
    // Boss abilities
    std::vector<BossAbility> abilities;
    float globalAbilityCooldown = 0.0f;
    
    // Boss-specific mechanics
    float teleportCooldown = 0.0f;
    float chargeCooldown = 0.0f;
    float summonCooldown = 0.0f;
    float barrageTimer = 0.0f;
    bool isCharging = false;
    float chargeDirection = 0.0f; // angle in radians
    
    // Phase transition
    bool hasTransitioned[3] = {false, false, false}; // track phase transitions
    
    // Minions (for Goblin King)
    std::vector<std::unique_ptr<Enemy>> minions;
    int maxMinions = 3;
    
    // Boss initialization
    void initializeBoss();
    void setupAbilities();
    
    // Phase management
    void checkPhaseTransition();
    void enterPhase(BossPhase newPhase);
    
    // Boss abilities
    void useTeleport(float playerX, float playerY);
    void useCharge(float playerX, float playerY);
    void useProjectileBarrage(float playerX, float playerY, AssetManager* assetManager);
    void useSummonMinions(float playerX, float playerY);
    
    // AI behavior per boss type
    void updateDemonLord(float deltaTime, float playerX, float playerY);
    void updateAncientWizard(float deltaTime, float playerX, float playerY);
    void updateGoblinKing(float deltaTime, float playerX, float playerY);
    
    // Visual effects
    void createDeathEffect();
    
    // Utility functions
    float distanceToPlayer(float playerX, float playerY) const;
    bool canSeePlayer(float playerX, float playerY) const;
    void facePlayer(float playerX, float playerY);
};