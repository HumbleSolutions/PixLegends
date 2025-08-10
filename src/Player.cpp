#include "Player.h"
#include "Game.h"
#include "Renderer.h"
#include "AssetManager.h"
#include "AudioManager.h"
#include "InputManager.h"
#include "Projectile.h"
#include "Object.h"
#include "World.h"
#include "Enemy.h"
#include <iostream>
#include <algorithm>
#include <cmath>

Player::Player(Game* game) : game(game), x(Game::WINDOW_WIDTH / 2.0f - 32.0f), y(Game::WINDOW_HEIGHT / 2.0f - 32.0f), width(64), height(64),
                            spawnX(x), spawnY(y),
                            moveSpeed(DEFAULT_MOVE_SPEED), currentState(PlayerState::IDLE),
                            currentDirection(Direction::DOWN), currentSpriteSheet(nullptr),
                            currentFrame(0), frameTimer(0.0f), frameDuration(FRAME_DURATION), deathAnimationFinished(false),
                            meleeAttackCooldown(MELEE_ATTACK_COOLDOWN), rangedAttackCooldown(RANGED_ATTACK_COOLDOWN),
                            meleeAttackTimer(0.0f), rangedAttackTimer(0.0f), meleeDamage(20), rangedDamage(15),
                            health(BASE_HEALTH), maxHealth(BASE_HEALTH), mana(BASE_MANA), maxMana(BASE_MANA),
                            level(1), experience(0), experienceToNext(100), strength(BASE_STRENGTH), intelligence(BASE_INTELLIGENCE),
                            gold(0),
                            healthPotionCharges(POTION_MAX_CHARGES), manaPotionCharges(POTION_MAX_CHARGES),
                            idleLeftSpriteSheet(nullptr), idleRightSpriteSheet(nullptr), walkLeftSpriteSheet(nullptr), walkRightSpriteSheet(nullptr), meleeAttackLeftSpriteSheet(nullptr), meleeAttackRightSpriteSheet(nullptr),
                             rangedAttackLeftSpriteSheet(nullptr), rangedAttackRightSpriteSheet(nullptr), hurtLeftSpriteSheet(nullptr), hurtRightSpriteSheet(nullptr), deathSpriteSheet(nullptr) {
    
    loadSprites();
    calculateExperienceToNext();
    
    // Debug: Print starting position
    int startTileX = static_cast<int>(x / 32);
    int startTileY = static_cast<int>(y / 32);
    std::cout << "Player starting position: pixel(" << x << ", " << y << ") tile(" << startTileX << ", " << startTileY << ")" << std::endl;
    std::cout << "Objects placed at: Chest(10,10), ClayPot(15,12), Flag(20,8), WoodCrate(8,15), SteelCrate(12,18), WoodFence(5,5), BrokenFence(6,5), WoodSign(25,10), Bonfire(18,20)" << std::endl;
}

void Player::loadSprites() {
    AssetManager* assetManager = game->getAssetManager();
    
    // Get preloaded wizard spritesheets
    idleLeftSpriteSheet = assetManager->getSpriteSheet(AssetManager::WIZARD_PATH + "IDLE_LEFT.png");
    idleRightSpriteSheet = assetManager->getSpriteSheet(AssetManager::WIZARD_PATH + "IDLE_RIGHT.png");
    walkLeftSpriteSheet = assetManager->getSpriteSheet(AssetManager::WIZARD_PATH + "WALK_LEFT.png");
    walkRightSpriteSheet = assetManager->getSpriteSheet(AssetManager::WIZARD_PATH + "WALK_RIGHT.png");
    meleeAttackLeftSpriteSheet = assetManager->getSpriteSheet(AssetManager::WIZARD_PATH + "MELEE ATTACK_LEFT.png");
    meleeAttackRightSpriteSheet = assetManager->getSpriteSheet(AssetManager::WIZARD_PATH + "MELEE ATTACK_RIGHT.png");
    rangedAttackLeftSpriteSheet = assetManager->getSpriteSheet(AssetManager::WIZARD_PATH + "RANGED ATTACK_LEFT.png");
    rangedAttackRightSpriteSheet = assetManager->getSpriteSheet(AssetManager::WIZARD_PATH + "RANGED ATTACK_RIGHT.png");
    hurtLeftSpriteSheet = assetManager->getSpriteSheet(AssetManager::WIZARD_PATH + "HURT_LEFT.png");
    hurtRightSpriteSheet = assetManager->getSpriteSheet(AssetManager::WIZARD_PATH + "HURT_RIGHT.png");
    deathSpriteSheet = assetManager->getSpriteSheet(AssetManager::WIZARD_PATH + "DEATH.png");
    
    // Set initial sprite sheet
    currentSpriteSheet = idleLeftSpriteSheet;
}

