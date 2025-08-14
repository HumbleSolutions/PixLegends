#include "Enemy.h"
#include "AssetManager.h"
#include "Renderer.h"
#include "Projectile.h"
#include <algorithm>

Enemy::Enemy(float spawnX_, float spawnY_, AssetManager* assetManager, EnemyKind kind_)
    : x(spawnX_), y(spawnY_), width(128), height(128), moveSpeed(70.0f), health(200), maxHealth(200),
      currentState(EnemyState::IDLE), currentDirection(EnemyDirection::RIGHT),
      currentSpriteSheet(nullptr), currentFrame(0), frameTimer(0.0f), frameDuration(0.12f),
      currentSpriteSheetFrameWidth(0), currentSpriteSheetFrameHeight(0),
      idleLeftSpriteSheet(nullptr), idleRightSpriteSheet(nullptr),
      flyingLeftSpriteSheet(nullptr), flyingRightSpriteSheet(nullptr),
      attackLeftSpriteSheet(nullptr), attackRightSpriteSheet(nullptr),
      hurtLeftSpriteSheet(nullptr), hurtRightSpriteSheet(nullptr),
      deathSpriteSheet(nullptr),
      aggroRadius(180.0f), attackRange(140.0f), isAggroed(false),
      attackCooldownSeconds(0.8f), attackCooldownTimer(0.0f), contactDamage(10),
      spawnX(spawnX_), spawnY(spawnY_), assets(assetManager),
      kind(kind_), rangedCooldownSeconds(1.2f), rangedCooldownTimer(0.0f), rangedRange(600.0f) {
    loadSprites(assetManager);
    setState(EnemyState::IDLE);
}

