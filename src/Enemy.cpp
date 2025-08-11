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
        idleLeftSpriteSheet   = assetManager->loadSpriteSheet(base + "IDLE_LEFT.png",   128, 78, 6, 6);
        idleRightSpriteSheet  = assetManager->loadSpriteSheet(base + "IDLE_RIGHT.png",  128, 78, 6, 6);
        flyingLeftSpriteSheet  = assetManager->loadSpriteSheet(base + "WALK_LEFT.png",  128, 78, 4, 4);
        flyingRightSpriteSheet = assetManager->loadSpriteSheet(base + "WALK_RIGHT.png", 128, 78, 4, 4);
        attackLeftSpriteSheet  = assetManager->loadSpriteSheet(base + "MELEE ATTACK_LEFT.png",  128, 78, 6, 6);
        attackRightSpriteSheet = assetManager->loadSpriteSheet(base + "MELEE ATTACK_RIGHT.png", 128, 78, 6, 6);
        hurtLeftSpriteSheet  = assetManager->loadSpriteSheet(base + "HURT_LEFT.png",  128, 78, 4, 4);
        hurtRightSpriteSheet = assetManager->loadSpriteSheet(base + "HURT_RIGHT.png", 128, 78, 4, 4);
        deathSpriteSheet = assetManager->loadSpriteSheet(base + "DEATH.png", 128, 78, 6, 6);
        packRarity = PackRarity::Elite; // per request: wizard can be elite
        renderScale = 2.0f; // default scale
    } else {
        if (kind == EnemyKind::Demon) {
            // Demon boss assets (existing)
            const std::string base = "assets/Demon Boss 2D Pixel Art/Sprites/without_outline/";
            idleLeftSpriteSheet   = assetManager->loadSpriteSheetAuto(base + "IDLE_LEFT.png",   4, 4);
            idleRightSpriteSheet  = assetManager->loadSpriteSheetAuto(base + "IDLE_RIGHT.png",  4, 4);
            flyingLeftSpriteSheet  = assetManager->loadSpriteSheetAuto(base + "FLYING_LEFT.png", 4, 4);
            flyingRightSpriteSheet = assetManager->loadSpriteSheetAuto(base + "FLYING_RIGHT.png",4, 4);
            attackLeftSpriteSheet  = assetManager->loadSpriteSheetAuto(base + "ATTACK_LEFT.png",  6, 6);
            attackRightSpriteSheet = assetManager->loadSpriteSheetAuto(base + "ATTACK_RIGHT.png", 6, 6);
            hurtLeftSpriteSheet  = assetManager->loadSpriteSheetAuto(base + "HURT_LEFT.png", 3, 3);
            hurtRightSpriteSheet = assetManager->loadSpriteSheetAuto(base + "HURT_RIGHT.png",3, 3);
            deathSpriteSheet = assetManager->loadSpriteSheetAuto(base + "DEATH.png", 10, 10);
            packRarity = PackRarity::Elite; // boss -> elite
            renderScale = 2.2f;
        } else {
            // Goblin minion assets (your new monster)
            const std::string base = "assets/Goblin 2D Pixel Art v1.1/Sprites/without_outline/";
            // The provided files are 115x78 per frame and separate left/right sheets
            idleLeftSpriteSheet   = assetManager->loadSpriteSheet(base + "LEFT_IDLE.png", 115, 78, 3, 3);
            idleRightSpriteSheet  = assetManager->loadSpriteSheet(base + "RIGHT_IDLE.png", 115, 78, 3, 3);
            flyingLeftSpriteSheet  = assetManager->loadSpriteSheet(base + "LEFT_RUN.png", 115, 78, 6, 6);
            flyingRightSpriteSheet = assetManager->loadSpriteSheet(base + "RIGHT_RUN.png", 115, 78, 6, 6);
            attackLeftSpriteSheet  = assetManager->loadSpriteSheet(base + "LEFT_ATTACK1.png", 115, 78, 6, 6);
            attackRightSpriteSheet = assetManager->loadSpriteSheet(base + "RIGHT_ATTACK1.png", 115, 78, 6, 6);
            // Optional second attack set if you want to swap: LEFT_ATTACK2/RIGHT_ATTACK2
            hurtLeftSpriteSheet  = assetManager->loadSpriteSheet(base + "LEFT_HURT.png", 115, 78, 3, 3);
            hurtRightSpriteSheet = assetManager->loadSpriteSheet(base + "RIGHT_HURT.png", 115, 78, 3, 3);
            deathSpriteSheet = assetManager->loadSpriteSheet(base + "LEFT_DEATH.png", 115, 78, 10, 10); // death is symmetric
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
            // Fallback: if idle sheets are missing, use flying (walk) sheets so enemies are always visible when idle
            if (currentDirection == EnemyDirection::LEFT) {
                return idleLeftSpriteSheet ? idleLeftSpriteSheet : (flyingLeftSpriteSheet ? flyingLeftSpriteSheet : attackLeftSpriteSheet);
            } else {
                return idleRightSpriteSheet ? idleRightSpriteSheet : (flyingRightSpriteSheet ? flyingRightSpriteSheet : attackRightSpriteSheet);
            }
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

    // Cooldowns
    if (attackCooldownTimer > 0.0f) attackCooldownTimer -= deltaTime;

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
        // Attack state (looping animation)
        if (currentState != EnemyState::ATTACKING)
            setState(EnemyState::ATTACKING);
    }

    // Ranged behavior for wizard
    if (kind == EnemyKind::Wizard) {
        // Cooldown timer
        if (rangedCooldownTimer > 0.0f) rangedCooldownTimer -= deltaTime;
        // Fire at range if within rangedRange and has cooldown ready
        if (distSq <= rangedRange * rangedRange && rangedCooldownTimer <= 0.0f) {
            fireProjectileTowards(playerX, playerY, assets);
            rangedCooldownTimer = rangedCooldownSeconds;
        }
        updateProjectiles(deltaTime);
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
    // Apply rarity color tint (outline proxy via color mod)
    Uint8 r=255,g=255,b=255;
    switch (packRarity) {
        case PackRarity::Elite:  r=255; g=215; b=0;   break; // gold
        case PackRarity::Magic:  r=80;  g=160; b=255; break; // blue
        case PackRarity::Common: r=255; g=255; b=255; break; // white
        case PackRarity::Trash:  r=180; g=180; b=180; break; // grey
    }
    SDL_SetTextureColorMod(tex, r, g, b);
    SDL_RenderCopy(renderer->getSDLRenderer(), tex, &src, &dst);
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

void Enemy::fireProjectileTowards(float targetX, float targetY, AssetManager* assetManager) {
    if (!assetManager) assetManager = assets;
    float px = x; float py = y;
    float dirX = targetX - px; float dirY = targetY - py;
    ProjectileDirection dir(dirX, dirY); dir.normalize();
    projectiles.push_back(std::make_unique<Projectile>(px, py, dir, assetManager, 12));
}