void Player::update(float deltaTime) {
    // Handle input
    handleInput(game->getInputManager());
    
    // Update movement
    move(deltaTime);
    
    // Update attack cooldowns
    updateAttackCooldowns(deltaTime);
    
    // Update animation
    updateAnimation(deltaTime);

    // Footstep sound while walking
    if (currentState == PlayerState::WALKING && game && game->getAudioManager()) {
        footstepTimer -= deltaTime;
        if (footstepTimer <= 0.0f) {
            game->getAudioManager()->playSound("footstep_dirt");
            footstepTimer = footstepInterval;
        }
    } else {
        // Reset timer when not walking to play promptly when walking resumes
        footstepTimer = 0.0f;
    }

    // Update potion cooldowns
    if (healthPotionCooldown > 0.0f) healthPotionCooldown -= deltaTime;
    if (manaPotionCooldown > 0.0f) manaPotionCooldown -= deltaTime;
    
    // Update projectiles
    updateProjectiles(deltaTime);
}

void Player::handleInput(const InputManager* inputManager) {
    if (!inputManager || currentState == PlayerState::DEAD) {
        return;
    }
    
    // Handle movement input
    float moveX, moveY;
    inputManager->getMovementVector(moveX, moveY);
    
    // Only change direction when there's clear horizontal input
    // This prevents direction changes during rapid key presses
    if (moveX > 0.1f) {
        setDirection(Direction::RIGHT);
    } else if (moveX < -0.1f) {
        setDirection(Direction::LEFT);
    }
    // If moveX is between -0.1 and 0.1, keep the current direction
    
    if (moveX != 0.0f || moveY != 0.0f) {
        if (currentState != PlayerState::ATTACKING_MELEE && currentState != PlayerState::ATTACKING_RANGED) {
            setState(PlayerState::WALKING);
        }
    } else if (currentState == PlayerState::WALKING) {
        setState(PlayerState::IDLE);
    }
    
    // Handle attack input
    if (inputManager->isActionHeld(InputAction::ATTACK_MELEE) && canAttack()) {
        std::cout << "Melee attack triggered!" << std::endl;
        performMeleeAttack();
    }
    
    if (inputManager->isActionHeld(InputAction::ATTACK_RANGED) && canAttack()) {
        std::cout << "Ranged attack triggered!" << std::endl;
        performRangedAttack();
    }
    
    // Handle interaction input
    if (inputManager->isActionPressed(InputAction::INTERACT)) {
        std::cout << "E key pressed - attempting interaction!" << std::endl;
        std::cout << "DEBUG: Player at pixel(" << x << ", " << y << ") tile(" << static_cast<int>(x / 32) << ", " << static_cast<int>(y / 32) << ")" << std::endl;
        interact();
    }
    
    // Debug: Show position when I is pressed
    if (inputManager->isActionPressed(InputAction::INVENTORY)) {
        int currentTileX = static_cast<int>(x / 32);
        int currentTileY = static_cast<int>(y / 32);
        std::cout << "Current position: pixel(" << x << ", " << y << ") tile(" << currentTileX << ", " << currentTileY << ")" << std::endl;
    }

    // Consumables: 1 for HP, 2 for MP
    if (inputManager->isActionPressed(InputAction::USE_HP_POTION)) {
        if (consumeHealthPotion()) {
            std::cout << "Used HP potion. Charges left: " << healthPotionCharges << std::endl;
        } else {
            std::cout << "No HP potion charges left or HP already full." << std::endl;
        }
    }
    if (inputManager->isActionPressed(InputAction::USE_MP_POTION)) {
        if (consumeManaPotion()) {
            std::cout << "Used MP potion. Charges left: " << manaPotionCharges << std::endl;
        } else {
            std::cout << "No MP potion charges left or MP already full." << std::endl;
        }
    }
}