void Enemy::loadSprites(AssetManager* assetManager) {
    if (kind == EnemyKind::Wizard) {
        const std::string base = AssetManager::WIZARD_PATH;
        usesSpriteFlipping = true;  // Enable sprite flipping for Wizard
        baseSpriteFacesLeft = false;  // Wizard sprites face right (renamed from RIGHT)
        
        // Load single direction sprites and use for both directions
        SpriteSheet* idleSprite = assetManager->loadSpriteSheet(base + "IDLE.png", 128, 78, 6, 6);
        SpriteSheet* walkSprite = assetManager->loadSpriteSheet(base + "WALK.png", 128, 78, 4, 4);
        SpriteSheet* attackSprite = assetManager->loadSpriteSheet(base + "MELEE ATTACK.png", 128, 78, 6, 6);
        SpriteSheet* hurtSprite = assetManager->loadSpriteSheet(base + "HURT.png", 128, 78, 4, 4);
        deathSpriteSheet = assetManager->loadSpriteSheet(base + "DEATH.png", 128, 78, 6, 6);
        
        // Assign same sprite to both directions
        idleLeftSpriteSheet = idleRightSpriteSheet = idleSprite;
        flyingLeftSpriteSheet = flyingRightSpriteSheet = walkSprite;
        attackLeftSpriteSheet = attackRightSpriteSheet = attackSprite;
        hurtLeftSpriteSheet = hurtRightSpriteSheet = hurtSprite;
        
        packRarity = PackRarity::Elite;
        renderScale = 2.0f;
    } else if (kind == EnemyKind::Skeleton) {
        // Skeleton only has single direction sprites - we'll flip them for left/right
        const std::string base = "assets/Skeleton Warrior/Sprites/";
        usesSpriteFlipping = true;
        baseSpriteFacesLeft = true;  // Skeleton sprites face left
        
        // Load the base sprites with correct frame counts
        SpriteSheet* idleSprite = assetManager->loadSpriteSheetAuto(base + "IDLE.png", 6, 6);
        SpriteSheet* walkSprite = assetManager->loadSpriteSheetAuto(base + "WALK.png", 6, 6);
        SpriteSheet* attackSprite = assetManager->loadSpriteSheetAuto(base + "ATTACK 1.png", 5, 5);
        SpriteSheet* hurtSprite = assetManager->loadSpriteSheetAuto(base + "HURT.png", 5, 5);
        deathSpriteSheet = assetManager->loadSpriteSheetAuto(base + "DEATH.png", 6, 6);
        
        // Use same sprite for both directions - we'll flip in render
        idleRightSpriteSheet = idleSprite;
        idleLeftSpriteSheet = idleSprite;  // Will be flipped when rendering
        flyingRightSpriteSheet = walkSprite;
        flyingLeftSpriteSheet = walkSprite;  // Will be flipped when rendering
        attackRightSpriteSheet = attackSprite;
        attackLeftSpriteSheet = attackSprite;  // Will be flipped when rendering
        hurtRightSpriteSheet = hurtSprite;
        hurtLeftSpriteSheet = hurtSprite;  // Will be flipped when rendering
        
        packRarity = PackRarity::Common;
        renderScale = 1.5f;
        health = maxHealth = 100;
        moveSpeed = 60.0f;
        contactDamage = 8;
        aggroRadius = 400.0f;
        attackRange = 40.0f;
    } else if (kind == EnemyKind::SkeletonMage) {
        // Magic tier - Skeleton Mage
        const std::string base = "assets/Skeleton Mage/Sprites/";
        usesSpriteFlipping = true;
        baseSpriteFacesLeft = false;  // SkeletonMage faces RIGHT
        
        SpriteSheet* idleSprite = assetManager->loadSpriteSheet(base + "IDLE.png", 192, 128, 4, 4);  // 4 frames: 768÷4=192
        SpriteSheet* walkSprite = assetManager->loadSpriteSheet(base + "WALK.png", 128, 128, 6, 6);  // 6 frames: 768÷6=128
        SpriteSheet* attackSprite = assetManager->loadSpriteSheet(base + "ATTACK.png", 128, 128, 9, 9);  // 9 frames: 1152÷9=128
        SpriteSheet* hurtSprite = assetManager->loadSpriteSheet(base + "HURT.png", 128, 128, 4, 4);  // 4 frames: 512÷4=128  
        deathSpriteSheet = assetManager->loadSpriteSheet(base + "DEATH.png", 128, 128, 10, 10);  // 10 frames: 1280÷10=128
        
        idleLeftSpriteSheet = idleRightSpriteSheet = idleSprite;
        flyingLeftSpriteSheet = flyingRightSpriteSheet = walkSprite;
        attackLeftSpriteSheet = attackRightSpriteSheet = attackSprite;
        hurtLeftSpriteSheet = hurtRightSpriteSheet = hurtSprite;
        
        packRarity = PackRarity::Magic;
        renderScale = 1.6f;
        health = maxHealth = 200;
        moveSpeed = 50.0f;  // Slower but powerful
        contactDamage = 15;
        aggroRadius = 500.0f;  // Long range magic user
        attackRange = 350.0f;  // Long range attacks
        rangedRange = 400.0f;  // Magic projectiles
    } else if (kind == EnemyKind::Pyromancer) {
        // Magic tier - Pyromancer
        const std::string base = "assets/Pyromancer/Sprites/";
        usesSpriteFlipping = true;
        baseSpriteFacesLeft = false;  // Pyromancer faces RIGHT
        
        SpriteSheet* idleSprite = assetManager->loadSpriteSheet(base + "IDLE.png", 100, 100, 4, 4);  // 4 frames: 400÷4=100
        SpriteSheet* walkSprite = assetManager->loadSpriteSheet(base + "WALK.png", 100, 100, 6, 6);  // 6 frames: 600÷6=100
        SpriteSheet* attackSprite = assetManager->loadSpriteSheet(base + "ATTACK.png", 100, 100, 6, 6);  // 6 frames: 600÷6=100
        SpriteSheet* hurtSprite = assetManager->loadSpriteSheet(base + "HURT.png", 100, 100, 3, 3);  // 3 frames: 300÷3=100
        deathSpriteSheet = assetManager->loadSpriteSheet(base + "DEATH.png", 100, 100, 10, 10);  // 10 frames: 1000÷10=100
        
        idleLeftSpriteSheet = idleRightSpriteSheet = idleSprite;
        flyingLeftSpriteSheet = flyingRightSpriteSheet = walkSprite;
        attackLeftSpriteSheet = attackRightSpriteSheet = attackSprite;
        hurtLeftSpriteSheet = hurtRightSpriteSheet = hurtSprite;
        
        packRarity = PackRarity::Magic;
        renderScale = 1.7f;
        health = maxHealth = 180;
        moveSpeed = 55.0f;
        contactDamage = 12;
        aggroRadius = 450.0f;
        attackRange = 300.0f;  // Fire magic range
        rangedRange = 350.0f;
    } else if (kind == EnemyKind::Witch) {
        // Magic tier - Witch
        const std::string base = "assets/Witch/Sprite/";
        usesSpriteFlipping = true;
        baseSpriteFacesLeft = true;  // Witch sprites face left
        
        SpriteSheet* idleSprite = assetManager->loadSpriteSheet(base + "IDLE.png", 125, 125, 6, 6);  // 6 frames: 750÷6=125
        SpriteSheet* moveSprite = assetManager->loadSpriteSheet(base + "MOVE.png", 125, 125, 6, 6);  // 6 frames: 750÷6=125
        SpriteSheet* attackSprite = assetManager->loadSpriteSheet(base + "ATTACK.png", 125, 125, 6, 6);  // 6 frames: 750÷6=125
        SpriteSheet* hurtSprite = assetManager->loadSpriteSheet(base + "HURT.png", 125, 125, 3, 3);  // 3 frames: 375÷3=125
        deathSpriteSheet = assetManager->loadSpriteSheet(base + "DEATH.png", 125, 125, 7, 7);  // 7 frames: 875÷7=125
        
        idleLeftSpriteSheet = idleRightSpriteSheet = idleSprite;
        flyingLeftSpriteSheet = flyingRightSpriteSheet = moveSprite;
        attackLeftSpriteSheet = attackRightSpriteSheet = attackSprite;
        hurtLeftSpriteSheet = hurtRightSpriteSheet = hurtSprite;
        
        packRarity = PackRarity::Magic;
        renderScale = 1.5f;
        health = maxHealth = 160;
        moveSpeed = 60.0f;
        contactDamage = 10;
        aggroRadius = 480.0f;
        attackRange = 320.0f;  // Magic projectiles
        rangedRange = 380.0f;
    } else if (kind == EnemyKind::Dragon) {
        // Elite tier - Dragon with dual attacks
        const std::string base = "assets/Dragon/Sprites/";
        usesSpriteFlipping = true;
        baseSpriteFacesLeft = true;  // Dragon faces LEFT
        hasDualAttacks = true;  // Enable dual attack system
        
        SpriteSheet* idleSprite = assetManager->loadSpriteSheet(base + "IDLE.png", 144, 96, 9, 9);  // IDLE(9)
        SpriteSheet* runSprite = assetManager->loadSpriteSheet(base + "RUN.png", 144, 96, 8, 8);  // RUN(8)
        SpriteSheet* attack1Sprite = assetManager->loadSpriteSheet(base + "ATTACK 1.png", 144, 96, 13, 13);  // MELEE ATTACK(13)
        SpriteSheet* attack2Sprite = assetManager->loadSpriteSheet(base + "ATTACK 2.png", 144, 96, 17, 17);  // FIREBREATH ATTACK(17)
        SpriteSheet* hurtSprite = assetManager->loadSpriteSheet(base + "HURT.png", 144, 96, 4, 4);  // HURT(4)
        deathSpriteSheet = assetManager->loadSpriteSheet(base + "DEATH.png", 144, 96, 10, 10);  // DEATH(10)
        
        idleLeftSpriteSheet = idleRightSpriteSheet = idleSprite;
        flyingLeftSpriteSheet = flyingRightSpriteSheet = runSprite;
        attackLeftSpriteSheet = attackRightSpriteSheet = attack1Sprite;  // Melee attack
        attack2LeftSpriteSheet = attack2RightSpriteSheet = attack2Sprite;  // Firebreath attack
        hurtLeftSpriteSheet = hurtRightSpriteSheet = hurtSprite;
        
        packRarity = PackRarity::Elite;
        renderScale = 2.5f;  // Large dragon
        health = maxHealth = 600;
        moveSpeed = 45.0f;  // Slow but powerful
        contactDamage = 35;
        aggroRadius = 600.0f;
        attackRange = 400.0f;  // Breath attacks
        rangedRange = 450.0f;
    } else if (kind == EnemyKind::Minotaur) {
        // Elite tier - Minotaur
        const std::string base = "assets/Minotaur/Sprites/";
        usesSpriteFlipping = true;
        baseSpriteFacesLeft = false;  // Minotaur faces RIGHT
        
        SpriteSheet* idleSprite = assetManager->loadSpriteSheet(base + "iDLE.png", 128, 128, 6, 6);  // IDLE(6)
        SpriteSheet* walkSprite = assetManager->loadSpriteSheet(base + "WALK.png", 128, 128, 6, 6);  // WALK(6)
        SpriteSheet* attackSprite = assetManager->loadSpriteSheet(base + "ATTACK1.png", 128, 128, 6, 6);  // ATTACK1(6)
        SpriteSheet* hurtSprite = assetManager->loadSpriteSheet(base + "HURT.png", 128, 128, 4, 4);  // HURT(4)
        deathSpriteSheet = assetManager->loadSpriteSheet(base + "DEATH.png", 128, 128, 6, 6);  // DEATH(6)
        
        idleLeftSpriteSheet = idleRightSpriteSheet = idleSprite;
        flyingLeftSpriteSheet = flyingRightSpriteSheet = walkSprite;
        attackLeftSpriteSheet = attackRightSpriteSheet = attackSprite;
        hurtLeftSpriteSheet = hurtRightSpriteSheet = hurtSprite;
        
        packRarity = PackRarity::Elite;
        renderScale = 2.2f;
        health = maxHealth = 500;
        moveSpeed = 50.0f;
        contactDamage = 25;
        aggroRadius = 450.0f;
        attackRange = 80.0f;  // Charge attacks
    } else if (kind == EnemyKind::StoneGolem) {
        // Elite tier - Stone Golem
        const std::string base = "assets/Stone Golem/Sprites/";
        usesSpriteFlipping = true;
        baseSpriteFacesLeft = true;  // StoneGolem faces LEFT
        
        SpriteSheet* idleSprite = assetManager->loadSpriteSheetAuto(base + "IDLE.png", 5, 5);
        SpriteSheet* walkSprite = assetManager->loadSpriteSheetAuto(base + "WALK.png", 6, 6);
        SpriteSheet* attackSprite = assetManager->loadSpriteSheetAuto(base + "ATTACK.png", 9, 9);
        SpriteSheet* hurtSprite = assetManager->loadSpriteSheetAuto(base + "HURT.png", 3, 3);
        deathSpriteSheet = assetManager->loadSpriteSheetAuto(base + "DEATH.png", 5, 5);
        
        idleLeftSpriteSheet = idleRightSpriteSheet = idleSprite;
        flyingLeftSpriteSheet = flyingRightSpriteSheet = walkSprite;
        attackLeftSpriteSheet = attackRightSpriteSheet = attackSprite;
        hurtLeftSpriteSheet = hurtRightSpriteSheet = hurtSprite;
        
        packRarity = PackRarity::Elite;
        renderScale = 2.3f;
        health = maxHealth = 550;  // Tank
        moveSpeed = 30.0f;  // Very slow but tough
        contactDamage = 20;
        aggroRadius = 350.0f;
        attackRange = 60.0f;
    } else if (kind == EnemyKind::HugeKnight) {
        // Elite tier - Huge Knight
        const std::string base = "assets/Huge Knight/Sprites/";
        usesSpriteFlipping = true;
        baseSpriteFacesLeft = false;  // HugeKnight faces RIGHT
        
        SpriteSheet* idleSprite = assetManager->loadSpriteSheetAuto(base + "IDLE.png", 8, 8);
        SpriteSheet* walkSprite = assetManager->loadSpriteSheetAuto(base + "WALK.png", 8, 8);
        SpriteSheet* attackSprite = assetManager->loadSpriteSheetAuto(base + "ATTACK.png", 11, 11);
        SpriteSheet* hurtSprite = assetManager->loadSpriteSheetAuto(base + "HURT.png", 6, 6);
        deathSpriteSheet = assetManager->loadSpriteSheetAuto(base + "DEATH.png", 7, 7);
        
        idleLeftSpriteSheet = idleRightSpriteSheet = idleSprite;
        flyingLeftSpriteSheet = flyingRightSpriteSheet = walkSprite;
        attackLeftSpriteSheet = attackRightSpriteSheet = attackSprite;
        hurtLeftSpriteSheet = hurtRightSpriteSheet = hurtSprite;
        
        packRarity = PackRarity::Elite;
        renderScale = 2.4f;
        health = maxHealth = 480;
        moveSpeed = 40.0f;
        contactDamage = 30;
        aggroRadius = 400.0f;
        attackRange = 90.0f;  // Heavy weapon reach
    } else if (kind == EnemyKind::Cyclops) {
        // Elite tier - Cyclops with dual attacks (melee + laser)
        const std::string base = "assets/Cyclops/Sprite/";
        usesSpriteFlipping = true;
        baseSpriteFacesLeft = true;  // Cyclops faces LEFT
        hasDualAttacks = true;  // Enable dual attack system
        
        SpriteSheet* idleSprite = assetManager->loadSpriteSheetAuto(base + "IDLE.png", 3, 3);
        SpriteSheet* walkSprite = assetManager->loadSpriteSheetAuto(base + "WALK.png", 6, 6);
        SpriteSheet* attack1Sprite = assetManager->loadSpriteSheetAuto(base + "ATTACK_1.png", 7, 7);  // Melee attack
        SpriteSheet* attack2Sprite = assetManager->loadSpriteSheetAuto(base + "ATTACK_2.png", 11, 11);  // Laser attack
        SpriteSheet* hurtSprite = assetManager->loadSpriteSheetAuto(base + "HURT.png", 3, 3);
        deathSpriteSheet = assetManager->loadSpriteSheetAuto(base + "DEATH.png", 8, 8);
        
        idleLeftSpriteSheet = idleRightSpriteSheet = idleSprite;
        flyingLeftSpriteSheet = flyingRightSpriteSheet = walkSprite;
        attackLeftSpriteSheet = attackRightSpriteSheet = attack1Sprite;  // Melee attack
        attack2LeftSpriteSheet = attack2RightSpriteSheet = attack2Sprite;  // Laser attack
        hurtLeftSpriteSheet = hurtRightSpriteSheet = hurtSprite;
        
        packRarity = PackRarity::Elite;
        renderScale = 2.6f;  // Large cyclops
        health = maxHealth = 520;
        moveSpeed = 35.0f;
        contactDamage = 28;
        aggroRadius = 550.0f;  // Laser vision range
        attackRange = 500.0f;  // Laser attacks
        rangedRange = 600.0f;
    } else if (kind == EnemyKind::Centaur) {
        // Elite tier - Centaur
        const std::string base = "assets/Centaur/Sprite/";
        usesSpriteFlipping = true;
        baseSpriteFacesLeft = true;  // Centaur faces LEFT
        
        SpriteSheet* idleSprite = assetManager->loadSpriteSheet(base + "IDLE.png", 100, 100, 6, 6);  // IDLE(6)
        SpriteSheet* walkSprite = assetManager->loadSpriteSheet(base + "RUN.png", 100, 100, 3, 3);  // RUN(3)
        SpriteSheet* attackSprite = assetManager->loadSpriteSheet(base + "ATTACK.png", 100, 100, 8, 8);  // ATTACK(8)
        SpriteSheet* hurtSprite = assetManager->loadSpriteSheet(base + "HURT.png", 100, 100, 3, 3);  // HURT(3)
        deathSpriteSheet = assetManager->loadSpriteSheet(base + "DEATH.png", 100, 100, 10, 10);  // DEATH(10)
        
        idleLeftSpriteSheet = idleRightSpriteSheet = idleSprite;
        flyingLeftSpriteSheet = flyingRightSpriteSheet = walkSprite;
        attackLeftSpriteSheet = attackRightSpriteSheet = attackSprite;
        hurtLeftSpriteSheet = hurtRightSpriteSheet = hurtSprite;
        
        packRarity = PackRarity::Elite;
        renderScale = 2.4f;  // Large centaur
        health = maxHealth = 480;
        moveSpeed = 85.0f;  // Fast horse movement
        contactDamage = 25;
        aggroRadius = 450.0f;
        attackRange = 120.0f;  // Spear reach
    } else if (kind == EnemyKind::HeadlessHorseman) {
        // Elite tier - Headless Horseman
        const std::string base = "assets/Headless Horseman/Sprites/";  // Fixed path
        usesSpriteFlipping = true;
        baseSpriteFacesLeft = true;  // HeadlessHorseman faces LEFT
        
        SpriteSheet* idleSprite = assetManager->loadSpriteSheet(base + "IDLE.png", 150, 150, 4, 4);  // IDLE(4)
        SpriteSheet* walkSprite = assetManager->loadSpriteSheet(base + "RUN.png", 150, 150, 4, 4);  // RUN(4)
        SpriteSheet* attackSprite = assetManager->loadSpriteSheet(base + "ATTACK.png", 150, 150, 8, 8);  // ATTACK(8)
        SpriteSheet* hurtSprite = assetManager->loadSpriteSheet(base + "HURT.png", 150, 150, 3, 3);  // HURT(3)
        deathSpriteSheet = assetManager->loadSpriteSheet(base + "DEATH.png", 150, 150, 10, 10);  // DEATH(10)
        
        idleLeftSpriteSheet = idleRightSpriteSheet = idleSprite;
        flyingLeftSpriteSheet = flyingRightSpriteSheet = walkSprite;
        attackLeftSpriteSheet = attackRightSpriteSheet = attackSprite;
        hurtLeftSpriteSheet = hurtRightSpriteSheet = hurtSprite;
        
        packRarity = PackRarity::Elite;
        renderScale = 2.3f;  // Large mounted figure
        health = maxHealth = 500;
        moveSpeed = 90.0f;  // Very fast horse
        contactDamage = 27;
        aggroRadius = 500.0f;
        attackRange = 150.0f;  // Sword reach from horseback
    } else if (kind == EnemyKind::Medusa) {
        // Elite tier - Medusa with dual attacks (magic + melee)
        const std::string base = "assets/Medusa/Sprite/";
        usesSpriteFlipping = true;
        baseSpriteFacesLeft = true;  // Medusa faces LEFT
        hasDualAttacks = true;  // Enable dual attack system
        
        SpriteSheet* idleSprite = assetManager->loadSpriteSheet(base + "IDLE.png", 150, 150, 6, 6);  // IDLE(6)
        SpriteSheet* walkSprite = assetManager->loadSpriteSheet(base + "MOVE.png", 150, 150, 4, 4);  // MOVE(4)
        SpriteSheet* attack1Sprite = assetManager->loadSpriteSheet(base + "ATTACK1.png", 150, 150, 6, 6);  // Magic attack (6)
        SpriteSheet* attack2Sprite = assetManager->loadSpriteSheet(base + "ATTACK2.png", 150, 150, 7, 7);  // Melee attack (7)
        SpriteSheet* hurtSprite = assetManager->loadSpriteSheet(base + "HURT.png", 150, 150, 3, 3);  // HURT(3)
        deathSpriteSheet = assetManager->loadSpriteSheet(base + "DEATH.png", 150, 150, 6, 6);  // DEATH(6)
        
        idleLeftSpriteSheet = idleRightSpriteSheet = idleSprite;
        flyingLeftSpriteSheet = flyingRightSpriteSheet = walkSprite;
        attackLeftSpriteSheet = attackRightSpriteSheet = attack1Sprite;  // Magic attack (default)
        attack2LeftSpriteSheet = attack2RightSpriteSheet = attack2Sprite;  // Melee attack
        hurtLeftSpriteSheet = hurtRightSpriteSheet = hurtSprite;
        
        packRarity = PackRarity::Elite;
        renderScale = 2.2f;  // Human-sized but menacing
        health = maxHealth = 440;
        moveSpeed = 50.0f;  // Slithering movement
        contactDamage = 26;
        aggroRadius = 400.0f;
        attackRange = 350.0f;  // Petrifying gaze
        rangedRange = 400.0f;
    } else if (kind == EnemyKind::Cerberus) {
        // Elite tier - Cerberus
        const std::string base = "assets/Cerberus/Sprite/";  // Fixed: Sprite not Sprites
        usesSpriteFlipping = true;
        baseSpriteFacesLeft = true;  // Cerberus faces LEFT
        
        SpriteSheet* idleSprite = assetManager->loadSpriteSheetAuto(base + "IDLE.png", 3, 3);
        SpriteSheet* walkSprite = assetManager->loadSpriteSheetAuto(base + "RUN.png", 3, 3);  // Changed WALK to RUN
        SpriteSheet* attackSprite = assetManager->loadSpriteSheetAuto(base + "ATTACK.png", 6, 6);
        SpriteSheet* hurtSprite = assetManager->loadSpriteSheetAuto(base + "HURT.png", 3, 3);
        deathSpriteSheet = assetManager->loadSpriteSheetAuto(base + "DEATH.png", 7, 7);
        
        idleLeftSpriteSheet = idleRightSpriteSheet = idleSprite;
        flyingLeftSpriteSheet = flyingRightSpriteSheet = walkSprite;
        attackLeftSpriteSheet = attackRightSpriteSheet = attackSprite;
        hurtLeftSpriteSheet = hurtRightSpriteSheet = hurtSprite;
        
        packRarity = PackRarity::Elite;
        renderScale = 2.5f;  // Large three-headed hound
        health = maxHealth = 580;
        moveSpeed = 75.0f;  // Fast predator
        contactDamage = 30;
        aggroRadius = 500.0f;
        attackRange = 100.0f;  // Multiple bite attacks
    } else if (kind == EnemyKind::Gryphon) {
        // Elite tier - Gryphon
        const std::string base = "assets/Gryphon/Sprites/";
        usesSpriteFlipping = true;
        baseSpriteFacesLeft = false;  // Gryphon faces RIGHT
        
        SpriteSheet* idleSprite = assetManager->loadSpriteSheetAuto(base + "IDLE.png", 4, 4);
        SpriteSheet* walkSprite = assetManager->loadSpriteSheetAuto(base + "MOVE.png", 4, 4);  // Changed FLY to MOVE
        SpriteSheet* attackSprite = assetManager->loadSpriteSheetAuto(base + "ATTACK .png", 7, 7);
        SpriteSheet* hurtSprite = assetManager->loadSpriteSheetAuto(base + "HURT.png", 4, 4);
        deathSpriteSheet = assetManager->loadSpriteSheetAuto(base + "DEATH.png", 7, 7);
        
        idleLeftSpriteSheet = idleRightSpriteSheet = idleSprite;
        flyingLeftSpriteSheet = flyingRightSpriteSheet = walkSprite;
        attackLeftSpriteSheet = attackRightSpriteSheet = attackSprite;
        hurtLeftSpriteSheet = hurtRightSpriteSheet = hurtSprite;
        
        packRarity = PackRarity::Elite;
        renderScale = 2.4f;  // Large flying creature
        health = maxHealth = 460;
        moveSpeed = 95.0f;  // Very fast flying
        contactDamage = 24;
        aggroRadius = 600.0f;  // Aerial spotting
        attackRange = 110.0f;  // Diving attacks
    } else if (kind == EnemyKind::Lizardman) {
        // Common tier - Lizardman
        const std::string base = "assets/Lizardman/Sprites/";
        usesSpriteFlipping = true;
        baseSpriteFacesLeft = false;  // Fixed: Lizardman sprites face RIGHT
        
        SpriteSheet* idleSprite = assetManager->loadSpriteSheet(base + "IDLE.png", 158, 125, 3, 3);  // IDLE(3)
        SpriteSheet* walkSprite = assetManager->loadSpriteSheet(base + "WALK.png", 158, 125, 6, 6);  // WALK(6)
        SpriteSheet* attackSprite = assetManager->loadSpriteSheet(base + "ATTACK.png", 158, 125, 6, 6);  // ATTACK(6)
        SpriteSheet* hurtSprite = assetManager->loadSpriteSheet(base + "HURT.png", 158, 125, 3, 3);  // HURT(3)
        deathSpriteSheet = assetManager->loadSpriteSheet(base + "DEATH.png", 158, 125, 6, 6);  // DEATH(6)
        
        idleLeftSpriteSheet = idleRightSpriteSheet = idleSprite;
        flyingLeftSpriteSheet = flyingRightSpriteSheet = walkSprite;
        attackLeftSpriteSheet = attackRightSpriteSheet = attackSprite;
        hurtLeftSpriteSheet = hurtRightSpriteSheet = hurtSprite;
        
        packRarity = PackRarity::Common;
        renderScale = 1.3f;
        health = maxHealth = 120;
        moveSpeed = 65.0f;
        contactDamage = 10;
        aggroRadius = 380.0f;
        attackRange = 45.0f;
    } else if (kind == EnemyKind::DwarfWarrior) {
        // Common tier - Dwarf Warrior
        const std::string base = "assets/Dwarf Warrior/Sprite/";
        usesSpriteFlipping = true;
        baseSpriteFacesLeft = true;  // Correct: Dwarf sprites face LEFT
        
        SpriteSheet* idleSprite = assetManager->loadSpriteSheet(base + "IDLE.png", 75, 100, 4, 3);  // IDLE(3) - Fixed frame count
        SpriteSheet* walkSprite = assetManager->loadSpriteSheet(base + "WALK.png", 100, 100, 6, 6);  // WALK(6) - Correct frame count
        SpriteSheet* attackSprite = assetManager->loadSpriteSheet(base + "ATTACK.png", 150, 100, 4, 6);  // ATTACK(6) - Fixed frame count  
        SpriteSheet* hurtSprite = assetManager->loadSpriteSheet(base + "HURT.png", 75, 100, 4, 3);  // HURT(3) - Fixed frame count
        deathSpriteSheet = assetManager->loadSpriteSheet(base + "DEATH.png", 150, 100, 4, 6);  // DEATH(6) - Fixed frame count
        
        idleLeftSpriteSheet = idleRightSpriteSheet = idleSprite;
        flyingLeftSpriteSheet = flyingRightSpriteSheet = walkSprite;
        attackLeftSpriteSheet = attackRightSpriteSheet = attackSprite;
        hurtLeftSpriteSheet = hurtRightSpriteSheet = hurtSprite;
        
        packRarity = PackRarity::Common;
        renderScale = 1.4f;
        health = maxHealth = 140;
        moveSpeed = 55.0f;
        contactDamage = 12;
        aggroRadius = 360.0f;
        attackRange = 40.0f;
    } else if (kind == EnemyKind::Harpy) {
        // Common tier - Harpy
        const std::string base = "assets/Harpy/Sprite/";
        usesSpriteFlipping = true;
        baseSpriteFacesLeft = true;  // Fixed: Harpy sprites face LEFT
        
        SpriteSheet* idleSprite = assetManager->loadSpriteSheet(base + "IDLE_MOVE.png", 100, 100, 4, 4);  // IDLE(4) - Combined idle/move file
        SpriteSheet* attackSprite = assetManager->loadSpriteSheet(base + "ATTACk.png", 200, 100, 4, 8);  // ATTACK(8) - Fixed frame count, weird capitalization
        SpriteSheet* hurtSprite = assetManager->loadSpriteSheet(base + "HURT.png", 75, 100, 4, 3);  // HURT(3) - Fixed frame count
        deathSpriteSheet = assetManager->loadSpriteSheet(base + "DEATH.png", 150, 100, 4, 6);  // DEATH(6) - Fixed frame count
        
        idleLeftSpriteSheet = idleRightSpriteSheet = idleSprite;
        flyingLeftSpriteSheet = flyingRightSpriteSheet = idleSprite;  // Flying creature
        attackLeftSpriteSheet = attackRightSpriteSheet = attackSprite;
        hurtLeftSpriteSheet = hurtRightSpriteSheet = hurtSprite;
        
        packRarity = PackRarity::Common;
        renderScale = 1.2f;
        health = maxHealth = 100;
        moveSpeed = 85.0f;  // Flying, faster
        contactDamage = 9;
        aggroRadius = 420.0f;
        attackRange = 50.0f;
    } else if (kind == EnemyKind::Imp) {
        // Trash tier - Imp
        const std::string base = "assets/Imp/Sprite/";
        usesSpriteFlipping = true;
        baseSpriteFacesLeft = false;  // Fixed: Imp sprites face RIGHT
        
        SpriteSheet* idleSprite = assetManager->loadSpriteSheet(base + "IDLE_MOVE.png", 100, 100, 4, 4);  // IDLE(4)
        SpriteSheet* attackSprite = assetManager->loadSpriteSheet(base + "ATTACK.png", 100, 100, 8, 8);  // ATTACK(8)
        SpriteSheet* hurtSprite = assetManager->loadSpriteSheet(base + "HURT.png", 100, 100, 3, 3);  // HURT(3)
        deathSpriteSheet = assetManager->loadSpriteSheet(base + "DEATH.png", 100, 100, 7, 7);  // DEATH(7)
        
        idleLeftSpriteSheet = idleRightSpriteSheet = idleSprite;
        flyingLeftSpriteSheet = flyingRightSpriteSheet = idleSprite;  // Use idle/move for flying
        attackLeftSpriteSheet = attackRightSpriteSheet = attackSprite;
        hurtLeftSpriteSheet = hurtRightSpriteSheet = hurtSprite;
        
        packRarity = PackRarity::Trash;
        renderScale = 1.0f;
        health = maxHealth = 60;
        moveSpeed = 80.0f;
        contactDamage = 5;
        aggroRadius = 300.0f;
        attackRange = 200.0f;  // Ranged attacks
        rangedRange = 250.0f;  // Flying Eye projectile range
    } else if (kind == EnemyKind::FlyingEye) {
        // Trash tier - Flying Eye
        const std::string base = "assets/Flying Eye/Sprites/";
        usesSpriteFlipping = true;
        baseSpriteFacesLeft = false;  // Fixed: Flying Eye sprites face RIGHT
        
        SpriteSheet* idleSprite = assetManager->loadSpriteSheet(base + "MOVE.png", 150, 150, 6, 6);  // IDLE(6) - using MOVE sprite
        SpriteSheet* moveSprite = assetManager->loadSpriteSheet(base + "MOVE.png", 150, 150, 6, 6);  // MOVE(6)
        SpriteSheet* attackSprite = assetManager->loadSpriteSheet(base + "ATTACK.png", 150, 150, 6, 6);  // ATTACK(6)
        SpriteSheet* hurtSprite = assetManager->loadSpriteSheet(base + "HURT.png", 150, 150, 4, 4);  // HURT(4)
        deathSpriteSheet = assetManager->loadSpriteSheet(base + "DEATH.png", 150, 150, 5, 5);  // DEATH(5)
        
        idleLeftSpriteSheet = idleRightSpriteSheet = idleSprite;
        flyingLeftSpriteSheet = flyingRightSpriteSheet = moveSprite;
        attackLeftSpriteSheet = attackRightSpriteSheet = attackSprite;
        hurtLeftSpriteSheet = hurtRightSpriteSheet = hurtSprite;
        
        packRarity = PackRarity::Trash;
        renderScale = 1.2f;
        health = maxHealth = 50;
        moveSpeed = 90.0f;
        contactDamage = 4;
        aggroRadius = 350.0f;
        attackRange = 200.0f;  // Ranged attacks
        rangedRange = 250.0f;  // Flying Eye projectile range
    } else if (kind == EnemyKind::PoisonSkull) {
        // Trash tier - Poison Skull
        const std::string base = "assets/Poison Skull/Sprite/";
        usesSpriteFlipping = true;
        baseSpriteFacesLeft = true;  // Correct: Poison Skull sprites face LEFT
        
        SpriteSheet* idleSprite = assetManager->loadSpriteSheet(base + "IDLE.png", 150, 100, 6, 6);  // IDLE(6)
        SpriteSheet* walkSprite = assetManager->loadSpriteSheet(base + "WALK.png", 150, 100, 5, 5);  // WALK(5)
        SpriteSheet* attackSprite = assetManager->loadSpriteSheet(base + "ATTACK.png", 150, 100, 9, 9);  // ATTACK(9)
        SpriteSheet* hurtSprite = assetManager->loadSpriteSheet(base + "HURT.png", 150, 100, 3, 3);  // HURT(3)
        deathSpriteSheet = assetManager->loadSpriteSheet(base + "DEATH.png", 150, 100, 9, 9);  // DEATH(9)
        
        idleLeftSpriteSheet = idleRightSpriteSheet = idleSprite;
        flyingLeftSpriteSheet = flyingRightSpriteSheet = walkSprite;
        attackLeftSpriteSheet = attackRightSpriteSheet = attackSprite;
        hurtLeftSpriteSheet = hurtRightSpriteSheet = hurtSprite;
        
        packRarity = PackRarity::Trash;
        renderScale = 1.1f;
        health = maxHealth = 70;
        moveSpeed = 70.0f;
        contactDamage = 6;
        aggroRadius = 320.0f;
        attackRange = 35.0f;
    } else if (kind == EnemyKind::Gargoyle) {
        // Elite tier - Gargoyle
        const std::string base = "assets/Gargoyle/Sprites/";
        usesSpriteFlipping = true;
        baseSpriteFacesLeft = true;  // Gargoyle faces LEFT
        
        SpriteSheet* idleSprite = assetManager->loadSpriteSheet(base + "IDLE.png", 150, 150, 4, 4);  // IDLE(4)
        SpriteSheet* walkSprite = assetManager->loadSpriteSheet(base + "MOVE.png", 150, 150, 8, 4);  // MOVE(4)
        SpriteSheet* attackSprite = assetManager->loadSpriteSheet(base + "ATTACK.png", 150, 150, 8, 8);  // ATTACK(8)
        SpriteSheet* hurtSprite = assetManager->loadSpriteSheet(base + "HURT.png", 150, 150, 3, 4);  // HURT(4)
        deathSpriteSheet = assetManager->loadSpriteSheet(base + "DEATH.png", 150, 150, 10, 5);  // DEATH(5)
        
        idleLeftSpriteSheet = idleRightSpriteSheet = idleSprite;
        flyingLeftSpriteSheet = flyingRightSpriteSheet = walkSprite;
        attackLeftSpriteSheet = attackRightSpriteSheet = attackSprite;
        hurtLeftSpriteSheet = hurtRightSpriteSheet = hurtSprite;
        
        packRarity = PackRarity::Elite;
        renderScale = 2.2f;  // Stone guardian size
        health = maxHealth = 420;
        moveSpeed = 80.0f;  // Flying stone creature
        contactDamage = 23;
        aggroRadius = 450.0f;
        attackRange = 95.0f;  // Stone claws
    } else if (kind == EnemyKind::Werewolf) {
        // Elite tier - Werewolf with transformation ability
        const std::string base = "assets/Werewolf/Sprites/";
        usesSpriteFlipping = true;
        baseSpriteFacesLeft = false;  // Werewolf faces RIGHT
        hasTransformationAbility = true;  // Enable transformation
        isTransformed = false;  // Start in human form
        
        // Load human form sprites (for initial idle state)
        humanIdleLeftSpriteSheet = assetManager->loadSpriteSheet(base + "IDLE HUMAN.png", 158, 125, 6, 6);  // HUMAN IDLE(6)
        humanIdleRightSpriteSheet = humanIdleLeftSpriteSheet;  // Use flipping for right direction
        
        // Load transformation animation
        transformationSpriteSheet = assetManager->loadSpriteSheet(base + "TRANSFORMATION.png", 158, 125, 8, 8);  // TRANSFORMATION(8)
        
        // Load werewolf form sprites (used after transformation)
        SpriteSheet* idleSprite = assetManager->loadSpriteSheet(base + "IDLE.png", 158, 125, 6, 6);  // WEREWOLF IDLE(6)
        SpriteSheet* walkSprite = assetManager->loadSpriteSheet(base + "RUN.png", 158, 125, 6, 6);  // RUN(6)
        SpriteSheet* attackSprite = assetManager->loadSpriteSheet(base + "ATTACK1.png", 158, 125, 4, 4);  // ATTACK1(4)
        SpriteSheet* hurtSprite = assetManager->loadSpriteSheet(base + "HURT.png", 158, 125, 4, 4);  // HURT(4)
        deathSpriteSheet = assetManager->loadSpriteSheet(base + "DEATH.png", 158, 125, 10, 10);  // DEATH(10)
        
        idleLeftSpriteSheet = idleRightSpriteSheet = idleSprite;
        flyingLeftSpriteSheet = flyingRightSpriteSheet = walkSprite;
        attackLeftSpriteSheet = attackRightSpriteSheet = attackSprite;
        hurtLeftSpriteSheet = hurtRightSpriteSheet = hurtSprite;
        
        packRarity = PackRarity::Elite;
        renderScale = 2.3f;  // Large beast form
        health = maxHealth = 490;
        moveSpeed = 95.0f;  // Very fast predator
        contactDamage = 28;
        aggroRadius = 520.0f;  // Keen senses
        attackRange = 105.0f;  // Claw attacks
    /*} else if (kind == EnemyKind::Mimic) {
        // Elite tier - Mimic - DISABLED: Reserved for chest spawning feature
        const std::string base = "assets/Mimic/Sprite/";
        usesSpriteFlipping = true;
        baseSpriteFacesLeft = false;  // Mimic sprites face right
        
        SpriteSheet* idleSprite = assetManager->loadSpriteSheetAuto(base + "IDLE.png", 4, 4);
        SpriteSheet* walkSprite = assetManager->loadSpriteSheetAuto(base + "WALK.png", 4, 4);
        SpriteSheet* attackSprite = assetManager->loadSpriteSheetAuto(base + "ATTACK.png", 4, 4);
        SpriteSheet* hurtSprite = assetManager->loadSpriteSheetAuto(base + "HURT.png", 4, 4);
        deathSpriteSheet = assetManager->loadSpriteSheetAuto(base + "DEATH.png", 4, 4);
        
        idleLeftSpriteSheet = idleRightSpriteSheet = idleSprite;
        flyingLeftSpriteSheet = flyingRightSpriteSheet = walkSprite;
        attackLeftSpriteSheet = attackRightSpriteSheet = attackSprite;
        hurtLeftSpriteSheet = hurtRightSpriteSheet = hurtSprite;
        
        packRarity = PackRarity::Elite;
        renderScale = 2.0f;  // Chest-sized trickster
        health = maxHealth = 380;
        moveSpeed = 45.0f;  // Slow but deceptive
        contactDamage = 26;
        aggroRadius = 300.0f;  // Ambush predator
        attackRange = 80.0f;  // Surprise attacks*/
    } else if (kind == EnemyKind::MaskedOrc) {
        // Elite tier - Masked Orc
        const std::string base = "assets/Masked Orc/Sprites/";
        usesSpriteFlipping = true;
        baseSpriteFacesLeft = true;  // MaskedOrc faces LEFT
        
        SpriteSheet* idleSprite = assetManager->loadSpriteSheetAuto(base + "IDLE.png", 4, 4);
        SpriteSheet* walkSprite = assetManager->loadSpriteSheetAuto(base + "WALK.png", 6, 6);
        SpriteSheet* attackSprite = assetManager->loadSpriteSheetAuto(base + "ATTACK.png", 6, 6);
        SpriteSheet* hurtSprite = assetManager->loadSpriteSheetAuto(base + "HURT.png", 3, 3);
        deathSpriteSheet = assetManager->loadSpriteSheetAuto(base + "DEATH.png", 10, 10);
        
        idleLeftSpriteSheet = idleRightSpriteSheet = idleSprite;
        flyingLeftSpriteSheet = flyingRightSpriteSheet = walkSprite;
        attackLeftSpriteSheet = attackRightSpriteSheet = attackSprite;
        hurtLeftSpriteSheet = hurtRightSpriteSheet = hurtSprite;
        
        packRarity = PackRarity::Elite;
        renderScale = 2.1f;  // Elite warrior size
        health = maxHealth = 450;
        moveSpeed = 70.0f;  // Armored warrior
        contactDamage = 25;
        aggroRadius = 420.0f;
        attackRange = 110.0f;  // Axe reach
    } else if (kind == EnemyKind::KoboldWarrior) {
        // Elite tier - Kobold Warrior with advanced abilities
        const std::string base = "assets/Kobold Warrior/Sprites/";
        usesSpriteFlipping = true;
        baseSpriteFacesLeft = false;  // KoboldWarrior faces RIGHT
        hasAdvancedAbilities = true;  // Enable advanced ability system
        
        SpriteSheet* idleSprite = assetManager->loadSpriteSheetAuto(base + "IDLE.png", 6, 6);  // IDLE(6)
        SpriteSheet* walkSprite = assetManager->loadSpriteSheetAuto(base + "RUN.png", 8, 8);  // RUN(8)
        SpriteSheet* attack1Sprite = assetManager->loadSpriteSheetAuto(base + "ATTACK 1.png", 5, 5);  // ATTACK1(5)
        SpriteSheet* attack2Sprite = assetManager->loadSpriteSheetAuto(base + "ATTACK 2.png", 5, 5);  // ATTACK2(5)
        SpriteSheet* attack3Sprite = assetManager->loadSpriteSheetAuto(base + "ATTACK 3.png", 6, 6);  // ATTACK3(6)
        SpriteSheet* superAttackSprite = assetManager->loadSpriteSheetAuto(base + "STRONG ATTACK.png", 12, 12);  // STRONG ATTACK(12)
        SpriteSheet* jumpSprite = assetManager->loadSpriteSheetAuto(base + "JUMP.png", 3, 3);  // JUMP(3)
        SpriteSheet* dashSprite = assetManager->loadSpriteSheetAuto(base + "DASH.png", 7, 7);  // DASH(7)
        SpriteSheet* hurtSprite = assetManager->loadSpriteSheetAuto(base + "HURT.png", 4, 4);  // HURT(4)
        deathSpriteSheet = assetManager->loadSpriteSheetAuto(base + "DEATH.png", 10, 10);  // DEATH(10)
        
        idleLeftSpriteSheet = idleRightSpriteSheet = idleSprite;
        flyingLeftSpriteSheet = flyingRightSpriteSheet = walkSprite;
        attackLeftSpriteSheet = attackRightSpriteSheet = attack1Sprite;  // Default to attack 1
        attack2LeftSpriteSheet = attack2RightSpriteSheet = attack2Sprite;
        attack3LeftSpriteSheet = attack3RightSpriteSheet = attack3Sprite;
        superAttackLeftSpriteSheet = superAttackRightSpriteSheet = superAttackSprite;
        jumpLeftSpriteSheet = jumpRightSpriteSheet = jumpSprite;
        dashLeftSpriteSheet = dashRightSpriteSheet = dashSprite;
        hurtLeftSpriteSheet = hurtRightSpriteSheet = hurtSprite;
        
        packRarity = PackRarity::Elite;
        renderScale = 1.8f;  // Small but elite warrior
        health = maxHealth = 350;
        moveSpeed = 85.0f;  // Quick and agile
        contactDamage = 22;
        aggroRadius = 380.0f;
        attackRange = 95.0f;  // Sword combat
        attackCooldownSeconds = 0.4f;  // Faster attacks for advanced warrior
    } else if (kind == EnemyKind::SatyrArcher) {
        // Elite tier - Satyr Archer
        const std::string base = "assets/Satyr Archer/Sprite/";  // Fixed path
        usesSpriteFlipping = true;
        baseSpriteFacesLeft = true;  // SatyrArcher faces LEFT
        
        SpriteSheet* idleSprite = assetManager->loadSpriteSheet(base + "IDLE.png", 125, 100, 3, 3);  // IDLE(3)
        SpriteSheet* walkSprite = assetManager->loadSpriteSheet(base + "WALK.png", 125, 100, 6, 6);  // WALK(6)
        SpriteSheet* attackSprite = assetManager->loadSpriteSheet(base + "ATTACK.png", 125, 100, 4, 4);  // ATTACK(4)
        SpriteSheet* hurtSprite = assetManager->loadSpriteSheet(base + "HURT.png", 125, 100, 4, 4);  // HURT(4)
        deathSpriteSheet = assetManager->loadSpriteSheet(base + "DEATH.png", 125, 100, 4, 4);  // DEATH(4)
        
        idleLeftSpriteSheet = idleRightSpriteSheet = idleSprite;
        flyingLeftSpriteSheet = flyingRightSpriteSheet = walkSprite;
        attackLeftSpriteSheet = attackRightSpriteSheet = attackSprite;
        hurtLeftSpriteSheet = hurtRightSpriteSheet = hurtSprite;
        
        packRarity = PackRarity::Elite;
        renderScale = 2.0f;  // Elite archer size
        health = maxHealth = 400;
        moveSpeed = 75.0f;  // Mobile archer
        contactDamage = 20;
        aggroRadius = 600.0f;  // Long range archer
        attackRange = 450.0f;  // Bow range
        rangedRange = 500.0f;
    } else if (kind == EnemyKind::BabyDragon) {
        // Elite tier - Baby Dragon
        const std::string base = "assets/Baby Dragon/Sprites/";
        usesSpriteFlipping = true;
        baseSpriteFacesLeft = true;  // BabyDragon faces LEFT
        
        SpriteSheet* idleSprite = assetManager->loadSpriteSheet(base + "IDLE.png", 150, 150, 4, 4);  // IDLE(4)
        SpriteSheet* walkSprite = assetManager->loadSpriteSheet(base + "MOVE.png", 150, 150, 4, 4);  // MOVE(4)
        SpriteSheet* attackSprite = assetManager->loadSpriteSheet(base + "ATTACK.png", 150, 150, 4, 4);  // ATTACK(4)
        SpriteSheet* hurtSprite = assetManager->loadSpriteSheet(base + "HURT.png", 150, 150, 4, 4);  // HURT(4)
        deathSpriteSheet = assetManager->loadSpriteSheet(base + "DEATH.png", 150, 150, 4, 4);  // DEATH(4)
        
        idleLeftSpriteSheet = idleRightSpriteSheet = idleSprite;
        flyingLeftSpriteSheet = flyingRightSpriteSheet = walkSprite;
        attackLeftSpriteSheet = attackRightSpriteSheet = attackSprite;
        hurtLeftSpriteSheet = hurtRightSpriteSheet = hurtSprite;
        
        packRarity = PackRarity::Elite;
        renderScale = 2.1f;  // Young but dangerous dragon
        health = maxHealth = 430;
        moveSpeed = 90.0f;  // Fast flying
        contactDamage = 24;
        aggroRadius = 500.0f;  // Dragon senses
        attackRange = 200.0f;  // Fire breath
        rangedRange = 250.0f;
    } else {
        if (kind == EnemyKind::Demon) {
            // Demon boss assets with sprite flipping
            const std::string base = "assets/Demon Boss/Sprites/";
            usesSpriteFlipping = true;  // Enable sprite flipping for Demon
            baseSpriteFacesLeft = false;  // Demon sprites face right (renamed from RIGHT)
            
            // Load single direction sprites
            SpriteSheet* idleSprite = assetManager->loadSpriteSheetAuto(base + "IDLE.png", 4, 4);
            SpriteSheet* flyingSprite = assetManager->loadSpriteSheetAuto(base + "FLYING.png", 4, 4);
            SpriteSheet* attackSprite = assetManager->loadSpriteSheetAuto(base + "ATTACK.png", 6, 6);
            SpriteSheet* hurtSprite = assetManager->loadSpriteSheetAuto(base + "HURT.png", 3, 3);
            deathSpriteSheet = assetManager->loadSpriteSheetAuto(base + "DEATH.png", 10, 10);
            
            // Assign same sprite to both directions
            idleLeftSpriteSheet = idleRightSpriteSheet = idleSprite;
            flyingLeftSpriteSheet = flyingRightSpriteSheet = flyingSprite;
            attackLeftSpriteSheet = attackRightSpriteSheet = attackSprite;
            hurtLeftSpriteSheet = hurtRightSpriteSheet = hurtSprite;
            
            packRarity = PackRarity::Elite;
            renderScale = 2.2f;
        } else {
            // Goblin minion assets with sprite flipping
            const std::string base = "assets/Goblin/Sprites/";
            usesSpriteFlipping = true;  // Enable sprite flipping for Goblin
            baseSpriteFacesLeft = false;  // Goblin sprites face right (renamed from RIGHT)
            
            // Load single direction sprites
            SpriteSheet* idleSprite = assetManager->loadSpriteSheet(base + "IDLE.png", 115, 78, 3, 3);
            SpriteSheet* runSprite = assetManager->loadSpriteSheet(base + "RUN.png", 115, 78, 6, 6);
            SpriteSheet* attackSprite = assetManager->loadSpriteSheet(base + "ATTACK1.png", 115, 78, 6, 6);
            SpriteSheet* hurtSprite = assetManager->loadSpriteSheet(base + "HURT.png", 115, 78, 3, 3);
            deathSpriteSheet = assetManager->loadSpriteSheet(base + "DEATH.png", 115, 78, 10, 10);
            
            // Assign same sprite to both directions
            idleLeftSpriteSheet = idleRightSpriteSheet = idleSprite;
            flyingLeftSpriteSheet = flyingRightSpriteSheet = runSprite;
            attackLeftSpriteSheet = attackRightSpriteSheet = attackSprite;
            hurtLeftSpriteSheet = hurtRightSpriteSheet = hurtSprite;
            packRarity = PackRarity::Trash; // default minion rarity
            renderScale = 0.75f; // about half of previous size
            health = maxHealth = 60;
            moveSpeed = 90.0f;
            contactDamage = 6;
            aggroRadius = 900.0f; // massive aggro radius
            // Require very close contact before entering ATTACKING so goblins push into the player
            attackRange = 28.0f;
        }
    }
}

