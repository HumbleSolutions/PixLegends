#pragma once

#include <SDL2/SDL.h>
#include <string>

// Forward declarations
class SpriteSheet;
class AssetManager;

enum class EnemyState {
    IDLE,
    FLYING,
    ATTACKING,
    HURT,
    DEAD
};

enum class EnemyDirection {
    LEFT,
    RIGHT
};

class Enemy {
public:
    Enemy(float spawnX, float spawnY, AssetManager* assetManager);
    ~Enemy() = default;

    void update(float deltaTime, float playerX, float playerY);
    void render(SDL_Renderer* sdlRenderer, int cameraX, int cameraY) const;

    float getX() const { return x; }
    float getY() const { return y; }
    int getWidth() const { return currentSpriteSheet ? currentSpriteSheetFrameWidth : width; }
    int getHeight() const { return currentSpriteSheet ? currentSpriteSheetFrameHeight : height; }

    bool isDead() const { return currentState == EnemyState::DEAD; }

private:
    // Core
    float x;
    float y;
    int width;
    int height;
    float moveSpeed;
    int health;

    // State
    EnemyState currentState;
    EnemyDirection currentDirection;

    // Animation
    SpriteSheet* currentSpriteSheet;
    int currentFrame;
    float frameTimer;
    float frameDuration;
    int currentSpriteSheetFrameWidth;
    int currentSpriteSheetFrameHeight;

    // Sprites
    SpriteSheet* idleLeftSpriteSheet;
    SpriteSheet* idleRightSpriteSheet;
    SpriteSheet* flyingLeftSpriteSheet;
    SpriteSheet* flyingRightSpriteSheet;
    SpriteSheet* attackLeftSpriteSheet;
    SpriteSheet* attackRightSpriteSheet;
    SpriteSheet* hurtLeftSpriteSheet;
    SpriteSheet* hurtRightSpriteSheet;
    SpriteSheet* deathSpriteSheet;

    // Helpers
    void loadSprites(AssetManager* assetManager);
    SpriteSheet* pickSpriteSheetForState(EnemyState state) const;
    void setState(EnemyState newState);
    void setDirection(EnemyDirection newDirection);
    void updateAnimation(float deltaTime);

    // Combat/behavior
    float aggroRadius;   // pixels
    float attackRange;   // pixels
    bool isAggroed = false;
};


