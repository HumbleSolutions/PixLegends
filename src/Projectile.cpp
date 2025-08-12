#include "Projectile.h"
#include "Renderer.h"
#include "AssetManager.h"
#include <iostream>

Projectile::Projectile(float x, float y,
                       ProjectileDirection direction,
                       AssetManager* assetManager,
                       int damage,
                       const std::string& spritePath,
                       int totalFrames,
                       int framesPerRow,
                       bool rotateByDirection)
    : x(x), y(y), width(32), height(32), speed(PROJECTILE_SPEED), direction(direction),
      currentFrame(0), frameTimer(0.0f), frameDuration(FRAME_DURATION),
      active(true), lifetime(0.0f), maxLifetime(MAX_LIFETIME), damage(damage), rotateByDir(rotateByDirection) {
    
    // Load the projectile sprite sheet
    // AssetManager may be null for enemy-fired projectiles; rely on preloaded cache in that case
    if (assetManager) {
        if (!spritePath.empty()) {
            spriteSheet = assetManager->getSpriteSheet(spritePath);
            if (!spriteSheet) {
                if (totalFrames > 0) spriteSheet = assetManager->loadSpriteSheetAuto(spritePath, totalFrames, framesPerRow);
                else spriteSheet = assetManager->loadSpriteSheetAuto(spritePath, 1, 1);
            }
        } else {
            // Default: wizard projectile
            const std::string path = AssetManager::WIZARD_PATH + std::string("Projectile.png");
            spriteSheet = assetManager->getSpriteSheet(path);
            if (!spriteSheet) spriteSheet = assetManager->loadSpriteSheet(path, 32, 32, 5, 5);
        }
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
    
    // Do not deactivate based on hardcoded screen bounds; world is chunked and camera can move.
    
    // Update animation
    updateAnimation(deltaTime);
}

void Projectile::render(Renderer* renderer) {
    if (!active || !renderer || !spriteSheet) return;
    
    // Get source rectangle for current frame
    SDL_Rect srcRect = spriteSheet->getFrameRect(currentFrame);
    
    // Destination rectangle (no scaling): use sprite frame size if available
    SDL_Rect dstRect;
    if (spriteSheet) {
        SDL_Rect frame = spriteSheet->getFrameRect(currentFrame);
        dstRect = { static_cast<int>(x), static_cast<int>(y), frame.w, frame.h };
    } else {
        dstRect = { static_cast<int>(x), static_cast<int>(y), width, height };
    }
    
    if (rotateByDir) {
        double angleDeg = 0.0;
        if (std::fabs(direction.x) > 0.0001f || std::fabs(direction.y) > 0.0001f) {
            angleDeg = std::atan2(direction.y, direction.x) * 180.0 / M_PI; // 0 deg points right
        }
        // Apply camera and zoom to destination like other render paths
        SDL_Rect adjusted = dstRect;
        int camX=0, camY=0; renderer->getCamera(camX, camY);
        adjusted.x -= camX; adjusted.y -= camY;
        float z = renderer->getZoom();
        if (std::fabs(z - 1.0f) > 0.001f) {
            adjusted.x = static_cast<int>(adjusted.x * z);
            adjusted.y = static_cast<int>(adjusted.y * z);
            adjusted.w = static_cast<int>(adjusted.w * z);
            adjusted.h = static_cast<int>(adjusted.h * z);
        }
        SDL_RenderCopyEx(renderer->getSDLRenderer(), spriteSheet->getTexture()->getTexture(), &srcRect, &adjusted, angleDeg, nullptr, SDL_FLIP_NONE);
    } else {
        renderer->renderTexture(spriteSheet->getTexture()->getTexture(), &srcRect, &dstRect);
    }
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