void Enemy::takeDamage(int amount) {
    if (currentState == EnemyState::DEAD) return;
    health -= std::max(0, amount);
    if (health <= 0) {
        health = 0;
        setState(EnemyState::DEAD);
        if (deathTicksMs == 0) deathTicksMs = SDL_GetTicks();
        // Death SFX per enemy kind
        if (assets) {
            if (kind == EnemyKind::Goblin) {
                // Play goblin death
                // Access via global AudioManager through assets is not wired; trigger via SDL_mixer-less fallback is not available here.
                // Death sound is instead handled in Game loop when detecting dead enemies (optional).
            }
        }
    } else {
        setState(EnemyState::HURT);
    }
}

SDL_Rect Enemy::getCollisionRect() const {
    // Approximate tight body hitbox based on visual sprite (ignore large transparent frame margins)
    // Start from the rendered destination footprint, then shrink by tuned factors per enemy kind.
    const int frameW = getWidth();
    const int frameH = getHeight();
    const float renderScaleLocal = renderScale; // per-enemy scale
    const int scaledW = static_cast<int>(frameW * renderScaleLocal);
    const int scaledH = static_cast<int>(frameH * renderScaleLocal);

    // Tuned scales to match visible body portion (smaller than the full frame)
    float widthScale = 0.55f;
    float heightScale = 0.70f;
    float yOffsetScale = 0.05f; // nudge down slightly towards feet
    if (kind == EnemyKind::Demon) {
        widthScale = 0.50f;
        heightScale = 0.65f;
        yOffsetScale = 0.08f;
    } else if (kind == EnemyKind::Wizard) {
        widthScale = 0.50f;
        heightScale = 0.72f;
        yOffsetScale = 0.04f;
    } else if (kind == EnemyKind::Skeleton) {
        widthScale = 0.45f;
        heightScale = 0.70f;
        yOffsetScale = 0.05f;
    } else if (kind == EnemyKind::SkeletonMage) {
        widthScale = 0.45f;
        heightScale = 0.75f;
        yOffsetScale = 0.05f;
    } else if (kind == EnemyKind::Pyromancer) {
        widthScale = 0.50f;
        heightScale = 0.80f;
        yOffsetScale = 0.05f;
    } else if (kind == EnemyKind::Witch) {
        widthScale = 0.45f;
        heightScale = 0.75f;
        yOffsetScale = 0.05f;
    } else if (kind == EnemyKind::Lizardman) {
        widthScale = 0.50f;
        heightScale = 0.75f;
        yOffsetScale = 0.05f;
    } else if (kind == EnemyKind::DwarfWarrior) {
        widthScale = 0.45f;
        heightScale = 0.70f;
        yOffsetScale = 0.10f;  // Shorter dwarf
    } else if (kind == EnemyKind::Harpy) {
        widthScale = 0.50f;
        heightScale = 0.65f;
        yOffsetScale = 0.0f;  // Flying creature
    } else if (kind == EnemyKind::Imp) {
        widthScale = 0.40f;
        heightScale = 0.65f;
        yOffsetScale = 0.10f;
    } else if (kind == EnemyKind::FlyingEye) {
        widthScale = 0.60f;
        heightScale = 0.60f;
        yOffsetScale = 0.0f;  // Floating eye
    } else if (kind == EnemyKind::PoisonSkull) {
        widthScale = 0.50f;
        heightScale = 0.55f;
        yOffsetScale = 0.0f;  // Floating skull
    } else if (kind == EnemyKind::Centaur) {
        widthScale = 0.60f;
        heightScale = 0.85f;
        yOffsetScale = 0.0f;  // Horse body
    } else if (kind == EnemyKind::HeadlessHorseman) {
        widthScale = 0.65f;
        heightScale = 0.80f;
        yOffsetScale = 0.0f;  // Mounted figure
    } else if (kind == EnemyKind::Medusa) {
        widthScale = 0.45f;
        heightScale = 0.70f;
        yOffsetScale = 0.05f;  // Humanoid snake
    } else if (kind == EnemyKind::Cerberus) {
        widthScale = 0.70f;
        heightScale = 0.60f;
        yOffsetScale = 0.05f;  // Large hound
    } else if (kind == EnemyKind::Gryphon) {
        widthScale = 0.55f;
        heightScale = 0.65f;
        yOffsetScale = 0.0f;  // Flying creature
    } else if (kind == EnemyKind::Gargoyle) {
        widthScale = 0.50f;
        heightScale = 0.70f;
        yOffsetScale = 0.0f;  // Stone creature
    } else if (kind == EnemyKind::Werewolf) {
        widthScale = 0.55f;
        heightScale = 0.75f;
        yOffsetScale = 0.05f;  // Beast form
    /*} else if (kind == EnemyKind::Mimic) {
        widthScale = 0.60f;
        heightScale = 0.50f;
        yOffsetScale = 0.15f;  // Chest shape*/
    } else if (kind == EnemyKind::MaskedOrc) {
        widthScale = 0.50f;
        heightScale = 0.75f;
        yOffsetScale = 0.05f;  // Armored warrior
    } else if (kind == EnemyKind::KoboldWarrior) {
        widthScale = 0.45f;
        heightScale = 0.70f;
        yOffsetScale = 0.10f;  // Small warrior
    } else if (kind == EnemyKind::SatyrArcher) {
        widthScale = 0.45f;
        heightScale = 0.75f;
        yOffsetScale = 0.05f;  // Humanoid archer
    } else if (kind == EnemyKind::BabyDragon) {
        widthScale = 0.55f;
        heightScale = 0.60f;
        yOffsetScale = 0.0f;  // Young dragon
    }

    const int bodyW = std::max(1, static_cast<int>(scaledW * widthScale));
    const int bodyH = std::max(1, static_cast<int>(scaledH * heightScale));
    const int centerX = static_cast<int>(x);
    const int centerY = static_cast<int>(y + scaledH * yOffsetScale);

    SDL_Rect r;
    r.w = bodyW;
    r.h = bodyH;
    r.x = centerX - bodyW / 2;
    r.y = centerY - bodyH / 2;
    return r;
}

