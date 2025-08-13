#include "Boss.h"
#include "AssetManager.h"
#include "Renderer.h"
#include "LootGenerator.h"
#include <iostream>
#include <cmath>

Boss::Boss(float spawnX, float spawnY, AssetManager* assetManager, BossType type) 
    : Enemy(spawnX, spawnY, assetManager, EnemyKind::Demon), // Base enemy constructor
      bossType(type), currentPhase(BossPhase::PHASE_1) {
    
    initializeBoss();
    setupAbilities();
}

void Boss::initializeBoss() {
    // Configure boss stats based on type
    switch (bossType) {
        case BossType::DEMON_LORD:
            // High health, moderate speed, fire-based attacks
            health = maxHealth = 500;
            moveSpeed = 80.0f;
            contactDamage = 25;
            aggroRadius = 800.0f;
            attackRange = 100.0f;
            attackCooldownSeconds = 2.0f;
            renderScale = 3.0f; // Larger than normal enemies
            packRarity = PackRarity::Elite;
            break;
            
        case BossType::ANCIENT_WIZARD:
            // Moderate health, slow speed, powerful magic
            health = maxHealth = 350;
            moveSpeed = 60.0f;
            contactDamage = 20;
            aggroRadius = 900.0f;
            attackRange = 600.0f;
            attackCooldownSeconds = 1.5f;
            renderScale = 2.5f;
            packRarity = PackRarity::Elite;
            break;
            
        case BossType::GOBLIN_KING:
            // Moderate health, high speed, summons minions
            health = maxHealth = 400;
            moveSpeed = 120.0f;
            contactDamage = 20;
            aggroRadius = 700.0f;
            attackRange = 80.0f;
            attackCooldownSeconds = 1.0f;
            renderScale = 2.2f;
            packRarity = PackRarity::Elite;
            break;
    }
}

void Boss::setupAbilities() {
    abilities.clear();
    
    switch (bossType) {
        case BossType::DEMON_LORD:
            abilities.push_back({"Charge Attack", 8.0f, 0.0f});
            abilities.push_back({"Fire Aura", 15.0f, 0.0f});
            break;
            
        case BossType::ANCIENT_WIZARD:
            abilities.push_back({"Teleport", 6.0f, 0.0f});
            abilities.push_back({"Projectile Barrage", 10.0f, 0.0f});
            abilities.push_back({"Magic Shield", 20.0f, 0.0f});
            break;
            
        case BossType::GOBLIN_KING:
            abilities.push_back({"Summon Minions", 12.0f, 0.0f});
            abilities.push_back({"Berserker Rage", 18.0f, 0.0f});
            break;
    }
}

void Boss::update(float deltaTime, float playerX, float playerY) {
    // Check phase transitions based on health
    checkPhaseTransition();
    
    // Update global ability cooldown
    if (globalAbilityCooldown > 0.0f) {
        globalAbilityCooldown -= deltaTime;
    }
    
    // Boss-specific AI
    switch (bossType) {
        case BossType::DEMON_LORD:
            updateDemonLord(deltaTime, playerX, playerY);
            break;
        case BossType::ANCIENT_WIZARD:
            updateAncientWizard(deltaTime, playerX, playerY);
            break;
        case BossType::GOBLIN_KING:
            updateGoblinKing(deltaTime, playerX, playerY);
            break;
    }
    
    // Update minions
    updateMinions(deltaTime, playerX, playerY);
    
    // Call base Enemy update for animation and basic behavior
    Enemy::update(deltaTime, playerX, playerY);
}

void Boss::render(Renderer* renderer) const {
    // Render boss with special effects based on phase
    Enemy::render(renderer);
    
    // Add phase-based visual effects
    if (currentPhase >= BossPhase::PHASE_2) {
        // Add red tint for enraged phase
        // TODO: Implement shader effects or overlay
    }
    
    if (currentPhase == BossPhase::PHASE_3) {
        // Add flashing effect for desperate phase
        // TODO: Implement flashing animation
    }
    
    // Render minions
    renderMinions(renderer);
}

