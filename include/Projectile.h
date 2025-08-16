#pragma once

#include <SDL.h>
#include <memory>
#include <cmath>
#include <string>

// Forward declarations
class Renderer;
class SpriteSheet;
class AssetManager;

// Projectile direction using a 2D vector
struct ProjectileDirection {
    float x, y;
    
    ProjectileDirection(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}
    
    // Normalize the direction vector
    void normalize() {
        float length = std::sqrt(x * x + y * y);
        if (length > 0.0f) {
            x /= length;
            y /= length;
        }
    }
};

class Projectile {
public:
    // Optional sprite override: if spritePath is provided, that sheet will be used with the given frame data.
    // Set rotateByDirection=true to rotate the sprite to match flight direction (e.g., arrows).
    Projectile(float x, float y,
              ProjectileDirection direction,
              AssetManager* assetManager,
              int damage,
              const std::string& spritePath = std::string(),
              int totalFrames = 0,
              int framesPerRow = 0,
              bool rotateByDirection = false);
    ~Projectile() = default;

    // Core update and render
    void update(float deltaTime);
    void render(Renderer* renderer);
    
    // Getters
    float getX() const { return x; }
    float getY() const { return y; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    bool isActive() const { return active; }
    int getDamage() const { return damage; }
    void deactivate() { active = false; }
    SDL_Rect getCollisionRect() const;
    
    // Animation
    void updateAnimation(float deltaTime);

private:
    // Position and size
    float x, y;
    int width, height;
    
    // Movement
    float speed;
    ProjectileDirection direction;
    
    // Animation
    SpriteSheet* spriteSheet;
    int currentFrame;
    float frameTimer;
    float frameDuration;
    bool rotateByDir;
    
    // State
    bool active;
    float lifetime;
    float maxLifetime;
    int damage;
    
    // Constants
    static constexpr float PROJECTILE_SPEED = 300.0f;
    static constexpr float FRAME_DURATION = 0.1f;
    static constexpr float MAX_LIFETIME = 3.0f; // 3 seconds
};