bool Enemy::isWithinAttackRange(float playerX, float playerY) const {
    float dx = playerX - x;
    float dy = playerY - y;
    return (dx * dx + dy * dy) <= attackRange * attackRange;
}

void Enemy::resetToSpawn() {
    x = spawnX;
    y = spawnY;
    health = maxHealth;
    isAggroed = false;
    attackCooldownTimer = 0.0f;
    setDirection(EnemyDirection::RIGHT);
    setState(EnemyState::IDLE);
}

void Enemy::setState(EnemyState newState) {
    if (currentState == newState) return;
    currentState = newState;
    currentFrame = 0;
    frameTimer = 0.0f;
    currentSpriteSheet = pickSpriteSheetForState(currentState);
    if (currentSpriteSheet) {
        currentSpriteSheetFrameWidth = currentSpriteSheet->getFrameWidth();
        currentSpriteSheetFrameHeight = currentSpriteSheet->getFrameHeight();
    }
}

void Enemy::setDirection(EnemyDirection newDirection) {
    if (currentDirection == newDirection) return;
    currentDirection = newDirection;
    currentSpriteSheet = pickSpriteSheetForState(currentState);
}

SpriteSheet* Enemy::pickSpriteSheetForState(EnemyState state) const {
    switch (state) {
        case EnemyState::IDLE:
            // Special case for werewolf in human form (only if not yet transformed)
            if (hasTransformationAbility && !isTransformed && humanIdleLeftSpriteSheet) {
                if (currentDirection == EnemyDirection::LEFT) {
                    return humanIdleLeftSpriteSheet;
                } else {
                    return humanIdleRightSpriteSheet ? humanIdleRightSpriteSheet : humanIdleLeftSpriteSheet;
                }
            }
            // Normal idle sprites (including transformed werewolf)
            if (currentDirection == EnemyDirection::LEFT) {
                return idleLeftSpriteSheet ? idleLeftSpriteSheet : (flyingLeftSpriteSheet ? flyingLeftSpriteSheet : attackLeftSpriteSheet);
            } else {
                return idleRightSpriteSheet ? idleRightSpriteSheet : (flyingRightSpriteSheet ? flyingRightSpriteSheet : attackRightSpriteSheet);
            }
        case EnemyState::FLYING:
            return currentDirection == EnemyDirection::LEFT ? flyingLeftSpriteSheet : flyingRightSpriteSheet;
        case EnemyState::ATTACKING:
            // Check for advanced abilities (Kobold Warrior)
            if (hasAdvancedAbilities) {
                switch (currentAttackType) {
                    case 1:
                        // Attack 1 uses the default attackLeftSpriteSheet/attackRightSpriteSheet
                        return currentDirection == EnemyDirection::LEFT ? attackLeftSpriteSheet : attackRightSpriteSheet;
                    case 2:
                        if (attack2LeftSpriteSheet) {
                            return currentDirection == EnemyDirection::LEFT ? attack2LeftSpriteSheet : attack2RightSpriteSheet;
                        }
                        break;
                    case 3:
                        if (attack3LeftSpriteSheet) {
                            return currentDirection == EnemyDirection::LEFT ? attack3LeftSpriteSheet : attack3RightSpriteSheet;
                        }
                        break;
                }
            }
            // Check for dual attack support (Dragon)
            if (hasDualAttacks && useAttack2 && attack2LeftSpriteSheet) {
                return currentDirection == EnemyDirection::LEFT ? attack2LeftSpriteSheet : attack2RightSpriteSheet;
            }
            return currentDirection == EnemyDirection::LEFT ? attackLeftSpriteSheet : attackRightSpriteSheet;
        case EnemyState::HURT:
            return currentDirection == EnemyDirection::LEFT ? hurtLeftSpriteSheet : hurtRightSpriteSheet;
        case EnemyState::DEAD:
            return deathSpriteSheet;
        case EnemyState::TRANSFORMING:
            return transformationSpriteSheet;
        case EnemyState::JUMPING:
            if (hasAdvancedAbilities && jumpLeftSpriteSheet) {
                return currentDirection == EnemyDirection::LEFT ? jumpLeftSpriteSheet : jumpRightSpriteSheet;
            }
            return currentDirection == EnemyDirection::LEFT ? flyingLeftSpriteSheet : flyingRightSpriteSheet;
        case EnemyState::DASHING:
            if (hasAdvancedAbilities && dashLeftSpriteSheet) {
                return currentDirection == EnemyDirection::LEFT ? dashLeftSpriteSheet : dashRightSpriteSheet;
            }
            return currentDirection == EnemyDirection::LEFT ? flyingLeftSpriteSheet : flyingRightSpriteSheet;
        case EnemyState::SUPER_ATTACKING:
            if (hasAdvancedAbilities && superAttackLeftSpriteSheet) {
                return currentDirection == EnemyDirection::LEFT ? superAttackLeftSpriteSheet : superAttackRightSpriteSheet;
            }
            return currentDirection == EnemyDirection::LEFT ? attackLeftSpriteSheet : attackRightSpriteSheet;
    }
    return nullptr;
}