void Boss::renderMinions(Renderer* renderer) const {
    for (const auto& minion : minions) {
        if (minion && !minion->isDead()) {
            minion->render(renderer);
        }
    }
}

void Boss::takeDamage(int amount) {
    Enemy::takeDamage(amount);
    
    // Boss-specific damage reactions
    if (currentPhase == BossPhase::PHASE_3 && health > 0) {
        // Desperate phase: chance to use ability immediately
        if (rand() % 100 < 30) { // 30% chance
            globalAbilityCooldown = 0.0f; // Reset cooldown
        }
    }
}

void Boss::checkPhaseTransition() {
    float healthPercent = static_cast<float>(health) / maxHealth;
    
    if (healthPercent <= 0.25f && currentPhase != BossPhase::PHASE_3) {
        enterPhase(BossPhase::PHASE_3);
    } else if (healthPercent <= 0.5f && currentPhase == BossPhase::PHASE_1) {
        enterPhase(BossPhase::PHASE_2);
    }
}

void Boss::enterPhase(BossPhase newPhase) {
    if (currentPhase == newPhase) return;
    
    std::cout << getBossName() << " enters phase " << static_cast<int>(newPhase) + 1 << "!" << std::endl;
    
    currentPhase = newPhase;
    
    // Phase-specific buffs
    switch (newPhase) {
        case BossPhase::PHASE_2:
            // Increase aggression
            moveSpeed *= 1.3f;
            attackCooldownSeconds *= 0.8f;
            break;
            
        case BossPhase::PHASE_3:
            // Desperate phase
            moveSpeed *= 1.5f;
            attackCooldownSeconds *= 0.6f;
            contactDamage = static_cast<int>(contactDamage * 1.2f);
            break;
            
        default:
            break;
    }
}

void Boss::updateDemonLord(float deltaTime, float playerX, float playerY) {
    float distance = distanceToPlayer(playerX, playerY);
    
    // Use abilities if ready
    if (globalAbilityCooldown <= 0.0f) {
        // Charge attack
        if (abilities[0].isReady() && distance > 200.0f && distance < 600.0f) {
            useCharge(playerX, playerY);
            abilities[0].use();
            globalAbilityCooldown = 1.0f;
            return;
        }
        
        // Fire aura in desperate phase
        if (currentPhase == BossPhase::PHASE_3 && abilities[1].isReady() && distance < 300.0f) {
            // TODO: Implement fire aura damage
            std::cout << getBossName() << " unleashes Fire Aura!" << std::endl;
            abilities[1].use();
            globalAbilityCooldown = 2.0f;
        }
    }
    
    // Handle charging behavior
    if (isCharging) {
        // Move in charge direction
        float chargeSpeed = moveSpeed * 2.5f;
        x += cos(chargeDirection) * chargeSpeed * deltaTime;
        y += sin(chargeDirection) * chargeSpeed * deltaTime;
        
        chargeCooldown -= deltaTime;
        if (chargeCooldown <= 0.0f) {
            isCharging = false;
            std::cout << getBossName() << " stops charging." << std::endl;
        }
    }
}

void Boss::updateAncientWizard(float deltaTime, float playerX, float playerY) {
    float distance = distanceToPlayer(playerX, playerY);
    
    if (globalAbilityCooldown <= 0.0f) {
        // Teleport if player is too close
        if (abilities[0].isReady() && distance < 150.0f) {
            useTeleport(playerX, playerY);
            abilities[0].use();
            globalAbilityCooldown = 1.0f;
            return;
        }
        
        // Projectile barrage
        if (abilities[1].isReady() && distance > 200.0f && distance < 700.0f) {
            useProjectileBarrage(playerX, playerY, assets);
            abilities[1].use();
            globalAbilityCooldown = 2.0f;
            return;
        }
    }
}

void Boss::updateGoblinKing(float deltaTime, float playerX, float playerY) {
    float distance = distanceToPlayer(playerX, playerY);
    
    if (globalAbilityCooldown <= 0.0f) {
        // Summon minions if we don't have enough
        if (abilities[0].isReady() && minions.size() < maxMinions) {
            useSummonMinions(playerX, playerY);
            abilities[0].use();
            globalAbilityCooldown = 1.5f;
            return;
        }
    }
}

