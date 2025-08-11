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
#include "Database.h"
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
                            idleLeftSpriteSheet(nullptr), idleRightSpriteSheet(nullptr), idleUpSpriteSheet(nullptr), idleDownSpriteSheet(nullptr),
                            walkLeftSpriteSheet(nullptr), walkRightSpriteSheet(nullptr), walkUpSpriteSheet(nullptr), walkDownSpriteSheet(nullptr),
                            meleeAttackLeftSpriteSheet(nullptr), meleeAttackRightSpriteSheet(nullptr), meleeAttackUpSpriteSheet(nullptr), meleeAttackDownSpriteSheet(nullptr),
                            rangedAttackLeftSpriteSheet(nullptr), rangedAttackRightSpriteSheet(nullptr), rangedAttackUpSpriteSheet(nullptr), rangedAttackDownSpriteSheet(nullptr),
                            hurtLeftSpriteSheet(nullptr), hurtRightSpriteSheet(nullptr), deathSpriteSheet(nullptr) {
    
    loadSprites();
    calculateExperienceToNext();
    
    // Debug: Print starting position
    int startTileX = static_cast<int>(x / 32);
    int startTileY = static_cast<int>(y / 32);
    std::cout << "Player starting position: pixel(" << x << ", " << y << ") tile(" << startTileX << ", " << startTileY << ")" << std::endl;
    std::cout << "Objects placed at: Chest(10,10), ClayPot(15,12), Flag(20,8), WoodCrate(8,15), SteelCrate(12,18), WoodFence(5,5), BrokenFence(6,5), WoodSign(25,10), Bonfire(18,20)" << std::endl;

    // Initialize baseline equipment
    auto initEquip = [&](EquipmentSlot slot, const char* name, int basePower){
        size_t idx = static_cast<size_t>(slot);
        equipment[idx].name = name;
        equipment[idx].basePower = basePower;
        equipment[idx].plusLevel = 0;
    };
    initEquip(EquipmentSlot::RING,     "Simple Ring", 1);
    initEquip(EquipmentSlot::HELM,     "Leather Helm", 2);
    initEquip(EquipmentSlot::NECKLACE, "Old Necklace", 1);
    initEquip(EquipmentSlot::SWORD,    "Rusty Sword", 3);
    initEquip(EquipmentSlot::CHEST,    "Leather Armor", 4);
    initEquip(EquipmentSlot::SHIELD,   "Wooden Shield", 2);
    initEquip(EquipmentSlot::GLOVE,    "Cloth Gloves", 1);
    initEquip(EquipmentSlot::WAIST,    "Rope Belt", 1);
    initEquip(EquipmentSlot::FEET,     "Worn Boots", 2);
    // Testing: give 6 of each scroll type by default (upgrade + elements)
    addUpgradeScrolls(6);
    addElementScrolls("fire", 6);
    addElementScrolls("water", 6);
    addElementScrolls("poison", 6);
}