void Enemy::triggerTransformation() {
    if (!hasTransformationAbility || isTransformed) return;
    
    setState(EnemyState::TRANSFORMING);
    // After transformation animation completes, the werewolf will be in transformed state
}

void Enemy::update(float deltaTime, float playerX, float playerY) {
    if (currentState == EnemyState::DEAD) {
        updateAnimation(deltaTime);
        return;
    }

    // Cooldowns
    if (attackCooldownTimer > 0.0f) attackCooldownTimer -= deltaTime;
    
    // Advanced ability cooldowns (Kobold Warrior)
    if (hasAdvancedAbilities) {
        if (dashCooldown > 0.0f) dashCooldown -= deltaTime;
        if (jumpCooldown > 0.0f) jumpCooldown -= deltaTime;
        if (superAttackCooldown > 0.0f) superAttackCooldown -= deltaTime;
    }

    // Simple AI: face player, float towards player until within attack range
    float dx = playerX - x;
    float dy = playerY - y;
    float distSq = dx*dx + dy*dy;

    EnemyDirection newDirection = (dx < 0) ? EnemyDirection::LEFT : EnemyDirection::RIGHT;
    if (newDirection != currentDirection) {
        setDirection(newDirection);
    }

    // Aggro check
    if (!isAggroed) {
        if (distSq <= aggroRadius * aggroRadius) {
            isAggroed = true;
            // Trigger transformation for werewolves when they become aggroed
            if (hasTransformationAbility && !isTransformed) {
                triggerTransformation();
                updateAnimation(deltaTime);
                return;
            }
        } else {
            // Idle in place until aggroed
            if (currentState != EnemyState::IDLE) setState(EnemyState::IDLE);
            updateAnimation(deltaTime);
            return;
        }
    }

    if (distSq > attackRange * attackRange) {
        // Move towards player
        float dist = std::max(1.0f, sqrtf(distSq));
        // Slightly overshoot toward player center to encourage stacking into melee range
        float desiredX = playerX;
        float desiredY = playerY + 6.0f; // bias toward lower body
        float ddx = desiredX - x;
        float ddy = desiredY - y;
        float ddist = std::max(1.0f, sqrtf(ddx*ddx + ddy*ddy));
        float vx = (ddx / ddist) * moveSpeed;
        float vy = (ddy / ddist) * moveSpeed * 0.8f; // keep vertical a bit slower but closer
        x += vx * deltaTime;
        y += vy * deltaTime;
        if (currentState != EnemyState::FLYING)
            setState(EnemyState::FLYING);
    } else {
        // Attack state (looping animation) - only trigger new attacks when not already in combat states
        if (currentState != EnemyState::ATTACKING && currentState != EnemyState::JUMPING && 
            currentState != EnemyState::DASHING && currentState != EnemyState::SUPER_ATTACKING) {
            
            // Kobold Warrior advanced abilities
            if (hasAdvancedAbilities && kind == EnemyKind::KoboldWarrior) {
                float dist = sqrtf(distSq);
                
                // Super attack (has highest priority if available and close range)
                if (superAttackCooldown <= 0.0f && dist < 120.0f && rand() % 100 < 15) {  // 15% chance
                    superAttackCooldown = 8.0f;  // 8 second cooldown
                    setState(EnemyState::SUPER_ATTACKING);
                }
                // Jump attack (medium range, aggressive approach)
                else if (jumpCooldown <= 0.0f && dist > 150.0f && dist < 250.0f && rand() % 100 < 25) {  // 25% chance
                    jumpCooldown = 10.0f;  // 10 second cooldown
                    isJumping = true;
                    setState(EnemyState::JUMPING);
                }
                // Dash attack (close-medium range for gap closing)
                else if (dashCooldown <= 0.0f && dist > 100.0f && dist < 200.0f && rand() % 100 < 30) {  // 30% chance
                    dashCooldown = 10.0f;  // 10 second cooldown
                    isDashing = true;
                    dashTargetX = playerX;  // Store target position
                    dashTargetY = playerY;
                    setState(EnemyState::DASHING);
                }
                // Regular attacks (cycle through 1, 2, 3) - respect attack cooldown
                else if (attackCooldownTimer <= 0.0f) {
                    currentAttackType = (currentAttackType % 3) + 1;  // Cycle 1->2->3->1
                    attackCooldownTimer = attackCooldownSeconds;  // Set cooldown
                    setState(EnemyState::ATTACKING);
                }
            }
            // For dual attack enemies (Dragon and Cyclops)
            else if (hasDualAttacks) {
                float dist = sqrtf(distSq);
                
                if (kind == EnemyKind::Cyclops) {
                    // Cyclops: laser attack at range, melee up close
                    if (dist > 120.0f && dist < 500.0f) {
                        useAttack2 = true;  // Laser attack at range
                    } else {
                        useAttack2 = false;  // Melee attack up close
                    }
                } else if (kind == EnemyKind::Medusa) {
                    // Medusa: magic attack at mid-range, melee up close
                    if (dist > 100.0f && dist < 350.0f) {
                        useAttack2 = false;  // ATTACK1 = magic attack at mid-range
                    } else {
                        useAttack2 = true;   // ATTACK2 = melee attack up close
                    }
                } else {
                    // Dragon: firebreath for medium-long range, melee for very close
                    if (dist > 80.0f && dist < 400.0f) {
                        useAttack2 = true;  // Firebreath attack (prioritized)
                    } else {
                        useAttack2 = false;  // Melee attack (close range only)
                    }
                }
                setState(EnemyState::ATTACKING);
            }
            // Default attack behavior
            else {
                setState(EnemyState::ATTACKING);
            }
        }
    }

    // Ranged behavior for wizard, dragon, cyclops, medusa, magic tier enemies, and flying eye
    if (kind == EnemyKind::Wizard || (kind == EnemyKind::Dragon && useAttack2) || (kind == EnemyKind::Cyclops && useAttack2) ||
        (kind == EnemyKind::Medusa && !useAttack2) || kind == EnemyKind::Pyromancer || kind == EnemyKind::SkeletonMage || 
        kind == EnemyKind::Witch || kind == EnemyKind::FlyingEye) {
        // Cooldown timer
        if (rangedCooldownTimer > 0.0f) rangedCooldownTimer -= deltaTime;
        // Fire at range if within rangedRange and has cooldown ready
        if (distSq <= rangedRange * rangedRange && rangedCooldownTimer <= 0.0f) {
            // Use specific projectile sprites for each enemy type
            if (kind == EnemyKind::Cyclops && useAttack2) {
                fireProjectileTowards(playerX, playerY, assets, "assets/Cyclops/Sprite/cyclops_lazer_projectile.png", 1);
            } else if (kind == EnemyKind::Pyromancer) {
                fireProjectileTowards(playerX, playerY, assets, "assets/Pyromancer/Sprites/pyromancer_projectile.png", 4);
            } else if (kind == EnemyKind::SkeletonMage) {
                fireProjectileTowards(playerX, playerY, assets, "assets/Skeleton Mage/Sprites/skele_mage_projectile.png", 5);
            } else if (kind == EnemyKind::Witch) {
                fireProjectileTowards(playerX, playerY, assets, "assets/Witch/Sprite/WITCH_PROJECTILE.png", 6);
            } else if (kind == EnemyKind::FlyingEye) {
                fireProjectileTowards(playerX, playerY, assets, "assets/Flying Eye/Sprites/projectile.png", 1);
            } else if (kind == EnemyKind::Medusa && !useAttack2) {
                // Medusa magic attack projectile (petrifying gaze)
                fireProjectileTowards(playerX, playerY, assets);  // Use default projectile for now
            } else {
                // Default projectile for other enemies (like Wizard)
                fireProjectileTowards(playerX, playerY, assets);
            }
            rangedCooldownTimer = rangedCooldownSeconds;
        }
        updateProjectiles(deltaTime);
    }

    // Special movement behavior for advanced abilities
    if (hasAdvancedAbilities && kind == EnemyKind::KoboldWarrior) {
        if (currentState == EnemyState::JUMPING && isJumping) {
            // Jump towards player with high speed
            float jumpSpeed = moveSpeed * 2.5f;
            float dist = std::max(1.0f, sqrtf(distSq));
            float vx = (dx / dist) * jumpSpeed;
            float vy = (dy / dist) * jumpSpeed;
            x += vx * deltaTime;
            y += vy * deltaTime;
        }
        else if (currentState == EnemyState::DASHING && isDashing) {
            // Dash towards stored target position with very high speed
            float dashSpeed = moveSpeed * 3.5f;  // Even faster dash
            float targetDx = dashTargetX - x;
            float targetDy = dashTargetY - y;
            float targetDist = std::max(1.0f, sqrtf(targetDx*targetDx + targetDy*targetDy));
            float vx = (targetDx / targetDist) * dashSpeed;
            float vy = (targetDy / targetDist) * dashSpeed;
            x += vx * deltaTime;
            y += vy * deltaTime;
        }
    }

    updateAnimation(deltaTime);
}

