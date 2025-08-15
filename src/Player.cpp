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
#include "DatabaseSQLite.h"
#include "ItemSystem.h"
#include "SpellSystem.h"
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
                             hurtLeftSpriteSheet(nullptr), hurtRightSpriteSheet(nullptr), hurtUpSpriteSheet(nullptr), hurtDownSpriteSheet(nullptr),
                             deathLeftSpriteSheet(nullptr), deathRightSpriteSheet(nullptr), deathUpSpriteSheet(nullptr), deathDownSpriteSheet(nullptr) {
    
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
    initEquip(EquipmentSlot::SHIELD,   "Recurve Bow", 2);
    initEquip(EquipmentSlot::GLOVE,    "Cloth Gloves", 1);
    initEquip(EquipmentSlot::WAIST,    "Rope Belt", 1);
    initEquip(EquipmentSlot::FEET,     "Worn Boots", 2);
    // Initialize enhanced item system
    if (game && game->getAssetManager()) {
        itemSystem = std::make_unique<ItemSystem>(game->getAssetManager());
        
        // Initialize spell system
        spellSystem = std::make_unique<SpellSystem>(game, this);
        // Give player starter equipment and auto-equip it
        const char* starterItems[9] = {
            "copper_ring",      // RING - slot 0
            "cloth_cap",        // HELM - slot 1  
            "string_necklace",  // NECKLACE - slot 2
            "rusty_sword",      // SWORD - slot 3
            "torn_shirt",       // CHEST - slot 4
            "wooden_shield",    // SHIELD - slot 5
            "fabric_gloves",    // GLOVE - slot 6
            "frayed_rope",      // WAIST - slot 7
            "canvas_shoes"      // FEET - slot 8
        };
        
        // Create and equip starter items directly
        for (int i = 0; i < 9; i++) {
            Item* starterItem = itemSystem->createItem(starterItems[i], ItemRarity::COMMON, 1);
            if (starterItem) {
                // Give the sword a +1 to test bracket display
                if (i == 3) { // SWORD slot
                    starterItem->plusLevel = 1;
                }
                
                // Add to inventory slot i first
                if (itemSystem->addItemToSlot(starterItem, i, false)) {
                    // Then equip from that slot
                    itemSystem->equipItem(i);
                }
            }
        }
        
        // Add some basic scrolls for upgrades
        itemSystem->addItem("upgrade_scroll", 3, ItemRarity::COMMON);
        itemSystem->addItem("fire_scroll", 2, ItemRarity::COMMON);
    }
    
    // Testing: give 6 of each scroll type by default (upgrade + elements)
    // Ensure sword name and stats reflect current + level mapping
    updateSwordNameByPlus();
    updateSwordStatsByPlus();
    addUpgradeScrolls(6);
    addElementScrolls("fire", 6);
    addElementScrolls("water", 6);
    addElementScrolls("poison", 6);
}

void Player::loadSprites() {
    AssetManager* assetManager = game->getAssetManager();
    
    // Load new main character directional sprite sheets (Idle/Run 6 frames @128, Hurt 6, Death 12)
    idleLeftSpriteSheet  = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "IDLE/Sword_Idle_Left.png");
    idleRightSpriteSheet = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "IDLE/Sword_Idle_Right.png");
    idleUpSpriteSheet    = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "IDLE/Sword_Idle_Up.png");
    idleDownSpriteSheet  = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "IDLE/Sword_Idle_Down.png");

    walkLeftSpriteSheet  = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "RUN/Sword_Run_Left.png");
    walkRightSpriteSheet = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "RUN/Sword_Run_Right.png");
    walkUpSpriteSheet    = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "RUN/Sword_Run_Up.png");
    walkDownSpriteSheet  = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "RUN/Sword_Run_Down.png");

    meleeAttackLeftSpriteSheet  = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "SWORD ATTACK 1/Sword_Attack_1_Left.png");
    meleeAttackRightSpriteSheet = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "SWORD ATTACK 1/Sword_Attack_1_Right.png");
    meleeAttackUpSpriteSheet    = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "SWORD ATTACK 1/Sword_Attack_1_Up.png");
    meleeAttackDownSpriteSheet  = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "SWORD ATTACK 1/Sword_Attack_1_Down.png");

    // Do not load Sword_Attack_2 yet; reserve for future ability
    rangedAttackLeftSpriteSheet  = nullptr;
    rangedAttackRightSpriteSheet = nullptr;
    rangedAttackUpSpriteSheet    = nullptr;
    rangedAttackDownSpriteSheet  = nullptr;

    // Attack end sheets (sword 4 frames, bow end may be 2 frames but reused via ATTACK_END logic)
    attackEndLeftSpriteSheet  = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "END ANIMATION/Sword_Attack_End_Left.png");
    attackEndRightSpriteSheet = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "END ANIMATION/Sword_Attack_End_Right.png");
    attackEndUpSpriteSheet    = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "END ANIMATION/Sword_Attack_End_Up.png");
    attackEndDownSpriteSheet  = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "END ANIMATION/Sword_Attack_End_Down.png");
    // Bow idle/run
    bowIdleLeftSpriteSheet  = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "IDLE/Bow_Idle_Left.png");
    bowIdleRightSpriteSheet = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "IDLE/Bow_Idle_Right.png");
    bowIdleUpSpriteSheet    = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "IDLE/Bow_Idle_Up.png");
    bowIdleDownSpriteSheet  = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "IDLE/Bow_Idle_Down.png");
    bowRunLeftSpriteSheet   = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "RUN/Bow_Run_Left.png");
    bowRunRightSpriteSheet  = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "RUN/Bow_Run_Right.png");
    bowRunUpSpriteSheet     = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "RUN/Bow_Run_Up.png");
    bowRunDownSpriteSheet   = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "RUN/Bow_Run_Down.png");
    // Bow attack
    bowAttackLeftSpriteSheet  = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "BOW ATTACK 1/Bow_Attack_1_Left.png");
    bowAttackRightSpriteSheet = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "BOW ATTACK 1/Bow_Attack_1_Right.png");
    bowAttackUpSpriteSheet    = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "BOW ATTACK 1/Bow_Attack_1_Up.png");
    bowAttackDownSpriteSheet  = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "BOW ATTACK 1/Bow_Attack_1_Down.png");
    // Bow end
    bowEndLeftSpriteSheet  = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "BOW ATTACK 1/Bow_Attack_End_Left.png");
    bowEndRightSpriteSheet = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "BOW ATTACK 1/Bow_Attack_End_Right.png");
    bowEndUpSpriteSheet    = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "BOW ATTACK 1/Bow_Attack_End_Up.png");
    bowEndDownSpriteSheet  = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "BOW ATTACK 1/Bow_Attack_End_Down.png");
    // Dash sheets (8 frames)
    dashLeftSpriteSheet  = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "DASH/Dash_Left.png");
    dashRightSpriteSheet = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "DASH/Dash_Right.png");
    dashUpSpriteSheet    = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "DASH/Dash_Up.png");
    dashDownSpriteSheet  = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "DASH/Dash_Down.png");

    // Hurt (take damage) sheets: 6 frames each
    // Hurt sheets: load hurt_* files directly (Damage_*.png files don't exist)
    hurtLeftSpriteSheet  = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "TAKE_DMG/hurt_left.png");
    hurtRightSpriteSheet = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "TAKE_DMG/hurt_right.png");
    hurtUpSpriteSheet    = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "TAKE_DMG/hurt_up.png");
    hurtDownSpriteSheet  = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "TAKE_DMG/hurt_down.png");
    // Death sheets: 12 frames each, choose by direction during DEAD state
    deathLeftSpriteSheet  = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "DEATH/Death_Left.png");
    deathRightSpriteSheet = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "DEATH/Death_Right.png");
    deathUpSpriteSheet    = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "DEATH/Death_Up.png");
    deathDownSpriteSheet  = assetManager->getSpriteSheet(AssetManager::MAIN_CHAR_PATH + "DEATH/Death_Down.png");
    // Fire shield sheet: 8 frames, 48x48; use auto loader and treat as looping overlay
    fireShieldSpriteSheet = assetManager->loadSpriteSheetAuto("assets/Textures/Spells/fire_sheild.png", 8, 8);
    
    // Set initial sprite sheet
    currentSpriteSheet = idleLeftSpriteSheet;
}