void Player::move(float deltaTime) {
    if (currentState == PlayerState::DEAD || currentState == PlayerState::ATTACKING_MELEE || currentState == PlayerState::ATTACKING_RANGED) {
        return; // Can't move while attacking
    }
    
    InputManager* inputManager = game->getInputManager();
    float moveX, moveY;
    inputManager->getMovementVector(moveX, moveY);
    
    // Apply movement
    float dx = moveX * moveSpeed * deltaTime;
    float dy = moveY * moveSpeed * deltaTime;
    x += dx;
    y += dy;

    // Tile hazards: water and lava
    if (game && game->getWorld()) {
        World* world = game->getWorld();
        int ts = world->getTileSize();
        int tileX = static_cast<int>((x + width * 0.5f) / ts);
        int tileY = static_cast<int>((y + height * 0.9f) / ts); // feet position bias
        int tileId = world->getTile(tileX, tileY);
        if (tileId == TILE_WATER_DEEP) {
            // Drown: continuous heavy damage
            takeDamage(static_cast<int>(std::ceil(40.0f * deltaTime))); // ~40 DPS
        } else if (tileId == TILE_LAVA) {
            // Instant death
            if (!isDead()) takeDamage(health);
        }
    }

    // Footstep SFX when moving
    if ((dx != 0.0f || dy != 0.0f) && game && game->getAudioManager()) {
        footstepTimer -= deltaTime;
        if (footstepTimer <= 0.0f) {
            game->getAudioManager()->playSound("footstep_dirt");
            game->getAudioManager()->startMusicDuck(0.15f, 0.85f);
            footstepTimer = footstepInterval;
        }
    } else {
        footstepTimer = 0.0f;
    }
    
    // Unlimited world roaming: do not clamp to window or finite world size
}

void Player::performMeleeAttack() {
    if (meleeAttackTimer > 0.0f) {
        return; // Still on cooldown
    }

    setState(PlayerState::ATTACKING_MELEE);
    meleeAttackTimer = meleeAttackCooldown;
    currentFrame = 0;
    frameTimer = 0.0f;
    meleeHitConsumedThisSwing = false;
    // Play melee swing SFX at attack start (alternating variants), regardless of hit
    if (game && game->getAudioManager()) {
        static bool altSwing = false;
        game->getAudioManager()->playSound(altSwing ? "player_melee_2" : "player_melee_1");
        altSwing = !altSwing;
        game->getAudioManager()->startMusicDuck(0.25f, 0.6f);
    }
    
    std::cout << "Melee attack! Damage: " << meleeDamage << std::endl;
}

bool Player::isMeleeHitActive() const {
    if (currentState != PlayerState::ATTACKING_MELEE || !currentSpriteSheet) return false;
    // Consider most of the swing as active to ensure responsiveness
    int total = currentSpriteSheet->getTotalFrames();
    if (total <= 0) return false;
    int start = static_cast<int>(total * 0.15f);
    int end = static_cast<int>(total * 0.95f);
    return currentFrame >= start && currentFrame <= end;
}

SDL_Rect Player::getMeleeHitbox() const {
    if (currentState != PlayerState::ATTACKING_MELEE) return SDL_Rect{0,0,0,0};
    // Omnidirectional melee: centered square around player
    const int size = 160; // affects reach in all directions
    SDL_Rect r;
    r.w = size;
    r.h = size;
    int cx = static_cast<int>(x + width / 2.0f);
    int cy = static_cast<int>(y + height / 2.0f);
    r.x = cx - size / 2;
    r.y = cy - size / 2;
    return r;
}

bool Player::consumeMeleeHitIfActive() {
    // Reset when not melee attacking
    if (currentState != PlayerState::ATTACKING_MELEE) {
        meleeHitConsumedThisSwing = false;
        return false;
    }
    if (!isMeleeHitActive()) return false;
    if (meleeHitConsumedThisSwing) return false;
    meleeHitConsumedThisSwing = true;
    return true;
}