void Boss::useCharge(float playerX, float playerY) {
    std::cout << getBossName() << " charges toward the player!" << std::endl;
    
    // Calculate direction to player
    float dx = playerX - getX();
    float dy = playerY - getY();
    chargeDirection = atan2(dy, dx);
    
    isCharging = true;
    chargeCooldown = 2.0f; // Charge for 2 seconds
    
    // Face the player
    facePlayer(playerX, playerY);
}

void Boss::useTeleport(float playerX, float playerY) {
    std::cout << getBossName() << " teleports!" << std::endl;
    
    // Teleport to a position behind or to the side of the player
    float angle = atan2(getY() - playerY, getX() - playerX) + (rand() % 2 == 0 ? 1.57f : -1.57f); // +/- 90 degrees
    float teleportDistance = 200.0f + (rand() % 100);
    
    x = playerX + cos(angle) * teleportDistance;
    y = playerY + sin(angle) * teleportDistance;
    
    // TODO: Add teleport particle effect
}

void Boss::useProjectileBarrage(float playerX, float playerY, AssetManager* assetManager) {
    std::cout << getBossName() << " launches a projectile barrage!" << std::endl;
    
    // Fire multiple projectiles in a spread pattern
    for (int i = 0; i < 5; i++) {
        float spreadAngle = (i - 2) * 0.3f; // -0.6 to +0.6 radians spread
        float dx = playerX - getX();
        float dy = playerY - getY();
        float baseAngle = atan2(dy, dx);
        float finalAngle = baseAngle + spreadAngle;
        
        float targetX = getX() + cos(finalAngle) * 500.0f;
        float targetY = getY() + sin(finalAngle) * 500.0f;
        
        fireProjectileTowards(targetX, targetY, assets);
    }
}

void Boss::useSummonMinions(float playerX, float playerY) {
    std::cout << getBossName() << " summons minions!" << std::endl;
    
    // Spawn 1-2 goblin minions
    int numToSpawn = 1 + (rand() % 2);
    for (int i = 0; i < numToSpawn && minions.size() < maxMinions; i++) {
        spawnMinion(playerX, playerY);
    }
}

void Boss::spawnMinion(float playerX, float playerY) {
    // Spawn minion at a random position around the boss
    float angle = (rand() % 360) * 3.14159f / 180.0f;
    float distance = 80.0f + (rand() % 40);
    
    float minionX = getX() + cos(angle) * distance;
    float minionY = getY() + sin(angle) * distance;
    
    auto minion = std::make_unique<Enemy>(minionX, minionY, assets, EnemyKind::Goblin);
    // Note: setPackRarity and setRenderScale are not available, we'll access directly
    minions.push_back(std::move(minion));
}

void Boss::updateMinions(float deltaTime, float playerX, float playerY) {
    // Update all minions and remove dead ones
    for (auto it = minions.begin(); it != minions.end();) {
        if ((*it)->isDead()) {
            it = minions.erase(it);
        } else {
            (*it)->update(deltaTime, playerX, playerY);
            ++it;
        }
    }
}

std::string Boss::getBossName() const {
    switch (bossType) {
        case BossType::DEMON_LORD:     return "Demon Lord";
        case BossType::ANCIENT_WIZARD: return "Ancient Wizard";
        case BossType::GOBLIN_KING:    return "Goblin King";
        default:                       return "Unknown Boss";
    }
}

float Boss::distanceToPlayer(float playerX, float playerY) const {
    float dx = playerX - getX();
    float dy = playerY - getY();
    return sqrt(dx * dx + dy * dy);
}

bool Boss::canSeePlayer(float playerX, float playerY) const {
    // For now, assume boss can always see player if within aggro range
    return distanceToPlayer(playerX, playerY) <= aggroRadius;
}

void Boss::facePlayer(float playerX, float playerY) {
    if (playerX < getX()) {
        currentDirection = EnemyDirection::LEFT;
    } else {
        currentDirection = EnemyDirection::RIGHT;
    }
}