void Player::update(float deltaTime) {
    // Handle input (disable combat when options menu is open)
    if (!game->isOptionsOpen()) {
        handleInput(game->getInputManager());
    } else {
        // Soft-reset movement/attacks while in menu
        if (currentState == PlayerState::WALKING) setState(PlayerState::IDLE);
    }
    
    // Update movement
    move(deltaTime);

    // Dash cooldown
    if (dashCooldownTimer > 0.0f) {
        dashCooldownTimer = std::max(0.0f, dashCooldownTimer - deltaTime);
    }

    // Removed diagonal animation flip logic per request
    
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
    
    // Update spell system
    if (spellSystem) {
        spellSystem->update(deltaTime);
        
        // Apply passive health regen if available
        float regen = spellSystem->getRegenPerSecond();
        if (regen > 0) {
            health = std::min(maxHealth, health + static_cast<int>(regen * deltaTime));
        }
    }
    
    // Mana regeneration (base rate)
    mana = std::min(maxMana, mana + static_cast<int>(5.0f * deltaTime)); // 5 mana per second

    // Handle delayed second melee SFX (does not re-trigger animation)
    if (meleeSecondSfxPending && game && game->getAudioManager()) {
        meleeSecondSfxTimer -= deltaTime;
        if (meleeSecondSfxTimer <= 0.0f) {
            meleeSecondSfxPending = false;
            meleeSecondSfxTimer = 0.0f;
            static bool alt2 = true;
            game->getAudioManager()->playSound(alt2 ? "player_melee_2" : "player_melee_1");
            alt2 = !alt2;
        }
    }

    // Update fire shield if active (disabled during dash for visuals)
    if (shieldActive && currentState != PlayerState::DASHING && fireShieldSpriteSheet) {
        if (mana <= 0) { stopShield(); }
        if (!shieldActive) {
            // Early out if we just stopped due to 0 mana
            return;
        }
        fireShieldTimer += deltaTime;
        if (fireShieldTimer >= fireShieldFrameDuration) {
            fireShieldTimer = 0.0f;
            // Safety check for null sprite sheet before accessing getTotalFrames()
            if (fireShieldSpriteSheet) {
                fireShieldFrame = (fireShieldFrame + 1) % std::max(1, fireShieldSpriteSheet->getTotalFrames());
            }
        }
        // Tick AoE damage
        fireShieldTickTimer -= deltaTime;
        if (fireShieldTickTimer <= 0.0f) {
            fireShieldTickTimer = FIRE_SHIELD_TICK_SECONDS;
            // Deal AoE to nearby enemies
            if (game && game->getWorld()) {
                try {
                    auto& enemies = const_cast<std::vector<std::unique_ptr<Enemy>>&>(game->getWorld()->getEnemies());
                    SDL_Rect area = getCollisionRect();
                    // Expand radius
                    area.x -= 40; area.y -= 40; area.w += 80; area.h += 80;
                    for (auto& e : enemies) {
                        if (!e || e->isDead()) continue;
                        SDL_Rect er = e->getCollisionRect();
                        SDL_Rect inter;
                        if (SDL_IntersectRect(&area, &er, &inter)) {
                            e->takeDamage(getFireShieldDamage());
                        }
                    }
                } catch (...) {
                    std::cout << "Error: Exception in Fire Shield AoE damage calculation" << std::endl;
                }
            }
        }
        // Mana drain over time; disable if depleted
        fireShieldManaAccumulator += deltaTime * FIRE_SHIELD_MANA_DRAIN_PER_SEC;
        int drainWhole = static_cast<int>(fireShieldManaAccumulator);
        if (drainWhole > 0) {
            fireShieldManaAccumulator -= static_cast<float>(drainWhole);
            useMana(drainWhole);
            if (mana <= 0) {
                stopShield();
            }
        }
    }
}

void Player::handleInput(const InputManager* inputManager) {
    if (!inputManager || currentState == PlayerState::DEAD) {
        return;
    }
    
    // Handle movement input
    float moveX, moveY;
    inputManager->getMovementVector(moveX, moveY);
    // Removed diagonal flip tracking
    
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
        if (currentState != PlayerState::ATTACKING_MELEE && currentState != PlayerState::ATTACKING_RANGED && currentState != PlayerState::DASHING) {
            setState(PlayerState::WALKING);
        }
    } else if (currentState == PlayerState::WALKING) {
        setState(PlayerState::IDLE);
    }
    
    // Handle attack input
    // Melee on left click: trigger on press to avoid double-playing while held
    if (inputManager->isActionPressed(InputAction::ATTACK_MELEE) && canAttack()) {
        std::cout << "Melee attack triggered!" << std::endl;
        performMeleeAttack();
    }
    
    // Right-click: bow shoot (regardless of fire weapon). Sword fireball moved to melee context below
    if (inputManager->isActionHeld(InputAction::ATTACK_RANGED) && canAttack()) {
        std::cout << "Bow shot triggered!" << std::endl;
        setWeaponVisual(WeaponVisual::BOW);
        performRangedAttack();
    }

    // Dash ability on Spacebar (press)
    if (inputManager->isActionPressed(InputAction::DASH) && canDash()) {
        performDash();
    }

    // Fire shield on F key: while held, keep active; on release, stop
    if (inputManager->isActionHeld(InputAction::SHIELD)) {
        if (hasFireShield()) startShield();
    } else if (shieldActive) {
        stopShield();
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
    
    // Spell casting: 3-6 keys or Q,E,R,F
    if (spellSystem && currentState != PlayerState::ATTACKING_MELEE && currentState != PlayerState::ATTACKING_RANGED) {
        const Uint8* keyState = SDL_GetKeyboardState(nullptr);
        
        // Number keys 3-6 for spells (with cooldown checks)
        if (keyState[SDL_SCANCODE_3] && isSpellSlotReady(0)) {
            spellSystem->castSpell(0);
        } else if (keyState[SDL_SCANCODE_4] && isSpellSlotReady(1)) {
            spellSystem->castSpell(1);
        } else if (keyState[SDL_SCANCODE_5] && isSpellSlotReady(2)) {
            spellSystem->castSpell(2);
        } else if (keyState[SDL_SCANCODE_6] && isSpellSlotReady(3)) {
            spellSystem->castSpell(3);
        }
        
        // Alternative QERF keys (with cooldown checks)
        else if (keyState[SDL_SCANCODE_Q] && isSpellSlotReady(0)) {
            spellSystem->castSpell(0);
        } else if (keyState[SDL_SCANCODE_E] && !inputManager->isActionPressed(InputAction::INTERACT) && isSpellSlotReady(1)) {
            spellSystem->castSpell(1);
        } else if (keyState[SDL_SCANCODE_R] && isSpellSlotReady(2)) {
            spellSystem->castSpell(2);
        } else if (keyState[SDL_SCANCODE_F] && !inputManager->isActionPressed(InputAction::DASH) && isSpellSlotReady(3)) {
            spellSystem->castSpell(3);
        }
        
        // Stop channeling if movement keys pressed
        if (moveX != 0.0f || moveY != 0.0f) {
            spellSystem->stopChanneling();
        }
    }
}