void Player::performRangedAttack() {
    if (rangedAttackTimer > 0.0f || mana < 10) {
        return; // Still on cooldown or not enough mana
    }
    
    setState(PlayerState::ATTACKING_RANGED);
    rangedAttackTimer = rangedAttackCooldown;
    useMana(10);
    currentFrame = 0;
    frameTimer = 0.0f;
    
    std::cout << "Ranged attack! Damage: " << rangedDamage << std::endl;
    // Play projectile fire SFX immediately on shoot
    if (game && game->getAudioManager()) {
        game->getAudioManager()->playSound("player_projectile");
        game->getAudioManager()->startMusicDuck(0.2f, 0.7f);
    }
    
    // Create projectile
    float projectileX = x + width / 2.0f; // Center of player
    float projectileY = y + height / 2.0f; // Center of player
    
    // Get mouse position
    InputManager* inputManager = game->getInputManager();
    int mouseX = inputManager->getMouseX();
    int mouseY = inputManager->getMouseY();
    
    // Calculate direction from player to mouse
    float dirX = static_cast<float>(mouseX) - projectileX;
    float dirY = static_cast<float>(mouseY) - projectileY;
    
    // Create normalized direction vector
    ProjectileDirection direction(dirX, dirY);
    direction.normalize();
    
    projectiles.push_back(std::make_unique<Projectile>(
        projectileX, projectileY, direction, game->getAssetManager(), static_cast<int>(rangedDamage)));
}

void Player::takeDamage(int damage) {
    if (currentState == PlayerState::DEAD) {
        return;
    }
    
    health -= damage;
    if (health <= 0) {
        health = 0;
        setState(PlayerState::DEAD);
        deathAnimationFinished = false;
        std::cout << "Player died!" << std::endl;
    } else {
        setState(PlayerState::HURT);
        std::cout << "Player took " << damage << " damage! Health: " << health << "/" << maxHealth << std::endl;
    }
}

void Player::respawn(float respawnX, float respawnY) {
    x = respawnX;
    y = respawnY;
    health = maxHealth;
    mana = maxMana;
    meleeAttackTimer = 0.0f;
    rangedAttackTimer = 0.0f;
    projectiles.clear();
    setDirection(Direction::DOWN);
    setState(PlayerState::IDLE);
}

void Player::interact() {
    if (!game || !game->getWorld()) {
        std::cout << "No game or world available for interaction" << std::endl;
        return;
    }
    
    std::cout << "DEBUG: Checking " << game->getWorld()->getObjects().size() << " objects for interaction" << std::endl;
    
    // Debug: Print all objects and their positions
    int tileSize = game->getWorld()->getTileSize();
    int playerTileX = static_cast<int>(x / tileSize);
    int playerTileY = static_cast<int>(y / tileSize);
    
    std::cout << "DEBUG: Player at pixel(" << x << ", " << y << ") tile(" << playerTileX << ", " << playerTileY << ")" << std::endl;
    
    for (const auto& object : game->getWorld()->getObjects()) {
        std::cout << "DEBUG: Object at tile(" << object->getX() << ", " << object->getY() << ") - interactable: " << (object->isInteractable() ? "yes" : "no") << std::endl;
        if (object->isInteractable()) {
            bool inRange = object->isInInteractionRange(static_cast<int>(x), static_cast<int>(y), tileSize);
            std::cout << "DEBUG: Object at tile(" << object->getX() << ", " << object->getY() << ") - in range: " << (inRange ? "yes" : "no") << std::endl;
        }
    }
    
    Object* nearbyObject = getNearbyInteractableObject();
    if (nearbyObject) {
        std::cout << "Interacting with " << nearbyObject->getInteractionPrompt() << std::endl;
        nearbyObject->interact();
        
        // Collect loot if any
        if (nearbyObject->hasLoot()) {
            std::vector<Loot> loot = nearbyObject->getLoot();
            std::string notification = "Found: ";
            bool firstItem = true;
            
            for (const auto& item : loot) {
                if (!firstItem) notification += ", ";
                firstItem = false;
                
                switch (item.type) {
                    case LootType::GOLD:
                        addGold(item.amount);
                        notification += std::to_string(item.amount) + " Gold";
                        break;
                    case LootType::EXPERIENCE:
                        gainExperience(item.amount);
                        notification += std::to_string(item.amount) + " XP";
                        break;
                    case LootType::HEALTH_POTION:
                        addHealthPotionCharges(item.amount / 10 > 0 ? item.amount / 10 : 1);
                        notification += "HP Potion Charges";
                        std::cout << "Found HP potion charges! +" << (item.amount / 10 > 0 ? item.amount / 10 : 1) << std::endl;
                        break;
                    case LootType::MANA_POTION:
                        addManaPotionCharges(item.amount / 10 > 0 ? item.amount / 10 : 1);
                        notification += "MP Potion Charges";
                        std::cout << "Found MP potion charges! +" << (item.amount / 10 > 0 ? item.amount / 10 : 1) << std::endl;
                        break;
                }
            }
            
            lastLootNotification = notification;
        }
    } else {
        std::cout << "No interactable objects nearby." << std::endl;
    }
}

