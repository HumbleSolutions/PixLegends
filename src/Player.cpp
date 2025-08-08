#include "Player.h"
#include "Game.h"
#include "Renderer.h"
#include "AssetManager.h"
#include "InputManager.h"
#include "Projectile.h"
#include "Object.h"
#include "World.h"
#include <iostream>
#include <algorithm>
#include <cmath>

Player::Player(Game* game) : game(game), x(Game::WINDOW_WIDTH / 2.0f - 32.0f), y(Game::WINDOW_HEIGHT / 2.0f - 32.0f), width(64), height(64),
                            moveSpeed(DEFAULT_MOVE_SPEED), currentState(PlayerState::IDLE),
                            currentDirection(Direction::DOWN), currentSpriteSheet(nullptr),
                            currentFrame(0), frameTimer(0.0f), frameDuration(FRAME_DURATION),
                            meleeAttackCooldown(MELEE_ATTACK_COOLDOWN), rangedAttackCooldown(RANGED_ATTACK_COOLDOWN),
                            meleeAttackTimer(0.0f), rangedAttackTimer(0.0f), meleeDamage(20), rangedDamage(15),
                            health(BASE_HEALTH), maxHealth(BASE_HEALTH), mana(BASE_MANA), maxMana(BASE_MANA),
                            level(1), experience(0), experienceToNext(100), strength(BASE_STRENGTH), intelligence(BASE_INTELLIGENCE),
                            gold(0),
                            idleLeftSpriteSheet(nullptr), idleRightSpriteSheet(nullptr), walkLeftSpriteSheet(nullptr), walkRightSpriteSheet(nullptr), meleeAttackLeftSpriteSheet(nullptr), meleeAttackRightSpriteSheet(nullptr),
                             rangedAttackLeftSpriteSheet(nullptr), rangedAttackRightSpriteSheet(nullptr), hurtSpriteSheet(nullptr), deathSpriteSheet(nullptr) {
    
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
    hurtSpriteSheet = assetManager->getSpriteSheet(AssetManager::WIZARD_PATH + "HURT.png");
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
}

void Player::move(float deltaTime) {
    if (currentState == PlayerState::ATTACKING_MELEE || currentState == PlayerState::ATTACKING_RANGED) {
        return; // Can't move while attacking
    }
    
    InputManager* inputManager = game->getInputManager();
    float moveX, moveY;
    inputManager->getMovementVector(moveX, moveY);
    
    // Apply movement
    x += moveX * moveSpeed * deltaTime;
    y += moveY * moveSpeed * deltaTime;
    
    // Keep player in bounds (temporary - will be replaced with collision detection)
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x > Game::WINDOW_WIDTH - width) x = static_cast<float>(Game::WINDOW_WIDTH - width);
    if (y > Game::WINDOW_HEIGHT - height) y = static_cast<float>(Game::WINDOW_HEIGHT - height);
}

void Player::performMeleeAttack() {
    if (meleeAttackTimer > 0.0f) {
        return; // Still on cooldown
    }
    
    setState(PlayerState::ATTACKING_MELEE);
    meleeAttackTimer = meleeAttackCooldown;
    currentFrame = 0;
    frameTimer = 0.0f;
    
    std::cout << "Melee attack! Damage: " << meleeDamage << std::endl;
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
        projectileX, projectileY, direction, game->getAssetManager()));
}

void Player::takeDamage(int damage) {
    if (currentState == PlayerState::DEAD) {
        return;
    }
    
    health -= damage;
    if (health <= 0) {
        health = 0;
        setState(PlayerState::DEAD);
        std::cout << "Player died!" << std::endl;
    } else {
        setState(PlayerState::HURT);
        std::cout << "Player took " << damage << " damage! Health: " << health << "/" << maxHealth << std::endl;
    }
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
                        heal(item.amount);
                        notification += "Health Potion";
                        std::cout << "Used health potion! Restored " << item.amount << " health." << std::endl;
                        break;
                    case LootType::MANA_POTION:
                        // Note: useMana is for spending, we need to add mana
                        mana = std::min(mana + item.amount, maxMana);
                        notification += "Mana Potion";
                        std::cout << "Used mana potion! Restored " << item.amount << " mana." << std::endl;
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
            } else if (currentState == PlayerState::HURT) {
                // End hurt animation
                setState(PlayerState::IDLE);
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
            return hurtSpriteSheet;
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