void Enemy::updateAnimation(float deltaTime) {
    if (!currentSpriteSheet) return;
    frameTimer += deltaTime;

    float duration = frameDuration;
    if (currentState == EnemyState::ATTACKING) {
        // Adjust timing for dual attacks
        if (hasDualAttacks && useAttack2) {
            if (kind == EnemyKind::Cyclops) {
                duration = 0.10f;  // Cyclops laser attack (11 frames)
            } else if (kind == EnemyKind::Medusa) {
                duration = 0.08f;  // Medusa melee attack (7 frames) - faster
            } else {
                duration = 0.12f;  // Dragon firebreath (17 frames)
            }
        } else if (hasDualAttacks && !useAttack2 && kind == EnemyKind::Medusa) {
            duration = 0.10f;  // Medusa magic attack (6 frames)
        } else {
            duration = 0.09f;  // Normal speed for melee attacks
        }
        // Adjust timing for Kobold Warrior advanced attacks
        if (hasAdvancedAbilities) {
            switch (currentAttackType) {
                case 1: duration = 0.06f; break;  // Attack 1: 5 frames (faster)
                case 2: duration = 0.06f; break;  // Attack 2: 5 frames (faster)
                case 3: duration = 0.05f; break;  // Attack 3: 6 frames (faster)
            }
        }
    }
    // Special ability timings for Kobold Warrior
    if (currentState == EnemyState::SUPER_ATTACKING) duration = 0.08f;  // Super Attack: 12 frames
    if (currentState == EnemyState::JUMPING) duration = 0.15f;  // Jump: 3 frames (slower for dramatic effect)
    if (currentState == EnemyState::DASHING) duration = 0.06f;  // Dash: 7 frames (fast)
    if (currentState == EnemyState::DEAD) duration = 0.12f;
    if (currentState == EnemyState::TRANSFORMING) duration = 0.15f; // Slower for dramatic effect

    if (frameTimer >= duration) {
        frameTimer = 0.0f;
        currentFrame++;
        if (currentFrame >= currentSpriteSheet->getTotalFrames()) {
            if (currentState == EnemyState::DEAD) {
                currentFrame = currentSpriteSheet->getTotalFrames() - 1; // hold last
            } else if (currentState == EnemyState::TRANSFORMING) {
                // Transformation complete - switch to werewolf form
                isTransformed = true;
                currentFrame = 0;
                setState(EnemyState::IDLE); // Return to idle in werewolf form
            } else if (currentState == EnemyState::JUMPING) {
                // Jump attack complete - return to normal behavior
                isJumping = false;
                currentFrame = 0;
                setState(EnemyState::IDLE);
            } else if (currentState == EnemyState::DASHING) {
                // Dash attack complete - return to normal behavior  
                isDashing = false;
                currentFrame = 0;
                setState(EnemyState::IDLE);
            } else if (currentState == EnemyState::SUPER_ATTACKING) {
                // Super attack complete - return to normal behavior
                currentFrame = 0;
                setState(EnemyState::IDLE);
            } else if (currentState == EnemyState::ATTACKING && hasAdvancedAbilities && kind == EnemyKind::KoboldWarrior) {
                // Kobold Warrior attacks complete after one cycle to allow attack type cycling
                currentFrame = 0;
                setState(EnemyState::IDLE);
            } else {
                currentFrame = 0; // loop
            }
        }
    }
}

