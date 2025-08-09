#pragma once

#include <SDL2/SDL.h>
#include <memory>
#include <cmath>

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
    Projectile(float x, float y, ProjectileDirection direction, AssetManager* assetManager, int damage);
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