void Player::loadSprites() {
    AssetManager* assetManager = game->getAssetManager();
    
    // Load new main character directional 8-frame sprite sheets
    idleLeftSpriteSheet  = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "IDLE/idle_left.png");
    idleRightSpriteSheet = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "IDLE/idle_right.png");
    idleUpSpriteSheet    = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "IDLE/idle_up.png");
    idleDownSpriteSheet  = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "IDLE/idle_down.png");

    walkLeftSpriteSheet  = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "RUN/run_left.png");
    walkRightSpriteSheet = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "RUN/run_right.png");
    walkUpSpriteSheet    = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "RUN/run_up.png");
    walkDownSpriteSheet  = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "RUN/run_down.png");

    meleeAttackLeftSpriteSheet  = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "ATTACK 1/attack1_left.png");
    meleeAttackRightSpriteSheet = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "ATTACK 1/attack1_right.png");
    meleeAttackUpSpriteSheet    = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "ATTACK 1/attack1_up.png");
    meleeAttackDownSpriteSheet  = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "ATTACK 1/attack1_down.png");

    rangedAttackLeftSpriteSheet  = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "ATTACK 2/attack2_left.png");
    rangedAttackRightSpriteSheet = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "ATTACK 2/attack2_right.png");
    rangedAttackUpSpriteSheet    = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "ATTACK 2/attack2_up.png");
    rangedAttackDownSpriteSheet  = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "ATTACK 2/attack2_down.png");

    // No hurt/death sheets for the new set; fall back to right/left idle to keep flow consistent
    hurtLeftSpriteSheet  = idleLeftSpriteSheet;
    hurtRightSpriteSheet = idleRightSpriteSheet;
    deathSpriteSheet     = idleDownSpriteSheet; // placeholder single loop
    
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
    
    // Update animation — ensure attack sheets have priority over movement
    // If attacking, keep current attack spritesheet locked
    if (currentState == PlayerState::ATTACKING_MELEE || currentState == PlayerState::ATTACKING_RANGED) {
        if (!currentSpriteSheet) currentSpriteSheet = getSpriteSheetForState(currentState);
    }
    updateAnimation(deltaTime);

    // Footstep sound while walking
    if (currentState == PlayerState::WALKING && game && game->getAudioManager() && game->getWorld()) {
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
    
    // Update facing directly from most recent input, favoring last non-zero axis to avoid sticky directions
    if (currentState != PlayerState::ATTACKING_MELEE && currentState != PlayerState::ATTACKING_RANGED && (std::fabs(moveX) >= 0.1f || std::fabs(moveY) >= 0.1f)) {
        if (std::fabs(moveX) >= std::fabs(moveY)) {
            if (moveX > 0.1f) setDirection(Direction::RIGHT);
            else if (moveX < -0.1f) setDirection(Direction::LEFT);
        } else {
            if (moveY > 0.1f) setDirection(Direction::DOWN);
            else if (moveY < -0.1f) setDirection(Direction::UP);
        }
    }
    
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
    
    if (inputManager->isActionHeld(InputAction::ATTACK_RANGED) && canAttack() && hasFireWeapon()) {
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
    if (currentState == PlayerState::DEAD) {
        return; // Can't move when dead
    }
    
    InputManager* inputManager = game->getInputManager();
    float moveX, moveY;
    inputManager->getMovementVector(moveX, moveY);
    
    // Apply movement with attack slow
    float speedFactor = 1.0f;
    if (currentState == PlayerState::ATTACKING_MELEE || currentState == PlayerState::ATTACKING_RANGED) {
        speedFactor = 0.65f; // slight slow while attacking (was 0.5)
    }
    float dx = moveX * moveSpeed * speedFactor * deltaTime;
    float dy = moveY * moveSpeed * speedFactor * deltaTime;
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

    // Footstep SFX only during walking state (avoid during attacks)
    if (currentState == PlayerState::WALKING && (dx != 0.0f || dy != 0.0f) && game && game->getAudioManager() && game->getWorld()) {
        footstepTimer -= deltaTime;
        if (footstepTimer <= 0.0f) {
            game->getAudioManager()->playSound("footstep_dirt");
            game->getAudioManager()->startMusicDuck(0.15f, 0.85f);
            footstepTimer = footstepInterval;
        }
    } else if (currentState != PlayerState::WALKING) {
        footstepTimer = 0.0f;
    }
    
    // Unlimited world roaming: do not clamp to window or finite world size
}

void Player::performMeleeAttack() {
    if (meleeAttackTimer > 0.0f) {
        return; // Still on cooldown
    }

    // Enter attack state; the animation system will pick the proper sheet per direction
    setState(PlayerState::ATTACKING_MELEE);
    meleeAttackTimer = meleeAttackCooldown;
    currentFrame = 0;
    frameTimer = 0.0f;
    meleeHitConsumedThisSwing = false;
    // Ensure attack animation uses the correct sheet for current direction
    currentSpriteSheet = getSpriteSheetForState(PlayerState::ATTACKING_MELEE);
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
    // Omnidirectional melee: centered square around player (reduced by half)
    const int size = 80; // affects reach in all directions
    SDL_Rect r;
    r.w = size;
    r.h = size;
    int cx = static_cast<int>(x + width / 2.0f);
    int cy = static_cast<int>(y + height / 2.0f);
    r.x = cx - size / 2;
    r.y = cy - size / 2;
    return r;
}

SDL_Rect Player::getCollisionRect() const {
    // Focus on the lower portion (feet/body) roughly matching visual stance
    int bodyWidth = static_cast<int>(width * 0.6f);
    int bodyHeight = static_cast<int>(height * 0.55f);
    int offsetX = (width - bodyWidth) / 2;
    int offsetY = static_cast<int>(height * 0.45f);
    SDL_Rect r;
    r.x = static_cast<int>(x) + offsetX;
    r.y = static_cast<int>(y) + offsetY;
    r.w = bodyWidth;
    r.h = bodyHeight;
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
    
    // Create projectile from player center (world coords)
    float projectileX = x + width / 2.0f;
    float projectileY = y + height / 2.0f;

    // Get mouse position (screen), convert to world using renderer camera and zoom
    InputManager* inputManager = game->getInputManager();
    int mouseScreenX = inputManager->getMouseX();
    int mouseScreenY = inputManager->getMouseY();
    int mouseWorldX = mouseScreenX;
    int mouseWorldY = mouseScreenY;
    if (auto* renderer = game->getRenderer()) {
        renderer->screenToWorld(mouseScreenX, mouseScreenY, mouseWorldX, mouseWorldY);
    }
    
    // Calculate direction from player to mouse (world space)
    float dirX = static_cast<float>(mouseWorldX) - projectileX;
    float dirY = static_cast<float>(mouseWorldY) - projectileY;
    
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
            // Rect-intersection check using player's collision rect for robustness
            SDL_Rect playerRect = getCollisionRect();
            SDL_Rect objRect{ object->getX() * tileSize, object->getY() * tileSize, tileSize, tileSize };
            // Slightly expand object rect to make interaction forgiving
            objRect.x -= 6; objRect.y -= 6; objRect.w += 12; objRect.h += 12;
            bool inter = SDL_HasIntersection(&playerRect, &objRect);
            std::cout << "DEBUG: intersection-based in range: " << (inter ? "yes" : "no") << std::endl;
        }
    }
    
    Object* nearbyObject = getNearbyInteractableObject();
    if (nearbyObject) {
        std::cout << "Interacting with " << nearbyObject->getInteractionPrompt() << std::endl;
        // Special-case Magic Anvil: open modal UI
        if (game && nearbyObject->getType() == ObjectType::MAGIC_ANVIL) {
            game->openAnvil();
        } else {
            nearbyObject->interact();
        }
        
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
    int playerTileX = static_cast<int>((x + width * 0.5f) / tileSize);
    int playerTileY = static_cast<int>((y + height * 0.9f) / tileSize);
    
    // Check all objects in the world
    for (const auto& object : game->getWorld()->getObjects()) {
        if (!object->isInteractable()) continue;
        // Primary check: player's collision rect vs expanded object tile rect
        SDL_Rect playerRect = getCollisionRect();
        SDL_Rect objRect{ object->getX() * tileSize, object->getY() * tileSize, tileSize, tileSize };
        objRect.x -= 6; objRect.y -= 6; objRect.w += 12; objRect.h += 12;
        if (SDL_HasIntersection(&playerRect, &objRect)) {
            return object.get();
        }

        // Fallback check: distance from player's feet center to object center
        int playerFeetX = static_cast<int>(x + width * 0.5f);
        int playerFeetY = static_cast<int>(y + height * 0.9f);
        int objCenterX = object->getX() * tileSize + tileSize / 2;
        int objCenterY = object->getY() * tileSize + tileSize / 2;
        int dx = playerFeetX - objCenterX;
        int dy = playerFeetY - objCenterY;
        int distSq = dx * dx + dy * dy;
        int maxDist = static_cast<int>(tileSize * 0.9f); // just under one tile radius
        if (distSq <= maxDist * maxDist) {
            return object.get();
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
    // No mid-swing extra SFX
        
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

    // Update sprite sheet with attack priority
    if (newState == PlayerState::ATTACKING_MELEE || newState == PlayerState::ATTACKING_RANGED) {
        SpriteSheet* locked = getSpriteSheetForState(newState);
        if (locked) currentSpriteSheet = locked;
    } else {
        SpriteSheet* newSpriteSheet = getSpriteSheetForState(newState);
        if (newSpriteSheet && newSpriteSheet != currentSpriteSheet) {
            currentSpriteSheet = newSpriteSheet;
        }
    }
}

void Player::setDirection(Direction newDirection) {
    // Ignore direction changes during attack animations to prevent visual double-swing glitches
    if (currentState == PlayerState::ATTACKING_MELEE || currentState == PlayerState::ATTACKING_RANGED) {
        return;
    }
    if (currentDirection == newDirection) return;
    currentDirection = newDirection;
    // Refresh sprite sheet immediately for non-attack states so visuals update promptly
    SpriteSheet* newSheet = getSpriteSheetForState(currentState);
    if (newSheet && newSheet != currentSpriteSheet) {
        currentSpriteSheet = newSheet;
        currentFrame = 0;
        frameTimer = 0.0f;
    }
}

void Player::render(Renderer* renderer) {
    if (!renderer || !currentSpriteSheet) {
        std::cout << "Player render failed: renderer=" << (renderer ? "valid" : "null") 
                  << ", spriteSheet=" << (currentSpriteSheet ? "valid" : "null") << std::endl;
        return;
    }
    
    // Get source rectangle for current frame
    SDL_Rect srcRect = currentSpriteSheet->getFrameRect(currentFrame);
    
    // Generic render: use frame size directly; scale to 64px width while keeping aspect
    int frameWidth = currentSpriteSheet->getFrameWidth();
    int frameHeight = currentSpriteSheet->getFrameHeight();

    int dstW = static_cast<int>(64 * renderScale); // upscale visual width
    int dstH = static_cast<int>(std::round(dstW * (static_cast<float>(frameHeight) / std::max(1, frameWidth))));
    int spriteOffsetY = std::max(0, (dstH - height) / 2);

    int camX = 0, camY = 0; renderer->getCamera(camX, camY);
    float z = renderer->getZoom();
    auto scaledEdge = [camX, camY, z](int wx, int wy) -> SDL_Point {
        float sx = (static_cast<float>(wx - camX)) * z;
        float sy = (static_cast<float>(wy - camY)) * z;
        return SDL_Point{ static_cast<int>(std::floor(sx)), static_cast<int>(std::floor(sy)) };
    };
    SDL_Point tl = scaledEdge(static_cast<int>(x), static_cast<int>(y - spriteOffsetY));
    SDL_Point br = scaledEdge(static_cast<int>(x + dstW), static_cast<int>(y - spriteOffsetY + dstH));
    SDL_Rect dstRect = { tl.x, tl.y, std::max(1, br.x - tl.x), std::max(1, br.y - tl.y) };
    
    // Render the sprite
    SDL_RenderCopy(renderer->getSDLRenderer(), currentSpriteSheet->getTexture()->getTexture(), &srcRect, &dstRect);
}

void Player::gainExperience(int xp) {
    experience += xp;
    std::cout << "Gained " << xp << " experience! Total: " << experience << "/" << experienceToNext << std::endl;
    
    // Check for level up
    while (experience >= experienceToNext) {
        levelUp();
    }
}

// --- Equipment upgrades and enchants ---
void Player::upgradeEquipment(EquipmentSlot slot, int deltaPlus) {
    size_t idx = static_cast<size_t>(slot);
    int before = equipment[idx].plusLevel;
    equipment[idx].plusLevel = std::max(0, before + deltaPlus);
    // Scale base stats per +: +1 STR/INT, +5 HP, +3 MP per plus across the board
    strength += std::max(0, deltaPlus);
    intelligence += std::max(0, deltaPlus);
    maxHealth += 5 * std::max(0, deltaPlus);
    maxMana += 3 * std::max(0, deltaPlus);
    health = std::min(health, maxHealth);
    mana = std::min(mana, maxMana);
    // Recalculate derived damage: sword + adds 2 dmg per plus level
    int weaponBonus = equipment[static_cast<size_t>(EquipmentSlot::SWORD)].plusLevel * 2;
    meleeDamage = strength * 2 + weaponBonus;
}

void Player::enchantEquipment(EquipmentSlot slot, const std::string& element, int amount) {
    size_t idx = static_cast<size_t>(slot);
    auto setElem = [&](int& ref){ ref = std::max(0, ref + amount); };
    if (element == "fire") setElem(equipment[idx].fire);
    else if (element == "ice" || element == "water") setElem(equipment[idx].ice);
    else if (element == "lightning") setElem(equipment[idx].lightning);
    else if (element == "poison") setElem(equipment[idx].poison);
    else if (element == "resist_fire") setElem(equipment[idx].resistFire);
    else if (element == "resist_ice") setElem(equipment[idx].resistIce);
    else if (element == "resist_lightning") setElem(equipment[idx].resistLightning);
    else if (element == "resist_poison") setElem(equipment[idx].resistPoison);
}

int Player::getElementScrolls(const std::string& element) const {
    auto it = elementScrolls.find(element);
    if (it == elementScrolls.end()) return 0;
    return it->second;
}

void Player::addElementScrolls(const std::string& element, int count) {
    if (count <= 0) return;
    elementScrolls[element] += count;
    // Also add to inventory bag 0 as a stackable item
    addItemToInventory(element + "_scroll", count);
}

bool Player::consumeUpgradeScroll() {
    if (upgradeScrolls <= 0) return false;
    upgradeScrolls--; return true;
}

bool Player::consumeElementScroll(const std::string& element) {
    auto it = elementScrolls.find(element);
    if (it == elementScrolls.end() || it->second <= 0) return false;
    it->second -= 1; return true;
}

void Player::addItemToInventory(const std::string& key, int amount) {
    if (amount <= 0) return;
    // Prefer bag 0; if key not present use bag 0 else keep stacking where present
    for (int i = 0; i < 2; ++i) {
        auto it = bags[i].find(key);
        if (it != bags[i].end()) { it->second += amount; return; }
    }
    bags[0][key] += amount;
    // Keep the special counters in sync for canonical keys
    if (key == "upgrade_scroll") {
        upgradeScrolls += amount;
    } else if (key == "fire_scroll" || key == "fire") {
        elementScrolls["fire"] += amount;
    } else if (key == "water_scroll" || key == "water") {
        elementScrolls["water"] += amount;
    } else if (key == "poison_scroll" || key == "poison") {
        elementScrolls["poison"] += amount;
    }
}

int Player::getInventoryCount(const std::string& key) const {
    int total = 0; for (int i = 0; i < 2; ++i) { auto it = bags[i].find(key); if (it != bags[i].end()) total += it->second; }
    return total;
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

void Player::applySaveState(const PlayerSave& s) {
    // Position and spawn
    x = s.x; y = s.y; spawnX = s.spawnX; spawnY = s.spawnY;
    // Stats
    level = s.level; experience = s.experience; calculateExperienceToNext();
    maxHealth = s.maxHealth; health = std::min(s.health, maxHealth);
    maxMana = s.maxMana; mana = std::min(s.mana, maxMana);
    strength = s.strength; intelligence = s.intelligence;
    gold = s.gold;
    // Consumables
    healthPotionCharges = std::clamp(s.healthPotionCharges, 0, POTION_MAX_CHARGES);
    manaPotionCharges = std::clamp(s.manaPotionCharges, 0, POTION_MAX_CHARGES);
    // Upgrades (will be recomputed from bags below to stay in sync)
    upgradeScrolls = std::max(0, s.upgradeScrolls);
    elementScrolls.clear();
    // Apply saved +levels and elemental mods to equipment
    for (int i = 0; i < 9; ++i) {
        equipment[i].plusLevel = std::max(0, s.equipPlus[i]);
        equipment[i].fire = std::max(0, s.equipFire[i]);
        equipment[i].ice = std::max(0, s.equipIce[i]);
        equipment[i].lightning = std::max(0, s.equipLightning[i]);
        equipment[i].poison = std::max(0, s.equipPoison[i]);
    }
    // Recompute melee damage with sword +
    int weaponBonus = equipment[static_cast<size_t>(EquipmentSlot::SWORD)].plusLevel * 2;
    meleeDamage = strength * 2 + weaponBonus;
    // Inventory persistence (exact grid)
    for (int b = 0; b < 2; ++b) {
        bags[b].clear();
        for (int i = 0; i < 9; ++i) {
            const std::string& key = s.invKey[b][i];
            int cnt = s.invCnt[b][i];
            if (!key.empty() && cnt > 0) bags[b][key] += cnt;
        }
    }
    // Recompute scroll counters from current bag contents so UI and consumption align
    upgradeScrolls = 0;
    elementScrolls.clear();
    for (int b = 0; b < 2; ++b) {
        for (const auto& kv : bags[b]) {
            const std::string& k = kv.first; int c = kv.second;
            if (k == "upgrade_scroll") upgradeScrolls += c;
            else if (k == "fire_scroll" || k == "fire") elementScrolls["fire"] += c;
            else if (k == "water_scroll" || k == "water") elementScrolls["water"] += c;
            else if (k == "poison_scroll" || k == "poison") elementScrolls["poison"] += c;
        }
    }
}

PlayerSave Player::makeSaveState() const {
    PlayerSave s;
    s.x = x; s.y = y; s.spawnX = spawnX; s.spawnY = spawnY;
    s.level = level; s.experience = experience;
    s.maxHealth = maxHealth; s.health = health;
    s.maxMana = maxMana; s.mana = mana;
    s.strength = strength; s.intelligence = intelligence;
    s.gold = gold;
    s.healthPotionCharges = healthPotionCharges; s.manaPotionCharges = manaPotionCharges;
    s.upgradeScrolls = upgradeScrolls;
    // Persist equipment +levels and element mods
    for (int i = 0; i < 9; ++i) {
        s.equipPlus[i] = equipment[i].plusLevel;
        s.equipFire[i] = equipment[i].fire;
        s.equipIce[i] = equipment[i].ice;
        s.equipLightning[i] = equipment[i].lightning;
        s.equipPoison[i] = equipment[i].poison;
    }
    // Inventory persistence (exact grid snapshot - distribute first 9 items per bag)
    for (int b = 0; b < 2; ++b) {
        int idx = 0;
        for (const auto& kv : bags[b]) {
            if (idx >= 9) break;
            s.invKey[b][idx] = kv.first;
            s.invCnt[b][idx] = kv.second;
            idx++;
        }
        for (; idx < 9; ++idx) { s.invKey[b][idx].clear(); s.invCnt[b][idx] = 0; }
    }
    return s;
}

bool Player::consumeHealthPotion() {
    bool infinite = game && game->getInfinitePotions();
    if (healthPotionCooldown > 0.0f || (!infinite && healthPotionCharges <= 0) || health >= maxHealth) {
        return false;
    }
    heal(HEALTH_POTION_HEAL);
    if (!infinite) {
        healthPotionCharges--;
    }
    healthPotionCooldown = POTION_COOLDOWN_SECONDS;
    if (game && game->getAudioManager()) game->getAudioManager()->playSound("potion_drink");
    return true;
}

bool Player::consumeManaPotion() {
    bool infinite = game && game->getInfinitePotions();
    if (manaPotionCooldown > 0.0f || (!infinite && manaPotionCharges <= 0) || mana >= maxMana) {
        return false;
    }
    mana = std::min(mana + MANA_POTION_RESTORE, maxMana);
    if (!infinite) {
        manaPotionCharges--;
    }
    manaPotionCooldown = POTION_COOLDOWN_SECONDS;
    if (game && game->getAudioManager()) game->getAudioManager()->playSound("potion_drink");
    return true;
}

int Player::getEffectiveHealthPotionCharges() const {
    if (game && game->getInfinitePotions()) return POTION_MAX_CHARGES;
    return healthPotionCharges;
}

int Player::getEffectiveManaPotionCharges() const {
    if (game && game->getInfinitePotions()) return POTION_MAX_CHARGES;
    return manaPotionCharges;
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
            if (currentDirection == Direction::LEFT) return idleLeftSpriteSheet;
            if (currentDirection == Direction::RIGHT) return idleRightSpriteSheet;
            if (currentDirection == Direction::UP) return idleUpSpriteSheet;
            return idleDownSpriteSheet;
        case PlayerState::WALKING:
            if (currentDirection == Direction::LEFT) return walkLeftSpriteSheet;
            if (currentDirection == Direction::RIGHT) return walkRightSpriteSheet;
            if (currentDirection == Direction::UP) return walkUpSpriteSheet;
            return walkDownSpriteSheet;
        case PlayerState::ATTACKING_MELEE:
            if (currentDirection == Direction::LEFT) return meleeAttackLeftSpriteSheet;
            if (currentDirection == Direction::RIGHT) return meleeAttackRightSpriteSheet;
            if (currentDirection == Direction::UP) return meleeAttackUpSpriteSheet;
            return meleeAttackDownSpriteSheet;
        case PlayerState::ATTACKING_RANGED:
            if (currentDirection == Direction::LEFT) return rangedAttackLeftSpriteSheet;
            if (currentDirection == Direction::RIGHT) return rangedAttackRightSpriteSheet;
            if (currentDirection == Direction::UP) return rangedAttackUpSpriteSheet;
            return rangedAttackDownSpriteSheet;
        case PlayerState::HURT:
            if (currentDirection == Direction::LEFT) return hurtLeftSpriteSheet;
            if (currentDirection == Direction::RIGHT) return hurtRightSpriteSheet;
            return (currentDirection == Direction::UP ? idleUpSpriteSheet : idleDownSpriteSheet);
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
