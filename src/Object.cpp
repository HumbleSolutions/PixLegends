#include "Object.h"
#include "AssetManager.h"
#include <iostream>
#include <cmath>

Object::Object(ObjectType type, int xPos, int yPos, const std::string& texturePath)
    : type(type), x(xPos), y(yPos), texturePath(texturePath), texture(nullptr), spriteSheet(nullptr), assetManager(nullptr),
      interactable(false), walkable(true), visible(true),
      animationTime(0.0f), currentFrame(0), totalFrames(1), frameDuration(0.1f) {
    
    // Set object-specific properties based on type
    switch (type) {
        case ObjectType::CHEST_OPENED:
            interactable = false;
            walkable = true;
            break;
        case ObjectType::CHEST_UNOPENED:
            interactable = true;
            walkable = false;
            break;
        case ObjectType::CLAY_POT:
            interactable = true;
            walkable = false;
            break;
        case ObjectType::FLAG:
            interactable = false;
            walkable = true;
            break;
        case ObjectType::WOOD_CRATE:
        case ObjectType::STEEL_CRATE:
            interactable = true;
            walkable = false;
            break;
        case ObjectType::WOOD_FENCE:
        case ObjectType::WOOD_FENCE_BROKEN:
            interactable = false;
            walkable = false;
            break;
        case ObjectType::WOOD_SIGN:
            interactable = true;
            walkable = true;
            break;
        case ObjectType::BONFIRE:
            interactable = false;
            walkable = false;
            // Bonfire has 6 animation frames
            totalFrames = 6;
            frameDuration = 0.2f;
            break;
    }
}

void Object::update(float deltaTime) {
    // Update animation if this object has multiple frames
    if (totalFrames > 1) {
        animationTime += deltaTime;
        if (animationTime >= frameDuration) {
            animationTime = 0.0f;
            currentFrame = (currentFrame + 1) % totalFrames;
        }
    }
}

void Object::render(SDL_Renderer* renderer, int cameraX, int cameraY, int tileSize) {
    if (!visible) {
        return;
    }
    
    // Calculate screen position
    int screenX = (x * tileSize) - cameraX;
    int screenY = (y * tileSize) - cameraY;
    
    SDL_Rect srcRect = {0, 0, 0, 0};
    SDL_Texture* sdlTexture = nullptr;
    
    // Use spritesheet if available, otherwise use regular texture
    if (spriteSheet) {
        // Use spritesheet for animation
        srcRect = spriteSheet->getFrameRect(currentFrame);
        sdlTexture = spriteSheet->getTexture()->getTexture();
    } else if (texture) {
        // Use regular texture (backward compatibility)
        int textureWidth = texture->getWidth();
        int textureHeight = texture->getHeight();
        
        srcRect = {0, 0, textureWidth, textureHeight};
        if (totalFrames > 1) {
            int frameWidth = textureWidth / totalFrames;
            srcRect.x = currentFrame * frameWidth;
            srcRect.w = frameWidth;
        }
        sdlTexture = texture->getTexture();
    }
    
    // If we have a valid texture, render it
    if (sdlTexture) {
        // Destination rectangle - use actual frame dimensions for spritesheets
        SDL_Rect dstRect;
        if (spriteSheet) {
            // Use actual frame dimensions for spritesheets
            dstRect = {
                screenX,
                screenY,
                spriteSheet->getFrameWidth(),
                spriteSheet->getFrameHeight()
            };
        } else {
            // Use tile size for regular textures
            dstRect = {
                screenX,
                screenY,
                tileSize,
                tileSize
            };
        }
        
        // Render the texture
        SDL_RenderCopy(renderer, sdlTexture, &srcRect, &dstRect);
    } else {
        // Fallback: render a colored rectangle based on object type
        SDL_Color color;
        switch (type) {
            case ObjectType::CHEST_UNOPENED:
            case ObjectType::CHEST_OPENED:
                color = {139, 69, 19, 255}; // Brown
                break;
            case ObjectType::CLAY_POT:
                color = {160, 82, 45, 255}; // Saddle brown
                break;
            case ObjectType::FLAG:
                color = {255, 0, 0, 255}; // Red
                break;
            case ObjectType::WOOD_CRATE:
            case ObjectType::STEEL_CRATE:
                color = {101, 67, 33, 255}; // Dark brown
                break;
            case ObjectType::WOOD_SIGN:
                color = {139, 69, 19, 255}; // Brown
                break;
            case ObjectType::BONFIRE:
                color = {255, 69, 0, 255}; // Orange red
                break;
            default:
                color = {128, 128, 128, 255}; // Gray
                break;
        }
        
        // Render colored rectangle as fallback
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_Rect fallbackRect = {screenX, screenY, tileSize, tileSize};
        SDL_RenderFillRect(renderer, &fallbackRect);
    }
}