Object* Player::getNearbyInteractableObject() const {
    if (!game || !game->getWorld()) {
        return nullptr;
    }
    
    int tileSize = game->getWorld()->getTileSize();
    int playerTileX = static_cast<int>(x / tileSize);
    int playerTileY = static_cast<int>(y / tileSize);
    
    // Check all objects in the world
    for (const auto& object : game->getWorld()->getObjects()) {
        if (object->isInteractable()) {
            bool inRange = object->isInInteractionRange(static_cast<int>(x), static_cast<int>(y), tileSize);
            
            if (inRange) {
                return object.get();
            }
        }
    }
    
    return nullptr;
}

std::string Player::getCurrentInteractionPrompt() const {
    Object* nearbyObject = getNearbyInteractableObject();
    if (nearbyObject) {
        return nearbyObject->getInteractionPrompt();
    }
    return "";
}

void Player::updateAnimation(float deltaTime) {
    if (!currentSpriteSheet) {
        return;
    }
    

    
    // Different animation speeds for different states
    float currentFrameDuration = frameDuration;
    switch (currentState) {
        case PlayerState::IDLE:
            currentFrameDuration = 1.0f;  // Much slower idle animation to make it clearly visible
            break;
        case PlayerState::WALKING:
            currentFrameDuration = 0.15f; // Faster walking animation
            break;
        case PlayerState::ATTACKING_MELEE:
        case PlayerState::ATTACKING_RANGED:
            currentFrameDuration = 0.1f;  // Fast attack animation
            break;
        case PlayerState::HURT:
            currentFrameDuration = 0.2f;  // Medium hurt animation
            break;
        case PlayerState::DEAD:
            currentFrameDuration = 0.4f;  // Slow death animation
            break;
        default:
            currentFrameDuration = frameDuration;
            break;
    }
    
    frameTimer += deltaTime;
    
    if (frameTimer >= currentFrameDuration) {
        frameTimer = 0.0f;
        currentFrame++;
        
        // Loop animation or end attack animation
        if (currentFrame >= currentSpriteSheet->getTotalFrames()) {
            if (currentState == PlayerState::ATTACKING_MELEE || 
                currentState == PlayerState::ATTACKING_RANGED) {
                // End attack animation
                setState(PlayerState::IDLE);
                meleeHitConsumedThisSwing = false;
            } else if (currentState == PlayerState::HURT) {
                // End hurt animation
                setState(PlayerState::IDLE);
            } else if (currentState == PlayerState::DEAD) {
                // Hold the last frame and mark finished
                currentFrame = currentSpriteSheet->getTotalFrames() - 1;
                deathAnimationFinished = true;
                return;
            } else {
                // Loop other animations
                currentFrame = 0;
            }
        }
    }
}

void Player::setState(PlayerState newState) {
    if (currentState == newState) {
        return;
    }
    
    currentState = newState;
    currentFrame = 0;
    frameTimer = 0.0f;
    
    // Update sprite sheet
    SpriteSheet* newSpriteSheet = getSpriteSheetForState(newState);
    if (newSpriteSheet && newSpriteSheet != currentSpriteSheet) {
        currentSpriteSheet = newSpriteSheet;
    }
}