void Enemy::render(Renderer* renderer) const {
    if (!renderer || !currentSpriteSheet || !currentSpriteSheet->getTexture() || !currentSpriteSheet->getTexture()->getTexture()) return;

    SDL_Rect src = currentSpriteSheet->getFrameRect(currentFrame);

    // Destination in screen space with zoom support
    float scale = renderScale;
    int camX = 0, camY = 0; renderer->getCamera(camX, camY);
    float z = renderer->getZoom();
    SDL_Rect dst;
    // Use top-left and bottom-right scaling to avoid subpixel gaps
    auto scaledEdge = [camX, camY, z](int wx, int wy) -> SDL_Point {
        float sx = (static_cast<float>(wx - camX)) * z;
        float sy = (static_cast<float>(wy - camY)) * z;
        return SDL_Point{ static_cast<int>(std::floor(sx)), static_cast<int>(std::floor(sy)) };
    };
    int halfW = static_cast<int>(src.w * scale / 2.0f);
    int halfH = static_cast<int>(src.h * scale / 2.0f);
    SDL_Point tl = scaledEdge(static_cast<int>(x) - halfW, static_cast<int>(y) - halfH);
    SDL_Point br = scaledEdge(static_cast<int>(x) + halfW, static_cast<int>(y) + halfH);
    dst.x = tl.x;
    dst.y = tl.y;
    dst.w = std::max(1, br.x - tl.x);
    dst.h = std::max(1, br.y - tl.y);

    SDL_Texture* tex = currentSpriteSheet->getTexture()->getTexture();
    // No color tinting - use original sprite colors for all tiers
    SDL_SetTextureColorMod(tex, 255, 255, 255);
    
    // Check if we need to flip the sprite for enemies with single-direction sprites
    if (usesSpriteFlipping) {
        bool shouldFlip = false;
        if (baseSpriteFacesLeft) {
            // Base sprite faces left, flip when facing right
            shouldFlip = (currentDirection == EnemyDirection::RIGHT);
        } else {
            // Base sprite faces right, flip when facing left
            shouldFlip = (currentDirection == EnemyDirection::LEFT);
        }
        
        if (shouldFlip) {
            SDL_RenderCopyEx(renderer->getSDLRenderer(), tex, &src, &dst, 0.0, nullptr, SDL_FLIP_HORIZONTAL);
        } else {
            SDL_RenderCopy(renderer->getSDLRenderer(), tex, &src, &dst);
        }
    } else {
        SDL_RenderCopy(renderer->getSDLRenderer(), tex, &src, &dst);
    }
    
    // Reset to white to avoid affecting other draws of same texture
    SDL_SetTextureColorMod(tex, 255, 255, 255);

    // Tiny HP bar above head for non-elite (minions/common). Hide on death.
    if (maxHealth > 0 && health > 0 && currentState != EnemyState::DEAD &&
        packRarity != PackRarity::Elite && packRarity != PackRarity::Magic) {
        float ratio = std::max(0.0f, std::min(1.0f, static_cast<float>(health) / static_cast<float>(maxHealth)));
        const int barW = std::max(10, dst.w / 4); // half previous size
        const int barH = 3;
        const int barX = dst.x + (dst.w - barW) / 2;
        const int barY = dst.y - (barH + 3);
        SDL_Rect bg{ barX, barY, barW, barH };
        SDL_Rect fg{ barX, barY, static_cast<int>(barW * ratio), barH };
        SDL_SetRenderDrawBlendMode(renderer->getSDLRenderer(), SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer->getSDLRenderer(), 0, 0, 0, 180);
        SDL_RenderFillRect(renderer->getSDLRenderer(), &bg);
        SDL_SetRenderDrawColor(renderer->getSDLRenderer(), 220, 0, 0, 220);
        SDL_RenderFillRect(renderer->getSDLRenderer(), &fg);
        SDL_SetRenderDrawColor(renderer->getSDLRenderer(), 255, 255, 255, 220);
        SDL_RenderDrawRect(renderer->getSDLRenderer(), &bg);
    }
}

void Enemy::renderProjectiles(Renderer* renderer) const {
    if (!renderer) return;
    for (const auto& p : projectiles) {
        if (p && p->isActive()) {
            p->render(renderer);
        }
    }
}

void Enemy::updateProjectiles(float deltaTime) {
    for (auto& p : projectiles) {
        if (p) p->update(deltaTime);
    }
    projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(), [](const std::unique_ptr<Projectile>& p){ return !p || !p->isActive(); }), projectiles.end());
}

void Enemy::fireProjectileTowards(float targetX, float targetY, AssetManager* assetManager, const std::string& projectileSprite, int frames) {
    if (!assetManager) assetManager = assets;
    float px = x; float py = y;
    float dirX = targetX - px; float dirY = targetY - py;
    ProjectileDirection dir(dirX, dirY); dir.normalize();
    
    if (!projectileSprite.empty() && frames > 0) {
        // Use specific projectile sprite
        projectiles.push_back(std::make_unique<Projectile>(px, py, dir, assetManager, 12, projectileSprite, frames, frames));
    } else {
        // Use default projectile
        projectiles.push_back(std::make_unique<Projectile>(px, py, dir, assetManager, 12));
    }
}


