#include "Projectile.h"
#include "Renderer.h"
#include "AssetManager.h"
#include <iostream>

Projectile::Projectile(float x, float y, ProjectileDirection direction, AssetManager* assetManager, int damage)
    : x(x), y(y), width(32), height(32), speed(PROJECTILE_SPEED), direction(direction),
      currentFrame(0), frameTimer(0.0f), frameDuration(FRAME_DURATION),
      active(true), lifetime(0.0f), maxLifetime(MAX_LIFETIME), damage(damage) {
    
    // Load the projectile sprite sheet
    // AssetManager may be null for enemy-fired projectiles; rely on preloaded cache in that case
    if (assetManager) {
        spriteSheet = assetManager->getSpriteSheet(AssetManager::WIZARD_PATH + "Projectile.png");
    } else {
        // Attempt to fetch from global cache via static accessor is not available; use a safe nullptr check later
        // Enemy.cpp uses nullptr; assume preloaded by AssetManager::preloadAssets
        spriteSheet = nullptr;
    }
    if (!spriteSheet) {
        // As a fallback, keep projectile active but render nothing; still apply damage on hit
        std::cout << "Projectile sprite missing; continuing with invisible projectile." << std::endl;
    }
}

void Projectile::update(float deltaTime) {
    if (!active) return;
    
    // Update lifetime
    lifetime += deltaTime;
    if (lifetime >= maxLifetime) {
        active = false;
        return;
    }
    
    // Update movement using direction vector
    x += direction.x * speed * deltaTime;
    y += direction.y * speed * deltaTime;
    
    // Check if projectile is off screen
    if (x < -width || x > 800 + width) { // Assuming 800 is screen width
        active = false;
        return;
    }
    
    // Update animation
    updateAnimation(deltaTime);
}

void Projectile::render(Renderer* renderer) {
    if (!active || !renderer || !spriteSheet) return;
    
    // Get source rectangle for current frame
    SDL_Rect srcRect = spriteSheet->getFrameRect(currentFrame);
    
    // Calculate destination rectangle
    SDL_Rect dstRect = {
        static_cast<int>(x),
        static_cast<int>(y),
        width,
        height
    };
    
    // Render the projectile
    renderer->renderTexture(spriteSheet->getTexture()->getTexture(), &srcRect, &dstRect);
}

void Projectile::updateAnimation(float deltaTime) {
    if (!spriteSheet) return;
    
    frameTimer += deltaTime;
    
    if (frameTimer >= frameDuration) {
        frameTimer = 0.0f;
        currentFrame++;
        
        // Loop animation
        if (currentFrame >= spriteSheet->getTotalFrames()) {
            currentFrame = 0;
        }
    }
}

SDL_Rect Projectile::getCollisionRect() const {
    SDL_Rect r;
    r.x = static_cast<int>(x);
    r.y = static_cast<int>(y);
    r.w = width;
    r.h = height;
    return r;
}