void Player::move(float deltaTime) {
    if (currentState == PlayerState::DEAD) {
        return; // Can't move when dead
    }
    
    // Can't move while channeling spells
    if (spellSystem && spellSystem->getIsChanneling()) {
        return; // Player is stunned in place while channeling
    }
    // During dash: move along dash velocity and consume remaining distance
    if (currentState == PlayerState::DASHING) {
        float stepX = dashVelocityX * deltaTime;
        float stepY = dashVelocityY * deltaTime;
        float stepDist = std::sqrt(stepX * stepX + stepY * stepY);
        if (stepDist > dashRemainingDistance) {
            // Clamp last step
            if (stepDist > 0.0f) {
                float scale = dashRemainingDistance / stepDist;
                stepX *= scale;
                stepY *= scale;
            }
        }
        x += stepX;
        y += stepY;
        dashRemainingDistance = std::max(0.0f, dashRemainingDistance - stepDist);
        if (dashRemainingDistance <= 0.0f) {
            setState(PlayerState::IDLE);
        }
        return;
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

    // Prevent stepping into non-walkable/hazard tiles (e.g., lava) and enforce ledge rules
    if (game && game->getWorld()) {
        World* world = game->getWorld();
        int ts = world->getTileSize();
        auto feetTile = [&](float px, float py) {
            int tileX = static_cast<int>((px + width * 0.5f) / ts);
            int tileY = static_cast<int>((py + height * 0.9f) / ts);
            return std::pair<int,int>(tileX, tileY);
        };
        // X axis
        float tryX = x + dx;
        auto [txX, tyX] = feetTile(tryX, y);
        int tileIdX = world->getTile(txX, tyX);
        bool walkableX = world->isWalkable(txX, tyX);
        bool allowX = (tileIdX >= 0) && (tileIdX != TILE_LAVA) && walkableX;
        if (allowX) {
            auto [curTx, curTy] = feetTile(x, y);
            if (world->isLedgeCrossingBlocked(curTx, curTy, txX, tyX)) {
                allowX = false;
            }
        }
        if (!allowX && dx != 0) {
            // Debug why movement is blocked
            std::cout << "X movement blocked: tile(" << txX << "," << tyX << ") id=" << tileIdX 
                      << " walkable=" << walkableX << " lava=" << (tileIdX == TILE_LAVA) << std::endl;
        }
        if (allowX) x = tryX;

        // Y axis
        float tryY = y + dy;
        auto [txY, tyY] = feetTile(x, tryY);
        auto [curTx, curTy] = feetTile(x, y);
        int tileIdY = world->getTile(txY, tyY); // note: getTile expects (x,y) in tile coords
        bool walkableY = world->isWalkable(txY, tyY);
        bool allowY = (tileIdY >= 0) && (tileIdY != TILE_LAVA) && walkableY;
        
        // Debug movement details
        if (dy != 0 && abs(tyY - curTy) > 1) {
            std::cout << "WARNING: Large Y movement detected! Current tile: (" << curTx << "," << curTy 
                      << ") Target tile: (" << txY << "," << tyY << ")" 
                      << " dy=" << dy << " tryY=" << tryY << " y=" << y << std::endl;
        }
        
        // Ledge rule: if vertical move crosses platform boundary and no stairs, block it
        if (allowY) {
            if (world->isLedgeBlockedVertical(curTx, curTy, txY, tyY) || world->isLedgeCrossingBlocked(curTx, curTy, txY, tyY)) {
                allowY = false;
            }
        }
        if (!allowY && dy != 0) {
            // Debug why movement is blocked
            std::cout << "Y movement blocked: from tile(" << curTx << "," << curTy << ") to tile(" << txY << "," << tyY << ") id=" << tileIdY 
                      << " walkable=" << walkableY << " lava=" << (tileIdY == TILE_LAVA) << std::endl;
        }
        if (allowY) y = tryY;
    } else {
        x += dx;
        y += dy;
    }

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
    // Apply sword attack speed multiplier to cooldown
    {
        const auto& sword = equipment[static_cast<size_t>(EquipmentSlot::SWORD)];
        float aspd = (sword.attackSpeedMultiplier > 0.01f ? sword.attackSpeedMultiplier : 1.0f);
        meleeAttackTimer = meleeAttackCooldown / aspd;
    }
    currentFrame = 0;
    frameTimer = 0.0f;
    meleeHitConsumedThisSwing = false;
    // Ensure attack animation uses the correct sheet for current direction
    currentSpriteSheet = getSpriteSheetForState(PlayerState::ATTACKING_MELEE);
    // If fire weapon equipped, also emit a fire projectile toward mouse
    if (hasFireWeapon() && game && game->getAssetManager()) {
        float projectileX = x + width / 2.0f;
        float projectileY = y + height / 2.0f;
        InputManager* input = game->getInputManager();
        int mx = input->getMouseX();
        int my = input->getMouseY();
        int wx = mx, wy = my;
        if (auto* r = game->getRenderer()) { r->screenToWorld(mx, my, wx, wy); }
        float dirX = static_cast<float>(wx) - projectileX;
        float dirY = static_cast<float>(wy) - projectileY;
        ProjectileDirection direction(dirX, dirY); direction.normalize();
        auto* am = game->getAssetManager();
        // Use the fireball projectile sprite (single frame)
        // Animate 5-frame 32x32 projectile sheet
        // Fire projectile gets base ranged damage + fire enchantment damage
        int fireProjectileDamage = static_cast<int>(rangedDamage) + getFireDamageForHit();
        projectiles.push_back(std::make_unique<Projectile>(
            projectileX, projectileY, direction, am, fireProjectileDamage,
            std::string("assets/Textures/Spells/Projectile.png"), 5, 5, true));
    }
    // Play melee swing SFX at attack start (alternating variants), regardless of hit
    if (game && game->getAudioManager()) {
        static bool altSwing = false;
        game->getAudioManager()->playSound(altSwing ? "player_melee_2" : "player_melee_1");
        altSwing = !altSwing;
        game->getAudioManager()->startMusicDuck(0.25f, 0.6f);
    }
    
    std::cout << "Melee attack! Damage: ~" << meleeDamage << std::endl;
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
    if (rangedAttackTimer > 0.0f) {
        return; // Still on cooldown
    }
    
    // Enter ranged attack state (bow). No mana cost for bow basic shot
    setState(PlayerState::ATTACKING_RANGED);
    rangedAttackTimer = rangedAttackCooldown;
    currentFrame = 0;
    frameTimer = 0.0f;
    
    std::cout << "Ranged attack! Damage: " << rangedDamage << std::endl;
    // Play projectile fire SFX immediately on shoot
    if (game && game->getAudioManager()) {
        // Play bow fire SFX
        game->getAudioManager()->playSound("bow_fire");
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
    
    // Spawn arrow at player center without scaling; rotation enabled
    // Arrow damage includes base ranged damage + fire enchantment if bow is enchanted
    int arrowDamage = static_cast<int>(rangedDamage) + getFireDamageForHit();
    projectiles.push_back(std::make_unique<Projectile>(
        projectileX, projectileY, direction, game->getAssetManager(), arrowDamage,
        std::string("assets/Main Character/BOW ATTACK 1/arrow.png"), 1, 1, true));
}

void Player::performDash() {
    // Initiate dash in current facing direction, total distance 200 units while playing 8-frame animation
    setState(PlayerState::DASHING);
    // Lock current dash duration to 8 frames worth of dashFrameDuration
    float totalTime = dashFrameDuration * std::max(1, getSpriteSheetForState(PlayerState::DASHING)->getTotalFrames());
    float speed = (totalTime > 0.0f ? 200.0f / totalTime : 0.0f);
    dashRemainingDistance = 200.0f;
    dashVelocityX = 0.0f;
    dashVelocityY = 0.0f;
    switch (currentDirection) {
        case Direction::LEFT:  dashVelocityX = -speed; break;
        case Direction::RIGHT: dashVelocityX =  speed; break;
        case Direction::UP:    dashVelocityY = -speed; break;
        case Direction::DOWN:  dashVelocityY =  speed; break;
    }
    // Set cooldown
    dashCooldownTimer = DASH_COOLDOWN_SECONDS;
}

void Player::takeDamage(int damage) {
    // I-frames during dash: ignore damage
    if (currentState == PlayerState::DEAD || currentState == PlayerState::DASHING) {
        return;
    }
    // Fire shield immunity
    if (shieldActive) {
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

void Player::takeDamageWithType(int damage, int fireDamage) {
    // I-frames during dash: ignore damage
    if (currentState == PlayerState::DEAD || currentState == PlayerState::DASHING) {
        return;
    }
    // Fire shield immunity
    if (shieldActive) {
        return;
    }
    
    // Calculate fire resistance from armor
    int fireResistance = 0;
    
    // Check legacy Player equipment for fire resistance
    for (int i = 0; i < 9; ++i) {
        fireResistance += equipment[i].ice; // Using ice field for fire resist temporarily
    }
    
    // Check ItemSystem equipment for fire resistance
    if (itemSystem) {
        const auto& equipSlots = itemSystem->getEquipmentSlots();
        for (const auto& slot : equipSlots) {
            if (!slot.isEmpty() && slot.item->type == ItemType::EQUIPMENT) {
                fireResistance += slot.item->stats.fireResist;
            }
        }
    }
    
    // Apply fire resistance to fire damage
    int mitigatedFireDamage = std::max(0, fireDamage - fireResistance);
    int totalDamage = damage + mitigatedFireDamage;
    
    health -= totalDamage;
    if (health <= 0) {
        health = 0;
        setState(PlayerState::DEAD);
        deathAnimationFinished = false;
        std::cout << "Player died!" << std::endl;
    } else {
        setState(PlayerState::HURT);
        std::cout << "Player took " << totalDamage << " damage (" << damage << " physical + " << mitigatedFireDamage << " fire after " << fireResistance << " resist)! Health: " << health << "/" << maxHealth << std::endl;
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
        // Portal to Underworld
        if (game && nearbyObject->getType() == ObjectType::PORTAL) {
            game->enterUnderworld();
        } else if (game && nearbyObject->getType() == ObjectType::MAGIC_ANVIL) {
            // Special-case Magic Anvil: open modal UI
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
                        if (game && game->getAudioManager()) {
                            game->getAudioManager()->playSound("exp_gather");
                        }
                        notification += std::to_string(item.amount) + " XP";
                        break;
                    case LootType::HEALTH_POTION:
                        addHealthPotionCharges(item.amount);
                        notification += "HP Potion x" + std::to_string(item.amount);
                        break;
                    case LootType::MANA_POTION:
                        addManaPotionCharges(item.amount);
                        notification += "MP Potion x" + std::to_string(item.amount);
                        break;
                    case LootType::EQUIPMENT:
                        // Map loot equipment names to item IDs
                        {
                            std::string itemId = "iron_sword"; // Default
                            if (item.name.find("Sword") != std::string::npos || item.name.find("Blade") != std::string::npos) {
                                itemId = "iron_sword";
                            } else if (item.name.find("Bow") != std::string::npos) {
                                itemId = "recurve_bow";
                            } else if (item.name.find("Helmet") != std::string::npos || item.name.find("Hat") != std::string::npos) {
                                itemId = "iron_helmet";
                            } else if (item.name.find("Chest") != std::string::npos || item.name.find("Armor") != std::string::npos || 
                                      item.name.find("Mail") != std::string::npos || item.name.find("Scale") != std::string::npos) {
                                itemId = "leather_chest";
                            } else if (item.name.find("Shield") != std::string::npos) {
                                itemId = "simple_shield";
                            } else if (item.name.find("Gloves") != std::string::npos) {
                                itemId = "leather_gloves";
                            } else if (item.name.find("Boots") != std::string::npos) {
                                itemId = "leather_boots";
                            } else if (item.name.find("Belt") != std::string::npos) {
                                itemId = "rope_belt";
                            } else if (item.name.find("Ring") != std::string::npos) {
                                itemId = "silver_ring";
                            } else if (item.name.find("Necklace") != std::string::npos) {
                                itemId = "gold_necklace";
                            }
                            
                            addItemToInventoryWithRarity(itemId, item.amount, static_cast<int>(item.rarity));
                        }
                        notification += item.getRarityString() + " " + item.name;
                        break;
                    case LootType::SCROLL:
                        // Map scroll names to item IDs and add to enhanced inventory
                        {
                            std::string scrollId = "upgrade_scroll"; // Default
                            if (item.name.find("Upgrade") != std::string::npos) {
                                scrollId = "upgrade_scroll";
                                addUpgradeScrolls(item.amount); // Also add to legacy system
                            } else if (item.name.find("Fire") != std::string::npos) {
                                scrollId = "fire_scroll";
                                addElementScrolls("fire", item.amount); // Also add to legacy system
                            } else if (item.name.find("Water") != std::string::npos) {
                                scrollId = "water_scroll";
                                addElementScrolls("water", item.amount); // Also add to legacy system
                            } else if (item.name.find("Poison") != std::string::npos) {
                                scrollId = "poison_scroll";
                                addElementScrolls("poison", item.amount); // Also add to legacy system
                            } else if (item.name.find("Armor") != std::string::npos) {
                                scrollId = "armor_scroll";
                            }
                            
                            addItemToInventoryWithRarity(scrollId, item.amount, static_cast<int>(item.rarity));
                        }
                        notification += item.getRarityString() + " " + item.name;
                        break;
                    case LootType::MATERIAL:
                        // Add materials to the ItemSystem resource inventory
                        if (itemSystem) {
                            ItemRarity rarity = static_cast<ItemRarity>(static_cast<int>(item.rarity));
                            itemSystem->addItem(item.name, item.amount, rarity);
                        } else {
                            // Fallback to legacy system if ItemSystem is not available
                            addItemToInventory("material_" + item.name, item.amount);
                        }
                        notification += item.getRarityString() + " " + item.name + " x" + std::to_string(item.amount);
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
    // Compute player tile for potential future use (not currently used)
    // int playerTileX = static_cast<int>((x + width * 0.5f) / tileSize);
    // int playerTileY = static_cast<int>((y + height * 0.9f) / tileSize);
    
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
            currentFrameDuration = 0.15f; // standard walking animation speed
            break;
        case PlayerState::ATTACKING_MELEE:
        case PlayerState::ATTACKING_RANGED:
            currentFrameDuration = 0.1f;  // Fast attack animation
            break;
        case PlayerState::DASHING:
            currentFrameDuration = dashFrameDuration;
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
                // End attack animation → transition into attack end wind-down
                setState(PlayerState::ATTACK_END);
                meleeHitConsumedThisSwing = false;
                
            } else if (currentState == PlayerState::ATTACK_END) {
                // End of wind-down → return to idle
                setState(PlayerState::IDLE);
            } else if (currentState == PlayerState::DASHING) {
                // End of dash animation: stop dash and return to idle
                dashRemainingDistance = 0.0f;
                setState(PlayerState::IDLE);
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

    if (newState == PlayerState::DEAD) {
        deathDirectionAtDeath = currentDirection;
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
    if (currentState == PlayerState::ATTACKING_MELEE || currentState == PlayerState::ATTACKING_RANGED || currentState == PlayerState::ATTACK_END || currentState == PlayerState::DEAD) {
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
    int spriteOffsetX = std::max(0, (dstW - width) / 2); // center horizontally over gameplay width

    int camX = 0, camY = 0; renderer->getCamera(camX, camY);
    float z = renderer->getZoom();
    auto scaledEdge = [camX, camY, z](int wx, int wy) -> SDL_Point {
        float sx = (static_cast<float>(wx - camX)) * z;
        float sy = (static_cast<float>(wy - camY)) * z;
        return SDL_Point{ static_cast<int>(std::floor(sx)), static_cast<int>(std::floor(sy)) };
    };
    SDL_Point tl = scaledEdge(static_cast<int>(x - spriteOffsetX), static_cast<int>(y - spriteOffsetY));
    SDL_Point br = scaledEdge(static_cast<int>(x - spriteOffsetX + dstW), static_cast<int>(y - spriteOffsetY + dstH));
    SDL_Rect dstRect = { tl.x, tl.y, std::max(1, br.x - tl.x), std::max(1, br.y - tl.y) };
    
    // Render the sprite
    SDL_RenderCopy(renderer->getSDLRenderer(), currentSpriteSheet->getTexture()->getTexture(), &srcRect, &dstRect);

    // Render fire shield overlay centered on player
    if (shieldActive && mana > 0 && fireShieldSpriteSheet) {
        // Additional safety checks for texture access
        if (!fireShieldSpriteSheet->getTexture()) {
            std::cout << "Error: Fire shield sprite sheet has null texture" << std::endl;
            return;
        }
        if (!fireShieldSpriteSheet->getTexture()->getTexture()) {
            std::cout << "Error: Fire shield sprite sheet texture SDL_Texture is null" << std::endl;
            return;
        }
        
        SDL_Rect fs = fireShieldSpriteSheet->getFrameRect(fireShieldFrame);
        int fW = fireShieldSpriteSheet->getFrameWidth();
        int fH = fireShieldSpriteSheet->getFrameHeight();
        int fx = static_cast<int>(x + width/2 - fW/2);
        int fy = static_cast<int>(y + height/2 - fH/2 - 5); // nudge up 5px
        int camX2=0, camY2=0; renderer->getCamera(camX2, camY2); float z2 = renderer->getZoom();
        auto scaled = [camX2, camY2, z2](int wx, int wy) -> SDL_Point {
            float sx = (static_cast<float>(wx - camX2)) * z2;
            float sy = (static_cast<float>(wy - camY2)) * z2;
            return SDL_Point{ static_cast<int>(std::floor(sx)), static_cast<int>(std::floor(sy)) };
        };
        SDL_Point tl2 = scaled(fx, fy);
        SDL_Point br2 = scaled(fx + fW, fy + fH);
        SDL_Rect dst2{ tl2.x, tl2.y, std::max(1, br2.x - tl2.x), std::max(1, br2.y - tl2.y) };
        SDL_RenderCopy(renderer->getSDLRenderer(), fireShieldSpriteSheet->getTexture()->getTexture(), &fs, &dst2);
    }
    
    // Render spell effects
    if (spellSystem) {
        spellSystem->render(renderer);
    }
}

void Player::startShield() {
    if (shieldActive) return;
    if (!hasFireShield()) return;
    if (mana <= 0) return;
    if (!fireShieldSpriteSheet) {
        std::cout << "Warning: Fire shield sprite sheet not loaded, cannot activate shield" << std::endl;
        return;
    }
    shieldActive = true;
    fireShieldTickTimer = 0.0f;
    fireShieldTimer = 0.0f;
    fireShieldFrame = 0;
    if (game && game->getAudioManager()) {
        game->getAudioManager()->startLoopingSound("fire_shield_loop");
        game->getAudioManager()->startMusicDuck(0.2f, 0.8f);
    }
}

void Player::stopShield() {
    shieldActive = false;
    if (game && game->getAudioManager()) {
        game->getAudioManager()->stopLoopingSound("fire_shield_loop");
    }
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
    
    // Also update ItemSystem equipment if it exists - sync to match Player equipment
    if (itemSystem) {
        const auto& equipSlots = itemSystem->getEquipmentSlots();
        if (idx < equipSlots.size() && !equipSlots[idx].isEmpty()) {
            // Set ItemSystem item's plus level to match Player equipment
            Item* item = equipSlots[idx].item;
            item->plusLevel = equipment[idx].plusLevel;
        }
    }
    
    // Scale base stats per +: +1 STR/INT, +5 HP, +3 MP per plus across the board
    strength += std::max(0, deltaPlus);
    intelligence += std::max(0, deltaPlus);
    maxHealth += 5 * std::max(0, deltaPlus);
    maxMana += 3 * std::max(0, deltaPlus);
    health = std::min(health, maxHealth);
    mana = std::min(mana, maxMana);
    // Recalculate derived damage: base from STR plus sword ATK contribution
    updateSwordStatsByPlus();
    int weaponBonus = equipment[static_cast<size_t>(EquipmentSlot::SWORD)].attack;
    meleeDamage = strength * 2 + weaponBonus;
    // Update sword display name if the sword was upgraded
    if (slot == EquipmentSlot::SWORD) {
        updateSwordNameByPlus();
    }
}

void Player::upgradeSpecificItem(Item* item, int deltaPlus) {
    if (!item || item->type != ItemType::EQUIPMENT) return;
    
    int before = item->plusLevel;
    item->plusLevel = std::max(0, before + deltaPlus);
    
    // If this item is also equipped, sync the Player equipment array
    if (itemSystem) {
        const auto& equipSlots = itemSystem->getEquipmentSlots();
        for (size_t i = 0; i < equipSlots.size(); ++i) {
            if (!equipSlots[i].isEmpty() && equipSlots[i].item == item) {
                // Found the equipped item, sync Player equipment
                equipment[i].plusLevel = item->plusLevel;
                
                // Scale base stats per +: +1 STR/INT, +5 HP, +3 MP per plus across the board
                strength += std::max(0, deltaPlus);
                intelligence += std::max(0, deltaPlus);
                maxHealth += 5 * std::max(0, deltaPlus);
                maxMana += 3 * std::max(0, deltaPlus);
                health = std::min(health, maxHealth);
                mana = std::min(mana, maxMana);
                
                // Recompute derived damage if it's a weapon
                if (i == static_cast<size_t>(EquipmentSlot::SWORD)) {
                    updateSwordStatsByPlus();
                    int weaponBonus = equipment[i].attack;
                    meleeDamage = strength * 2 + weaponBonus;
                    updateSwordNameByPlus();
                }
                break;
            }
        }
    }
}

void Player::enchantEquipment(EquipmentSlot slot, const std::string& element, int amount) {
    size_t idx = static_cast<size_t>(slot);
    auto setElem = [&](int& ref){ ref = std::max(0, ref + amount); };
    
    // Update old equipment array (handle both scroll keys and element names)
    if (element == "fire" || element == "fire_scroll") setElem(equipment[idx].fire);
    else if (element == "ice" || element == "water" || element == "water_scroll") setElem(equipment[idx].ice);
    else if (element == "lightning") setElem(equipment[idx].lightning);
    else if (element == "poison" || element == "poison_scroll") setElem(equipment[idx].poison);
    else if (element == "resist_fire") setElem(equipment[idx].resistFire);
    else if (element == "resist_ice") setElem(equipment[idx].resistIce);
    else if (element == "resist_lightning") setElem(equipment[idx].resistLightning);
    else if (element == "resist_poison") setElem(equipment[idx].resistPoison);
    
    // Also update ItemSystem equipment if it exists
    if (itemSystem) {
        const auto& equipSlots = itemSystem->getEquipmentSlots();
        if (idx < equipSlots.size() && !equipSlots[idx].isEmpty()) {
            Item* item = equipSlots[idx].item;
            auto setItemElem = [&](int& ref){ ref = std::max(0, ref + amount); };
            
            if (element == "fire" || element == "fire_scroll") setItemElem(item->stats.fireAttack);
            else if (element == "ice" || element == "water" || element == "water_scroll") setItemElem(item->stats.waterAttack);
            else if (element == "poison" || element == "poison_scroll") setItemElem(item->stats.poisonAttack);
            else if (element == "resist_fire") setItemElem(item->stats.fireResist);
            else if (element == "resist_ice") setItemElem(item->stats.waterResist);
            else if (element == "resist_poison") setItemElem(item->stats.poisonResist);
            // Lightning not implemented yet in ItemSystem
        }
    }
}

void Player::enchantSpecificItem(Item* item, const std::string& element, int amount) {
    if (!item || item->type != ItemType::EQUIPMENT) return;
    
    auto setItemElem = [&](int& ref){ ref = std::max(0, ref + amount); };
    
    // Update the specific item's stats (handle both scroll keys and element names)
    if (element == "fire" || element == "fire_scroll") setItemElem(item->stats.fireAttack);
    else if (element == "ice" || element == "water" || element == "water_scroll") setItemElem(item->stats.waterAttack);
    else if (element == "poison" || element == "poison_scroll") setItemElem(item->stats.poisonAttack);
    else if (element == "resist_fire") setItemElem(item->stats.fireResist);
    else if (element == "resist_ice") setItemElem(item->stats.waterResist);
    else if (element == "resist_poison") setItemElem(item->stats.poisonResist);
    // Lightning not implemented yet in ItemSystem
    
    // If this item is also equipped, sync the Player equipment array
    if (itemSystem) {
        const auto& equipSlots = itemSystem->getEquipmentSlots();
        for (size_t i = 0; i < equipSlots.size(); ++i) {
            if (!equipSlots[i].isEmpty() && equipSlots[i].item == item) {
                // Found the equipped item, sync Player equipment
                auto setElem = [&](int& ref){ ref = std::max(0, ref + amount); };
                
                if (element == "fire" || element == "fire_scroll") setElem(equipment[i].fire);
                else if (element == "ice" || element == "water" || element == "water_scroll") setElem(equipment[i].ice);
                else if (element == "lightning") setElem(equipment[i].lightning);
                else if (element == "poison" || element == "poison_scroll") setElem(equipment[i].poison);
                else if (element == "resist_fire") setElem(equipment[i].resistFire);
                else if (element == "resist_ice") setElem(equipment[i].resistIce);
                else if (element == "resist_lightning") setElem(equipment[i].resistLightning);
                else if (element == "resist_poison") setElem(equipment[i].resistPoison);
                break;
            }
        }
    }
}

void Player::clearEquipmentSlot(EquipmentSlot slot) {
    size_t idx = static_cast<size_t>(slot);
    if (idx >= 9) return;
    
    // Clear the Player equipment array entry
    equipment[idx] = EquipmentItem{};  // Reset to default empty state
}

void Player::syncEquipmentFromItem(EquipmentSlot slot, const Item* item) {
    size_t idx = static_cast<size_t>(slot);
    if (idx >= 9 || !item) return;
    
    // Sync the Player equipment array with the ItemSystem item
    equipment[idx].plusLevel = item->plusLevel;
    equipment[idx].fire = item->stats.fireAttack;
    equipment[idx].ice = item->stats.waterAttack;
    equipment[idx].lightning = 0; // Lightning not implemented yet in ItemSystem
    equipment[idx].poison = item->stats.poisonAttack;
    equipment[idx].name = item->id;  // Use item template ID
    
    // Update base stats and derived values if it's a weapon
    if (slot == EquipmentSlot::SWORD) {
        updateSwordStatsByPlus();
        int weaponBonus = equipment[idx].attack;
        meleeDamage = strength * 2 + weaponBonus;
        updateSwordNameByPlus();
    }
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
        if (it != bags[i].end()) { it->second += amount; 
            // Trigger save after inventory change - TEMPORARILY DISABLED
            // if (game) game->saveCurrentUserState();
            return; 
        }
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
    // Trigger save after inventory change - TEMPORARILY DISABLED
    // if (game) game->saveCurrentUserState();
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
    rangedDamage = static_cast<int>(std::round(intelligence * 1.5f));
    
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
    // Load equipment into ItemSystem if available
    if (itemSystem) {
        // Load equipment into ItemSystem with rarity
        itemSystem->loadEquipmentFromSave(s.equipNames, s.equipPlus, s.equipFire, s.equipIce, s.equipLightning, s.equipPoison, s.equipRarity);
        
        // Also update old equipment array for compatibility and sync both systems
        for (int i = 0; i < 9; ++i) {
            equipment[i].plusLevel = std::max(0, s.equipPlus[i]);
            equipment[i].fire = std::max(0, s.equipFire[i]);
            equipment[i].ice = std::max(0, s.equipIce[i]);
            equipment[i].lightning = std::max(0, s.equipLightning[i]);
            equipment[i].poison = std::max(0, s.equipPoison[i]);
            equipment[i].name = s.equipNames[i];
        }
        
        // Load inventory into ItemSystem with rarity and +levels
        itemSystem->loadInventoryFromSave(s.invKey, s.invCnt, s.invRarity, s.invPlusLevel,
                                          s.resourceKey, s.resourceCnt, s.resourceRarity, s.resourcePlusLevel);
        
        // Recompute melee damage with sword stats
        updateSwordStatsByPlus();
        int weaponBonus = equipment[static_cast<size_t>(EquipmentSlot::SWORD)].attack;
        meleeDamage = strength * 2 + weaponBonus;
    } else {
        // Fallback to old equipment array if ItemSystem not available
        for (int i = 0; i < 9; ++i) {
            equipment[i].plusLevel = std::max(0, s.equipPlus[i]);
            equipment[i].fire = std::max(0, s.equipFire[i]);
            equipment[i].ice = std::max(0, s.equipIce[i]);
            equipment[i].lightning = std::max(0, s.equipLightning[i]);
            equipment[i].poison = std::max(0, s.equipPoison[i]);
            equipment[i].name = s.equipNames[i];
        }
        // Recompute melee damage with sword stats
        updateSwordStatsByPlus();
        int weaponBonus = equipment[static_cast<size_t>(EquipmentSlot::SWORD)].attack;
        meleeDamage = strength * 2 + weaponBonus;
    }
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
    // Persist equipment from ItemSystem if available
    if (itemSystem) {
        std::cout << "Saving equipment from ItemSystem..." << std::endl;
        const auto& equipSlots = itemSystem->getEquipmentSlots();
        std::cout << "Equipment slots size: " << equipSlots.size() << std::endl;
        for (int i = 0; i < 9; ++i) {
            if (i < equipSlots.size() && !equipSlots[i].isEmpty()) {
                Item* item = equipSlots[i].item;
                if (item) {  // Safety check
                    s.equipPlus[i] = item->plusLevel;
                    s.equipFire[i] = item->stats.fireAttack;
                    s.equipIce[i] = item->stats.waterAttack;
                    s.equipLightning[i] = 0; // Lightning not implemented yet
                    s.equipPoison[i] = item->stats.poisonAttack;
                    s.equipNames[i] = item->id; // Save item ID, not display name
                    s.equipRarity[i] = static_cast<int>(item->rarity);
                    std::cout << "Saving slot " << i << ": " << item->id << " (plus: " << item->plusLevel << ", fire: " << item->stats.fireAttack << ")" << std::endl;
                } else {
                    // Null item, treat as empty
                    s.equipPlus[i] = 0;
                    s.equipFire[i] = 0;
                    s.equipIce[i] = 0;
                    s.equipLightning[i] = 0;
                    s.equipPoison[i] = 0;
                    s.equipNames[i].clear();
                    s.equipRarity[i] = 0;
                }
            } else {
                // Empty slot
                std::cout << "Empty slot " << i << " (either out of range or isEmpty)" << std::endl;
                s.equipPlus[i] = 0;
                s.equipFire[i] = 0;
                s.equipIce[i] = 0;
                s.equipLightning[i] = 0;
                s.equipPoison[i] = 0;
                s.equipNames[i].clear();
                s.equipRarity[i] = 0;
            }
        }
    } else {
        // Fallback to old equipment array if ItemSystem not available
        std::cout << "ItemSystem not available, falling back to legacy equipment array" << std::endl;
        for (int i = 0; i < 9; ++i) {
            s.equipPlus[i] = equipment[i].plusLevel;
            s.equipFire[i] = equipment[i].fire;
            s.equipIce[i] = equipment[i].ice;
            s.equipLightning[i] = equipment[i].lightning;
            s.equipPoison[i] = equipment[i].poison;
            s.equipNames[i] = equipment[i].name;
            std::cout << "Legacy slot " << i << ": " << equipment[i].name << " (plus: " << equipment[i].plusLevel << ", fire: " << equipment[i].fire << ")" << std::endl;
        }
    }
    // Inventory persistence
    if (itemSystem) {
        std::cout << "Saving inventory from ItemSystem..." << std::endl;
        // Save from ItemSystem inventories
        const auto& itemInv = itemSystem->getItemInventory();
        const auto& scrollInv = itemSystem->getScrollInventory();
        const auto& resourceInv = itemSystem->getResourceInventory();
        std::cout << "Item inventory size: " << itemInv.size() << ", Scroll inventory size: " << scrollInv.size() << ", Resource inventory size: " << resourceInv.size() << std::endl;
        
        // Clear arrays first
        for (int b = 0; b < 2; ++b) {
            for (int i = 0; i < 9; ++i) {
                s.invKey[b][i].clear();
                s.invCnt[b][i] = 0;
                s.invRarity[b][i] = 0;
                s.invPlusLevel[b][i] = 0;
            }
        }
        // Clear resource inventory
        for (int i = 0; i < 9; ++i) {
            s.resourceKey[i].clear();
            s.resourceCnt[i] = 0;
            s.resourceRarity[i] = 0;
            s.resourcePlusLevel[i] = 0;
        }
        
        // Save main item inventory (bag 0)
        int idx = 0;
        std::cout << "Saving item inventory (bag 0)..." << std::endl;
        for (int i = 0; i < itemInv.size() && idx < 9; ++i) {
            if (!itemInv[i].isEmpty() && itemInv[i].item) {  // Safety check for null item
                s.invKey[0][idx] = itemInv[i].item->id;
                s.invCnt[0][idx] = itemInv[i].item->currentStack;
                s.invRarity[0][idx] = static_cast<int>(itemInv[i].item->rarity);
                s.invPlusLevel[0][idx] = itemInv[i].item->plusLevel;
                std::cout << "Saving item inv[0][" << idx << "]: " << itemInv[i].item->id << " (count: " << itemInv[i].item->currentStack << ")" << std::endl;
                idx++;
            } else {
                std::cout << "Skipping empty item inventory slot " << i << std::endl;
            }
        }
        std::cout << "Saved " << idx << " items to bag 0" << std::endl;
        
        // Save scroll inventory (bag 1)
        idx = 0;
        std::cout << "Saving scroll inventory (bag 1)..." << std::endl;
        for (int i = 0; i < scrollInv.size() && idx < 9; ++i) {
            if (!scrollInv[i].isEmpty() && scrollInv[i].item) {  // Safety check for null item
                s.invKey[1][idx] = scrollInv[i].item->id;
                s.invCnt[1][idx] = scrollInv[i].item->currentStack;
                s.invRarity[1][idx] = static_cast<int>(scrollInv[i].item->rarity);
                s.invPlusLevel[1][idx] = scrollInv[i].item->plusLevel;
                std::cout << "Saving scroll inv[1][" << idx << "]: " << scrollInv[i].item->id << " (count: " << scrollInv[i].item->currentStack << ")" << std::endl;
                idx++;
            } else {
                std::cout << "Skipping empty scroll inventory slot " << i << std::endl;
            }
        }
        std::cout << "Saved " << idx << " scrolls to bag 1" << std::endl;
        
        // Save resource inventory (separate from bags)
        idx = 0;
        std::cout << "Saving resource inventory..." << std::endl;
        for (int i = 0; i < resourceInv.size() && idx < 9; ++i) {
            if (!resourceInv[i].isEmpty() && resourceInv[i].item) {  // Safety check for null item
                s.resourceKey[idx] = resourceInv[i].item->id;
                s.resourceCnt[idx] = resourceInv[i].item->currentStack;
                s.resourceRarity[idx] = static_cast<int>(resourceInv[i].item->rarity);
                s.resourcePlusLevel[idx] = resourceInv[i].item->plusLevel;
                std::cout << "Saving resource [" << idx << "]: " << resourceInv[i].item->id << " (count: " << resourceInv[i].item->currentStack << ")" << std::endl;
                idx++;
            } else {
                std::cout << "Skipping empty resource inventory slot " << i << std::endl;
            }
        }
        std::cout << "Saved " << idx << " resources" << std::endl;
    } else {
        // Fallback to old bag system (only 2 bags in legacy system)
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
    }
    return s;
}

bool Player::consumeHealthPotion() {
    bool infinite = game && game->getInfinitePotions();
    if (healthPotionCooldown > 0.0f || (!infinite && healthPotionCharges <= 0) || health >= maxHealth) {
        return false;
    }
    int healAmount = static_cast<int>(maxHealth * 0.2f); // 20% of max health
    heal(healAmount);
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
    int manaRestore = static_cast<int>(maxMana * 0.2f); // 20% of max mana
    mana = std::min(mana + manaRestore, maxMana);
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
            if (lastWeaponVisual == WeaponVisual::BOW) {
                if (currentDirection == Direction::LEFT) return bowIdleLeftSpriteSheet;
                if (currentDirection == Direction::RIGHT) return bowIdleRightSpriteSheet;
                if (currentDirection == Direction::UP) return bowIdleUpSpriteSheet;
                return bowIdleDownSpriteSheet;
            } else {
                if (currentDirection == Direction::LEFT) return idleLeftSpriteSheet;
                if (currentDirection == Direction::RIGHT) return idleRightSpriteSheet;
                if (currentDirection == Direction::UP) return idleUpSpriteSheet;
                return idleDownSpriteSheet;
            }
        case PlayerState::WALKING:
            if (lastWeaponVisual == WeaponVisual::BOW) {
                if (currentDirection == Direction::LEFT) return bowRunLeftSpriteSheet;
                if (currentDirection == Direction::RIGHT) return bowRunRightSpriteSheet;
                if (currentDirection == Direction::UP) return bowRunUpSpriteSheet;
                return bowRunDownSpriteSheet;
            } else {
                if (currentDirection == Direction::LEFT) return walkLeftSpriteSheet;
                if (currentDirection == Direction::RIGHT) return walkRightSpriteSheet;
                if (currentDirection == Direction::UP) return walkUpSpriteSheet;
                return walkDownSpriteSheet;
            }
        case PlayerState::DASHING:
            if (currentDirection == Direction::LEFT) return dashLeftSpriteSheet;
            if (currentDirection == Direction::RIGHT) return dashRightSpriteSheet;
            if (currentDirection == Direction::UP) return dashUpSpriteSheet;
            return dashDownSpriteSheet;
        case PlayerState::ATTACKING_MELEE:
            if (currentDirection == Direction::LEFT) return meleeAttackLeftSpriteSheet;
            if (currentDirection == Direction::RIGHT) return meleeAttackRightSpriteSheet;
            if (currentDirection == Direction::UP) return meleeAttackUpSpriteSheet;
            return meleeAttackDownSpriteSheet;
        case PlayerState::ATTACKING_RANGED:
            if (lastWeaponVisual == WeaponVisual::BOW) {
                if (currentDirection == Direction::LEFT) return bowAttackLeftSpriteSheet;
                if (currentDirection == Direction::RIGHT) return bowAttackRightSpriteSheet;
                if (currentDirection == Direction::UP) return bowAttackUpSpriteSheet;
                return bowAttackDownSpriteSheet;
            } else {
                if (currentDirection == Direction::LEFT) return rangedAttackLeftSpriteSheet;
                if (currentDirection == Direction::RIGHT) return rangedAttackRightSpriteSheet;
                if (currentDirection == Direction::UP) return rangedAttackUpSpriteSheet;
                return rangedAttackDownSpriteSheet;
            }
        case PlayerState::ATTACK_END:
            if (lastWeaponVisual == WeaponVisual::BOW) {
                if (currentDirection == Direction::LEFT) return bowEndLeftSpriteSheet;
                if (currentDirection == Direction::RIGHT) return bowEndRightSpriteSheet;
                if (currentDirection == Direction::UP) return bowEndUpSpriteSheet;
                return bowEndDownSpriteSheet;
            } else {
                if (currentDirection == Direction::LEFT) return attackEndLeftSpriteSheet;
                if (currentDirection == Direction::RIGHT) return attackEndRightSpriteSheet;
                if (currentDirection == Direction::UP) return attackEndUpSpriteSheet;
                return attackEndDownSpriteSheet;
            }
        case PlayerState::HURT:
            if (currentDirection == Direction::LEFT) return hurtLeftSpriteSheet;
            if (currentDirection == Direction::RIGHT) return hurtRightSpriteSheet;
            if (currentDirection == Direction::UP) return hurtUpSpriteSheet;
            return hurtDownSpriteSheet;
        case PlayerState::DEAD:
            if (deathDirectionAtDeath == Direction::LEFT) return deathLeftSpriteSheet;
            if (deathDirectionAtDeath == Direction::RIGHT) return deathRightSpriteSheet;
            if (deathDirectionAtDeath == Direction::UP) return deathUpSpriteSheet;
            return deathDownSpriteSheet;
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
    // Disallow starting a new attack while any attack or end-attack animation is playing
    if (currentState == PlayerState::ATTACKING_MELEE || currentState == PlayerState::ATTACKING_RANGED || currentState == PlayerState::ATTACK_END) {
        return false;
    }
    return currentState != PlayerState::DEAD &&
           meleeAttackTimer <= 0.0f &&
           rangedAttackTimer <= 0.0f;
}

bool Player::isAttackAnimationPlaying() const {
    return currentState == PlayerState::ATTACKING_MELEE || currentState == PlayerState::ATTACKING_RANGED || currentState == PlayerState::ATTACK_END;
}

void Player::calculateExperienceToNext() {
    experienceToNext = level * 100; // Simple formula: level * 100
}

void Player::updateSwordNameByPlus() {
    // Map + level to sword names (1..30)
    static const char* namesByPlus[31] = {
        "", // 0 unused
        "Rusty Sword", // +1
        "Rusty Sword", // +2
        "Rusty Sword", // +3
        "Iron Shortsword", // +4
        "Iron Shortsword", // +5
        "Iron Shortsword", // +6
        "Iron Shortsword", // +7
        "Steel Longsword", // +8
        "Steel Longsword", // +9
        "Steel Longsword", // +10
        "Tempered Steel Sword", // +11
        "Tempered Steel Sword", // +12
        "Tempered Steel Sword", // +13
        "Tempered Steel Sword", // +14
        "Elven Blade", // +15
        "Elven Blade", // +16
        "Elven Blade", // +17
        "Runed Silver Sword", // +18
        "Runed Silver Sword", // +19
        "Runed Silver Sword", // +20
        "Enchanted Mithril Sword", // +21
        "Enchanted Mithril Sword", // +22
        "Enchanted Mithril Sword", // +23
        "Dragonbone Sword", // +24
        "Dragonbone Sword", // +25
        "Dragonbone Sword", // +26
        "Flameforged Greatsword", // +27
        "Flameforged Greatsword", // +28
        "Flameforged Greatsword", // +29
        "Godslayer Blade" // +30
    };
    size_t idx = static_cast<size_t>(EquipmentSlot::SWORD);
    int p = std::clamp(equipment[idx].plusLevel, 0, 30);
    if (p >= 1) {
        equipment[idx].name = namesByPlus[p];
    } else {
        // default base
        equipment[idx].name = "Rusty Sword";
    }
}

void Player::updateSwordStatsByPlus() {
    size_t idx = static_cast<size_t>(EquipmentSlot::SWORD);
    int p = std::clamp(equipment[idx].plusLevel, 0, 30);
    // Defaults for +0 or out of range
    int atk = 0; float aspd = 1.0f; float crit = 0.0f; int dur = 0;
    if (p >= 1) {
        static const int atkTable[31] = {
            0,
            5, 8, 11, 15, 19, 23, 27, 32, 37, 42, 48, 54, 60, 66, 73,
            80, 87, 95, 103, 111, 120, 129, 138, 148, 158, 168, 180, 192, 204, 220
        };
        static const float aspdTable[31] = {
            1.0f,
            1.00f, 1.01f, 1.01f, 1.02f, 1.02f, 1.03f, 1.03f, 1.04f, 1.04f, 1.05f,
            1.05f, 1.06f, 1.06f, 1.07f, 1.07f, 1.08f, 1.08f, 1.09f, 1.09f, 1.10f,
            1.10f, 1.11f, 1.11f, 1.12f, 1.12f, 1.13f, 1.13f, 1.14f, 1.14f, 1.15f
        };
        static const float critTable[31] = {
            0.0f,
            1.0f, 1.5f, 2.0f, 2.5f, 3.0f, 3.5f, 4.0f, 4.5f, 5.0f, 5.5f,
            6.0f, 6.5f, 7.0f, 7.5f, 8.0f, 8.5f, 9.0f, 9.5f, 10.0f, 10.5f,
            11.0f, 11.5f, 12.0f, 12.5f, 13.0f, 13.5f, 14.0f, 14.5f, 15.0f, 15.0f
        };
        static const int durTable[31] = {
            0,
            50, 55, 60, 65, 70, 75, 80, 85, 90, 95,
            100, 105, 110, 115, 120, 125, 130, 135, 140, 145,
            150, 155, 160, 165, 170, 175, 180, 185, 190, 195
        };
        atk = atkTable[p]; aspd = aspdTable[p]; crit = critTable[p]; dur = durTable[p];
    }
    equipment[idx].attack = atk;
    equipment[idx].attackSpeedMultiplier = aspd;
    equipment[idx].critChancePercent = crit;
    // Only increase maxDurability; keep current durability if set, else initialize to max
    int newMax = dur;
    if (newMax > equipment[idx].maxDurability) {
        equipment[idx].maxDurability = newMax;
    } else {
        equipment[idx].maxDurability = newMax; // keep synchronized with table
    }
    if (equipment[idx].durability <= 0 || equipment[idx].durability > equipment[idx].maxDurability) {
        equipment[idx].durability = equipment[idx].maxDurability;
    }
}

int Player::rollMeleeDamageForHit() {
    // Base damage = strength*2 + sword ATK
    const auto& sword = equipment[static_cast<size_t>(EquipmentSlot::SWORD)];
    int base = strength * 2 + sword.attack;
    
    // Add fire damage from weapon enchantment
    int fireDamage = sword.fire;
    
    // Add fire damage from ItemSystem if available
    if (itemSystem) {
        const auto& equipSlots = itemSystem->getEquipmentSlots();
        int swordSlot = static_cast<int>(EquipmentSlot::SWORD);
        if (swordSlot < equipSlots.size() && !equipSlots[swordSlot].isEmpty()) {
            Item* swordItem = equipSlots[swordSlot].item;
            fireDamage = std::max(fireDamage, swordItem->stats.fireAttack);
        }
    }
    
    int totalBaseDamage = base + fireDamage;
    
    // Simple crit roll using SDL ticks as rng seed source
    float critChance = std::max(0.0f, std::min(100.0f, sword.critChancePercent));
    bool isCrit = false;
    if (critChance > 0.0f) {
        // Deterministic pseudo-rng for now
        unsigned r = static_cast<unsigned>((SDL_GetTicks() * 1103515245u + 12345u) & 0x7fffffff);
        float roll01 = (r % 10000) / 100.0f; // 0..100
        isCrit = (roll01 < critChance);
    }
    
    int dmg = totalBaseDamage;
    if (isCrit) {
        dmg = static_cast<int>(std::round(totalBaseDamage * 1.5f));
    }
    
    // Durability loss on hit if any
    if (equipment[static_cast<size_t>(EquipmentSlot::SWORD)].durability > 0) {
        equipment[static_cast<size_t>(EquipmentSlot::SWORD)].durability = std::max(0, equipment[static_cast<size_t>(EquipmentSlot::SWORD)].durability - 1);
    }
    
    return std::max(1, dmg);
}

int Player::getFireDamageForHit() {
    const auto& sword = equipment[static_cast<size_t>(EquipmentSlot::SWORD)];
    int fireDamage = sword.fire;
    
    // Add fire damage from ItemSystem if available
    if (itemSystem) {
        const auto& equipSlots = itemSystem->getEquipmentSlots();
        int swordSlot = static_cast<int>(EquipmentSlot::SWORD);
        if (swordSlot < equipSlots.size() && !equipSlots[swordSlot].isEmpty()) {
            Item* swordItem = equipSlots[swordSlot].item;
            fireDamage = std::max(fireDamage, swordItem->stats.fireAttack);
        }
    }
    
    return fireDamage;
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

int Player::getFireShieldDamage() {
    // Base fire shield damage
    int baseDamage = FIRE_SHIELD_DAMAGE;
    
    // Add fire damage from waist equipment (belt determines fire shield ability)
    const auto& waist = equipment[static_cast<size_t>(EquipmentSlot::WAIST)];
    int enchantedFireDamage = waist.fire;
    
    // Add fire damage from ItemSystem waist equipment if available
    if (itemSystem) {
        const auto& equipSlots = itemSystem->getEquipmentSlots();
        int waistSlot = static_cast<int>(EquipmentSlot::WAIST);
        if (waistSlot < equipSlots.size() && !equipSlots[waistSlot].isEmpty()) {
            Item* waistItem = equipSlots[waistSlot].item;
            enchantedFireDamage = std::max(enchantedFireDamage, waistItem->stats.fireAttack);
        }
    }
    
    return baseDamage + enchantedFireDamage;
}

// Enhanced item system methods
void Player::addItemToInventoryWithRarity(const std::string& itemId, int amount, int rarity) {
    if (itemSystem) {
        itemSystem->addItem(itemId, amount, static_cast<ItemRarity>(rarity));
        // Trigger save after inventory change - TEMPORARILY DISABLED
        // if (game) game->saveCurrentUserState();
    }
}

bool Player::isSpellSlotReady(int slot) const {
    if (!spellSystem) {
        return false;
    }
    
    std::vector<SpellType> available = spellSystem->getAvailableSpells();
    if (slot < 0 || slot >= available.size()) {
        return false;
    }
    
    SpellType spell = available[slot];
    return spellSystem->getCooldownRemaining(spell) <= 0.0f;
}