void Player::setDirection(Direction newDirection) {
    currentDirection = newDirection;
}

void Player::render(Renderer* renderer) {
    if (!renderer || !currentSpriteSheet) {
        std::cout << "Player render failed: renderer=" << (renderer ? "valid" : "null") 
                  << ", spriteSheet=" << (currentSpriteSheet ? "valid" : "null") << std::endl;
        return;
    }
    
    // Get source rectangle for current frame
    SDL_Rect srcRect = currentSpriteSheet->getFrameRect(currentFrame);
    
    // The sprite sheet has padding on the sides - each frame is 128x78 but the character is 64x78 in the center
    // Adjust the source rectangle to only extract the 64x78 character content from the center
    int frameWidth = currentSpriteSheet->getFrameWidth();  // This should be 128
    int frameHeight = currentSpriteSheet->getFrameHeight(); // This should be 78
    int characterWidth = 64;  // The actual character width
    int characterHeight = 78; // The actual character height
    
    // Calculate padding on the sides
    int paddingLeft = (frameWidth - characterWidth) / 2;  // (128 - 64) / 2 = 32 pixels
    
    // Adjust source rectangle to extract only the character content
    srcRect.x += paddingLeft;  // Start 32 pixels from the left of each frame
    srcRect.w = characterWidth; // Only extract 64 pixels wide
    srcRect.h = characterHeight; // Full height of 78 pixels
    
    // Calculate destination rectangle - render at player position with character size
    // The player's collision box is 64x64, so we need to center the 64x78 sprite vertically
    int playerHeight = 64;  // Player collision box height
    int spriteOffsetY = (characterHeight - playerHeight) / 2;  // (78 - 64) / 2 = 7 pixels
    
    SDL_Rect dstRect = {
        static_cast<int>(x),
        static_cast<int>(y - spriteOffsetY), // Offset upward to center the sprite on the player
        characterWidth,
        characterHeight
    };
    
    // Render the sprite
    renderer->renderTexture(currentSpriteSheet->getTexture()->getTexture(), &srcRect, &dstRect);
}

void Player::gainExperience(int xp) {
    experience += xp;
    std::cout << "Gained " << xp << " experience! Total: " << experience << "/" << experienceToNext << std::endl;
    
    // Check for level up
    while (experience >= experienceToNext) {
        levelUp();
    }
}

void Player::levelUp() {
    level++;
    experience -= experienceToNext;
    
    // Increase stats
    maxHealth += 20;
    maxMana += 10;
    strength += 2;
    intelligence += 3;
    health = maxHealth; // Full heal on level up
    mana = maxMana;
    
    // Recalculate damage
    meleeDamage = strength * 2;
    rangedDamage = intelligence * 1.5f;
    
    calculateExperienceToNext();
    
    std::cout << "Level up! Now level " << level << std::endl;
    std::cout << "Stats - HP: " << maxHealth << " MP: " << maxMana 
              << " STR: " << strength << " INT: " << intelligence << std::endl;
}

void Player::heal(int amount) {
    health = std::min(health + amount, maxHealth);
}

void Player::useMana(int amount) {
    mana = std::max(mana - amount, 0);
}

void Player::addGold(int amount) {
    gold += amount;
    std::cout << "Gained " << amount << " gold! Total gold: " << gold << std::endl;
}

void Player::spendGold(int amount) {
    if (gold >= amount) {
        gold -= amount;
        std::cout << "Spent " << amount << " gold. Remaining gold: " << gold << std::endl;
    } else {
        std::cout << "Not enough gold! Need " << amount << " but only have " << gold << std::endl;
    }
}

bool Player::consumeHealthPotion() {
    if (healthPotionCooldown > 0.0f || healthPotionCharges <= 0 || health >= maxHealth) {
        return false;
    }
    heal(HEALTH_POTION_HEAL);
    healthPotionCharges--;
    healthPotionCooldown = POTION_COOLDOWN_SECONDS;
    if (game && game->getAudioManager()) game->getAudioManager()->playSound("potion_drink");
    return true;
}