void Object::interact() {
    switch (type) {
        case ObjectType::CHEST_UNOPENED:
            std::cout << "Opening chest..." << std::endl;
            if (hasLoot()) {
                std::cout << "Found loot in chest!" << std::endl;
            } else {
                std::cout << "The chest is empty." << std::endl;
            }
            // Change to opened chest
            type = ObjectType::CHEST_OPENED;
            interactable = false;
            // Change texture to opened chest
            changeTexture("assets/Textures/Objects/chest_opened.png");
            break;
        case ObjectType::CLAY_POT:
            std::cout << "Breaking clay pot..." << std::endl;
            if (hasLoot()) {
                std::cout << "Found loot in pot!" << std::endl;
                // TODO: Add breaking animation and remove object
            } else {
                std::cout << "The pot shatters into pieces." << std::endl;
            }
            break;
        case ObjectType::WOOD_CRATE:
        case ObjectType::STEEL_CRATE:
            std::cout << "Opening crate..." << std::endl;
            if (hasLoot()) {
                std::cout << "Found loot in crate!" << std::endl;
                // TODO: Add opening animation and loot
            } else {
                std::cout << "The crate is empty." << std::endl;
            }
            break;
        case ObjectType::WOOD_SIGN:
            std::cout << "Reading sign..." << std::endl;
            // TODO: Show sign text
            break;
        default:
            std::cout << "Interacting with object..." << std::endl;
            break;
    }
}

void Object::addLoot(const Loot& newLoot) {
    loot.push_back(newLoot);
}

std::vector<Loot> Object::getLoot() {
    std::vector<Loot> currentLoot = loot;
    clearLoot();
    return currentLoot;
}

void Object::clearLoot() {
    loot.clear();
}

bool Object::isInInteractionRange(int playerX, int playerY, int tileSize) const {
    // Convert player position to tile coordinates using integer division
    int playerTileX = playerX / tileSize;
    int playerTileY = playerY / tileSize;
    
    // Check if object is within 1 tile distance (adjacent or same tile)
    int distanceX = abs(playerTileX - x);
    int distanceY = abs(playerTileY - y);
    
    bool inRange = distanceX <= 1 && distanceY <= 1;
    
    return inRange;
}

void Object::changeTexture(const std::string& newTexturePath) {
    if (assetManager) {
        Texture* newTexture = assetManager->getTexture(newTexturePath);
        if (newTexture) {
            texture = newTexture;
            texturePath = newTexturePath;
        } else {
            std::cerr << "Failed to load texture: " << newTexturePath << std::endl;
        }
    } else {
        std::cerr << "AssetManager not set for object" << std::endl;
    }
}

std::string Object::getInteractionPrompt() const {
    switch (type) {
        case ObjectType::CHEST_UNOPENED:
            return "Press E to open chest";
        case ObjectType::CLAY_POT:
            return "Press E to break pot";
        case ObjectType::WOOD_CRATE:
            return "Press E to open crate";
        case ObjectType::STEEL_CRATE:
            return "Press E to open steel crate";
        case ObjectType::WOOD_SIGN:
            return "Press E to read sign";
        case ObjectType::BONFIRE:
            return "Press E to warm up";
        default:
            return "Press E to interact";
    }
}
