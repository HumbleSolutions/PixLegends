#include "Enemy.h"
#include "AssetManager.h"
#include "Renderer.h"

Enemy::Enemy(float spawnX, float spawnY, AssetManager* assetManager)
    : x(spawnX), y(spawnY), width(128), height(128), moveSpeed(70.0f), health(200),
      currentState(EnemyState::IDLE), currentDirection(EnemyDirection::RIGHT),
      currentSpriteSheet(nullptr), currentFrame(0), frameTimer(0.0f), frameDuration(0.12f),
      currentSpriteSheetFrameWidth(0), currentSpriteSheetFrameHeight(0),
      idleLeftSpriteSheet(nullptr), idleRightSpriteSheet(nullptr),
      flyingLeftSpriteSheet(nullptr), flyingRightSpriteSheet(nullptr),
      attackLeftSpriteSheet(nullptr), attackRightSpriteSheet(nullptr),
      hurtLeftSpriteSheet(nullptr), hurtRightSpriteSheet(nullptr),
      deathSpriteSheet(nullptr),
      aggroRadius(180.0f), attackRange(140.0f), isAggroed(false) {
    loadSprites(assetManager);
    setState(EnemyState::IDLE);
}

void Enemy::loadSprites(AssetManager* assetManager) {
    const std::string base = "assets/Demon Boss 2D Pixel Art/Sprites/without_outline/";

    // Use exact frame counts; auto-detect frame size and columns to avoid glitches
    // Force single-row layouts to prevent partial multi-row sampling
    idleLeftSpriteSheet   = assetManager->loadSpriteSheetAuto(base + "IDLE_LEFT.png",   4, 4);
    idleRightSpriteSheet  = assetManager->loadSpriteSheetAuto(base + "IDLE_RIGHT.png",  4, 4);

    flyingLeftSpriteSheet  = assetManager->loadSpriteSheetAuto(base + "FLYING_LEFT.png", 4, 4);
    flyingRightSpriteSheet = assetManager->loadSpriteSheetAuto(base + "FLYING_RIGHT.png",4, 4);

    attackLeftSpriteSheet  = assetManager->loadSpriteSheetAuto(base + "ATTACK_LEFT.png",  6, 6);
    attackRightSpriteSheet = assetManager->loadSpriteSheetAuto(base + "ATTACK_RIGHT.png", 6, 6);

    hurtLeftSpriteSheet  = assetManager->loadSpriteSheetAuto(base + "HURT_LEFT.png", 3, 3);
    hurtRightSpriteSheet = assetManager->loadSpriteSheetAuto(base + "HURT_RIGHT.png",3, 3);

    deathSpriteSheet = assetManager->loadSpriteSheetAuto(base + "DEATH.png", 10, 10);
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
            return currentDirection == EnemyDirection::LEFT ? idleLeftSpriteSheet : idleRightSpriteSheet;
        case EnemyState::FLYING:
            return currentDirection == EnemyDirection::LEFT ? flyingLeftSpriteSheet : flyingRightSpriteSheet;
        case EnemyState::ATTACKING:
            return currentDirection == EnemyDirection::LEFT ? attackLeftSpriteSheet : attackRightSpriteSheet;
        case EnemyState::HURT:
            return currentDirection == EnemyDirection::LEFT ? hurtLeftSpriteSheet : hurtRightSpriteSheet;
        case EnemyState::DEAD:
            return deathSpriteSheet;
    }
    return nullptr;
}

void Enemy::update(float deltaTime, float playerX, float playerY) {
    if (currentState == EnemyState::DEAD) {
        updateAnimation(deltaTime);
        return;
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
        float vx = (dx / dist) * moveSpeed;
        float vy = (dy / dist) * moveSpeed * 0.6f; // slight slower vertical
        x += vx * deltaTime;
        y += vy * deltaTime;
        if (currentState != EnemyState::FLYING)
            setState(EnemyState::FLYING);
    } else {
        // Attack state (looping animation)
        if (currentState != EnemyState::ATTACKING)
            setState(EnemyState::ATTACKING);
    }

    updateAnimation(deltaTime);
}

void Enemy::updateAnimation(float deltaTime) {
    if (!currentSpriteSheet) return;
    frameTimer += deltaTime;

    float duration = frameDuration;
    if (currentState == EnemyState::ATTACKING) duration = 0.09f;
    if (currentState == EnemyState::DEAD) duration = 0.12f;

    if (frameTimer >= duration) {
        frameTimer = 0.0f;
        currentFrame++;
        if (currentFrame >= currentSpriteSheet->getTotalFrames()) {
            if (currentState == EnemyState::DEAD) {
                currentFrame = currentSpriteSheet->getTotalFrames() - 1; // hold last
            } else {
                currentFrame = 0; // loop
            }
        }
    }
}

void Enemy::render(SDL_Renderer* sdlRenderer, int cameraX, int cameraY) const {
    if (!currentSpriteSheet || !currentSpriteSheet->getTexture() || !currentSpriteSheet->getTexture()->getTexture()) return;

    SDL_Rect src = currentSpriteSheet->getFrameRect(currentFrame);

    // Scale with a conservative factor to avoid clipping when frame sizes vary
    int scale = 2;
    SDL_Rect dst;
    dst.w = src.w * scale;
    dst.h = src.h * scale;
    dst.x = static_cast<int>(x) - cameraX - dst.w / 2;
    dst.y = static_cast<int>(y) - cameraY - dst.h / 2;

    SDL_RenderCopy(sdlRenderer, currentSpriteSheet->getTexture()->getTexture(), &src, &dst);
}