bool Player::consumeManaPotion() {
    if (manaPotionCooldown > 0.0f || manaPotionCharges <= 0 || mana >= maxMana) {
        return false;
    }
    mana = std::min(mana + MANA_POTION_RESTORE, maxMana);
    manaPotionCharges--;
    manaPotionCooldown = POTION_COOLDOWN_SECONDS;
    if (game && game->getAudioManager()) game->getAudioManager()->playSound("potion_drink");
    return true;
}

void Player::addHealthPotionCharges(int charges) {
    if (charges <= 0) return;
    healthPotionCharges = std::min(healthPotionCharges + charges, POTION_MAX_CHARGES);
}

void Player::addManaPotionCharges(int charges) {
    if (charges <= 0) return;
    manaPotionCharges = std::min(manaPotionCharges + charges, POTION_MAX_CHARGES);
}

SpriteSheet* Player::getSpriteSheetForState(PlayerState state) {
    switch (state) {
        case PlayerState::IDLE:
            // Return the appropriate idle sprite based on direction
            if (currentDirection == Direction::LEFT) {
                return idleLeftSpriteSheet;
            } else if (currentDirection == Direction::RIGHT) {
                return idleRightSpriteSheet;
            } else {
                // For UP/DOWN directions, use the left idle sprite as default
                return idleLeftSpriteSheet;
            }
        case PlayerState::WALKING:
            // Return the appropriate walking sprite based on direction
            if (currentDirection == Direction::LEFT) {
                return walkLeftSpriteSheet;
            } else if (currentDirection == Direction::RIGHT) {
                return walkRightSpriteSheet;
            } else {
                // For UP/DOWN directions, use the left walking sprite as default
                return walkLeftSpriteSheet;
            }
        case PlayerState::ATTACKING_MELEE:
            // Return the appropriate melee attack sprite based on direction
            if (currentDirection == Direction::LEFT) {
                return meleeAttackLeftSpriteSheet;
            } else if (currentDirection == Direction::RIGHT) {
                return meleeAttackRightSpriteSheet;
            } else {
                // For UP/DOWN directions, use the left melee attack sprite as default
                return meleeAttackLeftSpriteSheet;
            }
        case PlayerState::ATTACKING_RANGED:
            // Return the appropriate ranged attack sprite based on direction
            if (currentDirection == Direction::LEFT) {
                return rangedAttackLeftSpriteSheet;
            } else if (currentDirection == Direction::RIGHT) {
                return rangedAttackRightSpriteSheet;
            } else {
                // For UP/DOWN directions, use the left ranged attack sprite as default
                return rangedAttackLeftSpriteSheet;
            }
        case PlayerState::HURT:
            return currentDirection == Direction::LEFT ? hurtLeftSpriteSheet : hurtRightSpriteSheet;
        case PlayerState::DEAD:
            return deathSpriteSheet;
        default:
            return idleLeftSpriteSheet;
    }
}

void Player::updateAttackCooldowns(float deltaTime) {
    if (meleeAttackTimer > 0.0f) {
        meleeAttackTimer -= deltaTime;
    }
    
    if (rangedAttackTimer > 0.0f) {
        rangedAttackTimer -= deltaTime;
    }
}

bool Player::canAttack() const {
    return currentState != PlayerState::DEAD && 
           meleeAttackTimer <= 0.0f && 
           rangedAttackTimer <= 0.0f;
}

void Player::calculateExperienceToNext() {
    experienceToNext = level * 100; // Simple formula: level * 100
}

void Player::updateProjectiles(float deltaTime) {
    // Update all projectiles
    for (auto& projectile : projectiles) {
        projectile->update(deltaTime);
    }
    
    // Remove inactive projectiles
    projectiles.erase(
        std::remove_if(projectiles.begin(), projectiles.end(),
            [](const std::unique_ptr<Projectile>& p) { return !p->isActive(); }),
        projectiles.end()
    );
}

void Player::renderProjectiles(Renderer* renderer) {
    for (auto& projectile : projectiles) {
        projectile->render(renderer);
    }
}
