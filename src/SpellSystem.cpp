#include "SpellSystem.h"
#include "Game.h"
#include "Player.h"
#include "Renderer.h"
#include "Enemy.h"
#include "InputManager.h"
#include "ItemSystem.h"
#include "AssetManager.h"
#include <cmath>
#include <algorithm>
#include <iostream>

SpellSystem::SpellSystem(Game* game, Player* player) 
    : game(game), player(player), assetManager(game->getAssetManager()),
      activeElement(SpellElement::NONE), currentEnchantLevel(0), isChanneling(false), 
      channelTimer(0.0f), channelDuration(0.0f), channelTargetX(0.0f), channelTargetY(0.0f) {
    initializeSpellDatabase();
    initializePassives();
}

void SpellSystem::initializeSpellDatabase() {
    // Fire Spells
    spellDatabase[SpellType::FIRE_BOLT] = {
        SpellType::FIRE_BOLT, "Fire Bolt", 0, 2.0f, 10, 50, 400.0f, false, 0.0f,
        "Basic fire projectile that deals fire damage"
    };
    
    spellDatabase[SpellType::FLAME_WAVE] = {
        SpellType::FLAME_WAVE, "Flame Wave", 10, 8.0f, 30, 100, 200.0f, false, 0.5f,
        "Unleash a cone of fire in front of you"
    };
    
    spellDatabase[SpellType::METEOR_STRIKE] = {
        SpellType::METEOR_STRIKE, "Meteor Strike", 20, 15.0f, 50, 250, 300.0f, false, 0.0f,
        "Channel to call down meteors at target location"
    };
    
    spellDatabase[SpellType::DRAGONS_BREATH] = {
        SpellType::DRAGONS_BREATH, "Dragon's Breath", 30, 20.0f, 5, 20, 250.0f, true, 3.0f,
        "Channel a continuous stream of fire"
    };
    
    
    // Fire Passive Abilities
    spellDatabase[SpellType::FIRE_MASTERY] = {
        SpellType::FIRE_MASTERY, "Fire Mastery", 5, 0.0f, 0, 0, 0.0f, false, 0.0f,
        "Passive: +10% fire damage and +5 mana regeneration"
    };
    
    spellDatabase[SpellType::BURNING_AURA] = {
        SpellType::BURNING_AURA, "Burning Aura", 15, 0.0f, 0, 0, 0.0f, false, 0.0f,
        "Passive: Enemies near you take fire damage over time"
    };
    
    spellDatabase[SpellType::INFERNO_LORD] = {
        SpellType::INFERNO_LORD, "Inferno Lord", 25, 0.0f, 0, 0, 0.0f, false, 0.0f,
        "Passive: +25% fire damage, +10% crit chance, immunity to fire"
    };
    
    // Water Spells
    spellDatabase[SpellType::ICE_SHARD] = {
        SpellType::ICE_SHARD, "Ice Shard", 0, 2.0f, 10, 40, 400.0f, false, 0.0f,
        "Projectile that slows enemies on hit"
    };
    
    spellDatabase[SpellType::FROZEN_GROUND] = {
        SpellType::FROZEN_GROUND, "Frozen Ground", 10, 10.0f, 25, 0, 150.0f, false, 5.0f,
        "Create slowing ice patches on the ground"
    };
    
    spellDatabase[SpellType::BLIZZARD] = {
        SpellType::BLIZZARD, "Blizzard", 20, 18.0f, 60, 15, 250.0f, false, 4.0f,
        "AoE storm that slows and damages enemies"
    };
    
    spellDatabase[SpellType::ABSOLUTE_ZERO] = {
        SpellType::ABSOLUTE_ZERO, "Absolute Zero", 30, 30.0f, 100, 0, 500.0f, false, 3.0f,
        "Freeze all visible enemies for 3 seconds"
    };
    
    // Poison Spells
    spellDatabase[SpellType::TOXIC_DART] = {
        SpellType::TOXIC_DART, "Toxic Dart", 0, 1.5f, 8, 30, 350.0f, false, 0.0f,
        "Fast projectile that applies poison stack"
    };
    
    spellDatabase[SpellType::POISON_CLOUD] = {
        SpellType::POISON_CLOUD, "Poison Cloud", 10, 12.0f, 35, 10, 200.0f, false, 10.0f,
        "Create a stationary poison cloud"
    };
    
    spellDatabase[SpellType::PLAGUE_BOMB] = {
        SpellType::PLAGUE_BOMB, "Plague Bomb", 20, 14.0f, 45, 80, 300.0f, false, 2.0f,
        "Thrown bomb that explodes into poison"
    };
    
    spellDatabase[SpellType::DEATHS_EMBRACE] = {
        SpellType::DEATHS_EMBRACE, "Death's Embrace", 30, 25.0f, 80, 500, 400.0f, false, 5.0f,
        "Mark enemy for death - 5x poison damage"
    };
}

void SpellSystem::initializePassives() {
    // Fire Passives
    firePassives.push_back({"Burning Strike", 5, "Melee attacks apply 2-second burn", false, 2.0f});
    firePassives.push_back({"Molten Blood", 15, "+20% attack speed when below 50% HP", false, 0.0f, 0.2f});
    firePassives.push_back({"Inferno Aura", 25, "Constant damage to nearby enemies", false, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 100.0f, 5.0f});
    
    // Water Passives
    waterPassives.push_back({"Frost Shield", 5, "10% damage reduction", false, 0.0f, 0.0f, 0.1f});
    waterPassives.push_back({"Life Stream", 15, "2% HP regeneration per second", false, 0.0f, 0.0f, 0.0f, 0.02f});
    waterPassives.push_back({"Permafrost", 25, "30% chance to freeze enemies on hit", false, 0.0f, 0.0f, 0.0f, 0.0f, 0.3f});
    
    // Poison Passives
    poisonPassives.push_back({"Venomous Strikes", 5, "All attacks apply poison", false});
    poisonPassives.push_back({"Toxic Blood", 15, "Enemies take damage when hitting you", false});
    poisonPassives.push_back({"Pestilence", 25, "Poison spreads between enemies", false, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f});
}

void SpellSystem::update(float deltaTime) {
    updateActiveSpellTree();
    updateCooldowns(deltaTime);
    updatePassives();
    updateActiveSpells(deltaTime);
    
    if (isChanneling) {
        channelTimer += deltaTime;
        
        // Check if channeling is complete
        if (channelTimer >= channelDuration) {
            stopChanneling();
        } else if (channeledSpell == SpellType::DRAGONS_BREATH) {
            // Continuous damage while channeling - but only create effects periodically
            static float breathEffectTimer = 0.0f;
            breathEffectTimer += deltaTime;
            
            if (breathEffectTimer >= 0.15f) { // Create effects every 150ms instead of every frame
                breathEffectTimer = 0.0f;
                
                float angleRad = 0.0f;
                switch (player->getFacingDirection()) {
                    case Direction::RIGHT: angleRad = 0.0f; break;
                    case Direction::LEFT: angleRad = static_cast<float>(M_PI); break;
                    case Direction::UP: angleRad = static_cast<float>(-M_PI/2); break;
                    case Direction::DOWN: angleRad = static_cast<float>(M_PI/2); break;
                }
                
                // Create fire particles in cone with spatial offset
                float playerCenterX = player->getX() + player->getWidth()/2;
                float playerCenterY = player->getY() + player->getHeight()/2;
                
                for (int i = 0; i < 3; i++) {
                    float spread = (i - 1) * 0.3f;
                    
                    // Add spatial offset to prevent all flames spawning at same position
                    float sideOffset = (i - 1) * 12.0f; // 12 pixels apart
                    float spawnX = playerCenterX + cos(angleRad + M_PI/2) * sideOffset;
                    float spawnY = playerCenterY + sin(angleRad + M_PI/2) * sideOffset;
                    
                    ActiveSpell flame;
                    flame.type = SpellType::DRAGONS_BREATH;
                    flame.x = spawnX;
                    flame.y = spawnY;
                    flame.velocityX = cos(angleRad + spread) * 300.0f;
                    flame.velocityY = sin(angleRad + spread) * 300.0f;
                    flame.lifetime = 0.0f;
                    flame.maxLifetime = 0.8f;
                    flame.damage = spellDatabase[SpellType::DRAGONS_BREATH].damage;
                    flame.active = true;
                    flame.radius = 20.0f;
                    
                    // Animation setup
                    flame.currentFrame = 0;
                    flame.animationTimer = 0.0f;
                    flame.animationSpeed = 20.0f; // 20 FPS for breath
                    
                    activeSpells.push_back(flame);
                }
            }
        }
    }
    
    // Apply passive aura effects
    if (hasInfernoAura()) {
        applyInfernoAura(deltaTime);
    }
}

void SpellSystem::updateActiveSpellTree() {
    // Check player's weapon enchantment
    const auto& weapon = player->getEquipment(Player::EquipmentSlot::SWORD);
    
    SpellElement newElement = SpellElement::NONE;
    int enchantLevel = 0;
    
    if (weapon.fire > 0) {
        newElement = SpellElement::FIRE;
        enchantLevel = weapon.fire;
    } else if (weapon.ice > 0) {
        newElement = SpellElement::WATER;
        enchantLevel = weapon.ice;
    } else if (weapon.poison > 0) {
        newElement = SpellElement::POISON;
        enchantLevel = weapon.poison;
    }
    
    if (newElement != activeElement || enchantLevel != currentEnchantLevel) {
        activeElement = newElement;
        currentEnchantLevel = enchantLevel;
        
        // Clear cooldowns when switching elements
        if (newElement != activeElement) {
            cooldowns.clear();
        }
    }
}

void SpellSystem::updateCooldowns(float deltaTime) {
    for (auto& [spell, cooldown] : cooldowns) {
        if (cooldown > 0) {
            cooldown -= deltaTime;
            if (cooldown < 0) cooldown = 0;
        }
    }
}

void SpellSystem::updatePassives() {
    // Update fire passives
    for (auto& passive : firePassives) {
        passive.active = (activeElement == SpellElement::FIRE && currentEnchantLevel >= passive.requiredEnchantLevel);
    }
    
    // Update water passives
    for (auto& passive : waterPassives) {
        passive.active = (activeElement == SpellElement::WATER && currentEnchantLevel >= passive.requiredEnchantLevel);
    }
    
    // Update poison passives
    for (auto& passive : poisonPassives) {
        passive.active = (activeElement == SpellElement::POISON && currentEnchantLevel >= passive.requiredEnchantLevel);
    }
}

void SpellSystem::updateActiveSpells(float deltaTime) {
    auto& enemies = game->getEnemies();
    
    for (auto it = activeSpells.begin(); it != activeSpells.end();) {
        if (!it->active) {
            it = activeSpells.erase(it);
            continue;
        }
        
        it->lifetime += deltaTime;
        
        // Update animation
        it->animationTimer += deltaTime;
        if (it->animationTimer >= 1.0f / it->animationSpeed) {
            it->currentFrame++;
            it->animationTimer = 0.0f;
        }
        
        // Update position for projectiles and effects
        if (it->type == SpellType::FIRE_BOLT || it->type == SpellType::ICE_SHARD || 
            it->type == SpellType::TOXIC_DART || it->type == SpellType::DRAGONS_BREATH ||
            it->type == SpellType::FLAME_WAVE || it->type == SpellType::METEOR_STRIKE ||
            it->type == SpellType::SMOKE_EFFECT) {
            it->x += it->velocityX * deltaTime;
            it->y += it->velocityY * deltaTime;
        }
        
        // Check collisions with enemies (with enhanced safety checks)
        for (auto& enemy : enemies) {
            // Enhanced null check and validity check
            if (!enemy || enemy.get() == nullptr) {
                continue; // Skip null enemies
            }
            
            // Check if enemy is in a valid state (not dead or being destroyed)
            if (enemy->isDead()) {
                continue; // Skip dead enemies
            }
            
            try {
                SDL_Rect enemyRect = {
                    static_cast<int>(enemy->getX()),
                    static_cast<int>(enemy->getY()),
                    static_cast<int>(enemy->getWidth()),
                    static_cast<int>(enemy->getHeight())
                };
                
                SDL_Rect spellRect = {
                    static_cast<int>(it->x - it->radius),
                    static_cast<int>(it->y - it->radius),
                    static_cast<int>(it->radius * 2),
                    static_cast<int>(it->radius * 2)
                };
                
                if (SDL_HasIntersection(&enemyRect, &spellRect)) {
                    // Triple-check enemy validity before damage application
                    if (!enemy || enemy.get() == nullptr || enemy->isDead()) {
                        continue; // Enemy became invalid during collision check
                    }
                    
                    // Apply damage based on spell type
                    if (it->type == SpellType::FIRE_BOLT || it->type == SpellType::FLAME_WAVE || 
                        it->type == SpellType::METEOR_STRIKE || it->type == SpellType::DRAGONS_BREATH) {
                        enemy->takeDamage(it->damage); // Fire damage
                    } else if (it->type == SpellType::ICE_SHARD || it->type == SpellType::FROZEN_GROUND || 
                               it->type == SpellType::BLIZZARD) {
                        enemy->takeDamage(it->damage);
                        // TODO: Apply slow effect
                    } else if (it->type == SpellType::TOXIC_DART || it->type == SpellType::POISON_CLOUD || 
                               it->type == SpellType::PLAGUE_BOMB) {
                        enemy->takeDamage(it->damage);
                        // TODO: Apply poison DoT
                    }
                    
                    // Projectiles disappear on hit with explosion effects
                    if (it->type == SpellType::FIRE_BOLT || it->type == SpellType::ICE_SHARD || 
                        it->type == SpellType::TOXIC_DART) {
                        // Create explosion effect on impact for fire spells
                        if (it->type == SpellType::FIRE_BOLT) {
                            createExplosionEffect(it->x, it->y);
                        }
                        it->active = false;
                    }
                }
            } catch (...) {
                // Catch any access violations or other exceptions during enemy interaction
                std::cout << "Error: Exception during spell-enemy collision check - enemy may be invalid" << std::endl;
                continue; // Skip this enemy and continue with others
            }
        }
        
        // Check lifetime
        if (it->lifetime >= it->maxLifetime) {
            // Create explosion effect for fire spells before marking inactive
            if (it->type == SpellType::FIRE_BOLT || it->type == SpellType::FLAME_WAVE || 
                it->type == SpellType::METEOR_STRIKE || it->type == SpellType::DRAGONS_BREATH) {
                createExplosionEffect(it->x, it->y);
            }
            it->active = false;
        }
        
        // Check if out of bounds
        if (it->x < -500 || it->x > game->getWorldWidth() + 500 ||
            it->y < -500 || it->y > game->getWorldHeight() + 500) {
            it->active = false;
        }
        
        if (it->active) {
            ++it;
        } else {
            it = activeSpells.erase(it);
        }
    }
}

void SpellSystem::render(Renderer* renderer) {
    renderActiveSpells(renderer);
}

void SpellSystem::renderActiveSpells(Renderer* renderer) {
    for (const auto& spell : activeSpells) {
        renderSpellEffects(renderer, spell);
    }
}

void SpellSystem::renderSpellEffects(Renderer* renderer, const ActiveSpell& spell) {
    if (!renderer) {
        std::cout << "Error: Renderer is null in renderSpellEffects" << std::endl;
        return;
    }
    
    if (!assetManager) {
        std::cout << "Warning: AssetManager is null in renderSpellEffects" << std::endl;
        renderSpellFallback(renderer, spell);
        return;
    }
    
    if (!spell.active) {
        return; // Don't render inactive spells
    }
    
    SpriteSheet* spriteSheet = nullptr;
    std::string spellPath;
    
    // Map spell types to their animation sprite sheets
    switch (spell.type) {
        case SpellType::FIRE_BOLT:
            spellPath = "assets/Textures/Spells/firebolt.png";
            break;
        case SpellType::FLAME_WAVE:
            spellPath = "assets/Textures/Spells/flame_wave_new.png";
            break;
        case SpellType::METEOR_STRIKE:
            spellPath = "assets/Textures/Spells/meteor shower-red.png";
            break;
        case SpellType::DRAGONS_BREATH:
            spellPath = "assets/Textures/Spells/dragon_breath.png";
            break;
        case SpellType::EXPLOSION_EFFECT:
            // Use the dynamically chosen explosion sprite path
            spellPath = spell.spritePath.empty() ? "assets/Textures/Spells/explosion.png" : spell.spritePath;
            break;
        case SpellType::PARTICLE_EFFECT:
            spellPath = "assets/Textures/Spells/fire_partical.png";
            break;
        case SpellType::SMOKE_EFFECT:
            spellPath = "assets/Textures/Spells/smoke cloud.png";
            break;
        case SpellType::ICE_SHARD:
        case SpellType::FROZEN_GROUND:
        case SpellType::BLIZZARD:
        case SpellType::ABSOLUTE_ZERO:
        case SpellType::TOXIC_DART:
        case SpellType::POISON_CLOUD:
        case SpellType::PLAGUE_BOMB:
        case SpellType::DEATHS_EMBRACE:
            // Fall back to colored rectangles for non-fire spells for now
            renderSpellFallback(renderer, spell);
            return;
        default:
            return;
    }
    
    spriteSheet = assetManager->getSpriteSheet(spellPath);
    if (!spriteSheet) {
        // Fall back to colored rectangle if sprite not found
        std::cout << "Warning: Sprite sheet not found for path: " << spellPath << std::endl;
        renderSpellFallback(renderer, spell);
        return;
    }
    
    // Calculate current frame (with wraparound and safety checks)
    int totalFrames = spriteSheet->getTotalFrames();
    if (totalFrames <= 0) {
        std::cout << "Warning: Invalid total frames for sprite: " << spellPath << std::endl;
        renderSpellFallback(renderer, spell);
        return;
    }
    int frameIndex = (spell.currentFrame >= 0) ? (spell.currentFrame % totalFrames) : 0;
    
    // Get the frame rectangle from the sprite sheet
    SDL_Rect srcRect = spriteSheet->getFrameRect(frameIndex);
    
    // Calculate destination rectangle (centered on spell position)
    int spriteWidth = srcRect.w;
    int spriteHeight = srcRect.h;
    
    // Scale based on spell type
    float scale = 1.0f;
    switch (spell.type) {
        case SpellType::FIRE_BOLT:
            scale = 0.6f; // Smaller for fast projectiles
            break;
        case SpellType::FLAME_WAVE:
            scale = 0.5f; // Reduced by half as requested
            break;
        case SpellType::METEOR_STRIKE:
            scale = 1.5f; // Large for meteors
            break;
        case SpellType::DRAGONS_BREATH:
            scale = 1.0f; // Normal size for breath
            break;
        case SpellType::EXPLOSION_EFFECT:
            scale = 1.2f; // Slightly larger explosions
            break;
        case SpellType::PARTICLE_EFFECT:
            scale = 0.8f; // Smaller particles
            break;
        case SpellType::SMOKE_EFFECT:
            scale = 1.0f; // Normal smoke size
            break;
    }
    
    spriteWidth = static_cast<int>(spriteWidth * scale);
    spriteHeight = static_cast<int>(spriteHeight * scale);
    
    SDL_Rect destRect = {
        static_cast<int>(spell.x - spriteWidth/2),
        static_cast<int>(spell.y - spriteHeight/2),
        spriteWidth,
        spriteHeight
    };
    
    // Render the animated sprite
    SDL_RenderCopy(renderer->getSDLRenderer(), spriteSheet->getTexture()->getTexture(), &srcRect, &destRect);
}

void SpellSystem::renderSpellFallback(Renderer* renderer, const ActiveSpell& spell) {
    if (!renderer || !renderer->getSDLRenderer()) {
        return; // Can't render without valid renderer
    }
    
    // Fallback colored rectangles for spells without animations
    SDL_Color color = {255, 0, 0, 200}; // Default red
    
    switch (spell.type) {
        case SpellType::ICE_SHARD:
        case SpellType::FROZEN_GROUND:
        case SpellType::BLIZZARD:
        case SpellType::ABSOLUTE_ZERO:
            color = {100, 200, 255, 200}; // Light blue for ice
            break;
        case SpellType::TOXIC_DART:
        case SpellType::POISON_CLOUD:
        case SpellType::PLAGUE_BOMB:
        case SpellType::DEATHS_EMBRACE:
            color = {100, 255, 0, 200}; // Green for poison
            break;
        default:
            color = {255, 100, 0, 200}; // Orange for fire fallback
            break;
    }
    
    SDL_Rect rect = {
        static_cast<int>(spell.x - spell.radius),
        static_cast<int>(spell.y - spell.radius),
        static_cast<int>(spell.radius * 2),
        static_cast<int>(spell.radius * 2)
    };
    SDL_SetRenderDrawColor(renderer->getSDLRenderer(), color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer->getSDLRenderer(), &rect);
}

void SpellSystem::castSpell(int spellSlot) {
    std::cout << "DEBUG: castSpell called with slot " << spellSlot << std::endl;
    std::vector<SpellType> available = getAvailableSpells();
    std::cout << "DEBUG: Available spells count: " << available.size() << std::endl;
    
    if (spellSlot >= 0 && spellSlot < available.size()) {
        SpellType spell = available[spellSlot];
        std::cout << "DEBUG: Casting spell type: " << static_cast<int>(spell) << std::endl;
        
        if (!isSpellReady(spell)) {
            std::cout << "DEBUG: Spell not ready (cooldown)" << std::endl;
            return;
        }
        if (player->getMana() < getSpellManaCost(spell)) {
            std::cout << "DEBUG: Not enough mana" << std::endl;
            return;
        }
        
        // Get mouse position for targeted spells
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        float worldX = mouseX + game->getCameraX();
        float worldY = mouseY + game->getCameraY();
        std::cout << "DEBUG: Target position: " << worldX << ", " << worldY << std::endl;
        
        castSpellAtPosition(spell, worldX, worldY);
        std::cout << "DEBUG: castSpellAtPosition completed" << std::endl;
    } else {
        std::cout << "DEBUG: Invalid spell slot " << spellSlot << " (available: " << available.size() << ")" << std::endl;
    }
}

void SpellSystem::castSpellAtPosition(SpellType spell, float targetX, float targetY) {
    if (!player) {
        std::cout << "Error: Player is null in castSpellAtPosition" << std::endl;
        return;
    }
    
    if (!game) {
        std::cout << "Error: Game is null in castSpellAtPosition" << std::endl;
        return;
    }
    
    if (!isSpellReady(spell)) return;
    
    auto it = spellDatabase.find(spell);
    if (it == spellDatabase.end()) {
        std::cout << "Error: Spell not found in database: " << static_cast<int>(spell) << std::endl;
        return;
    }
    
    const auto& spellData = it->second;
    if (player->getMana() < spellData.manaCost) return;
    
    player->useMana(spellData.manaCost);
    cooldowns[spell] = spellData.cooldown;
    
    switch (spell) {
        case SpellType::FIRE_BOLT:
            castFireBolt(targetX, targetY);
            break;
        case SpellType::FLAME_WAVE:
            castFlameWave();
            break;
        case SpellType::METEOR_STRIKE:
            castMeteorStrike(targetX, targetY);
            break;
        case SpellType::DRAGONS_BREATH:
            startDragonsBreath();
            break;
        case SpellType::ICE_SHARD:
            castIceShard(targetX, targetY);
            break;
        case SpellType::FROZEN_GROUND:
            castFrozenGround(targetX, targetY);
            break;
        case SpellType::BLIZZARD:
            castBlizzard(targetX, targetY);
            break;
        case SpellType::ABSOLUTE_ZERO:
            castAbsoluteZero();
            break;
        case SpellType::TOXIC_DART:
            castToxicDart(targetX, targetY);
            break;
        case SpellType::POISON_CLOUD:
            castPoisonCloud(targetX, targetY);
            break;
        case SpellType::PLAGUE_BOMB:
            castPlagueBomb(targetX, targetY);
            break;
        case SpellType::DEATHS_EMBRACE:
            castDeathsEmbrace(targetX, targetY);
            break;
    }
}

void SpellSystem::castFireBolt(float targetX, float targetY) {
    // Start from player center
    float playerCenterX = player->getX() + player->getWidth() / 2;
    float playerCenterY = player->getY() + player->getHeight() / 2;
    
    ActiveSpell spell = createSafeSpell(SpellType::FIRE_BOLT, playerCenterX, playerCenterY);
    spell.targetX = targetX;
    spell.targetY = targetY;
    
    // Calculate direction toward mouse/target
    float dx = targetX - playerCenterX;
    float dy = targetY - playerCenterY;
    float dist = sqrt(dx*dx + dy*dy);
    
    // Fast projectile speed - add safety check for division by zero
    if (dist > 0.1f) {
        spell.velocityX = (dx / dist) * 800.0f; // Increased speed for fast projectile
        spell.velocityY = (dy / dist) * 800.0f;
    } else {
        // Default direction if target is too close (avoid division by zero)
        spell.velocityX = 800.0f; // Fire to the right
        spell.velocityY = 0.0f;
    }
    spell.maxLifetime = 3.0f; // Longer range
    spell.damage = spellDatabase[SpellType::FIRE_BOLT].damage;
    spell.radius = 8.0f; // Smaller radius for projectile
    
    // Animation setup
    spell.currentFrame = 0;
    spell.animationTimer = 0.0f;
    spell.animationSpeed = 15.0f; // Faster animation for projectile
    spell.spritePath = ""; // Use default path
    
    activeSpells.push_back(spell);
}

void SpellSystem::castFlameWave() {
    // Get mouse position for direction
    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);
    float worldMouseX = mouseX + game->getCameraX();
    float worldMouseY = mouseY + game->getCameraY();
    
    // Calculate direction from player to mouse
    float playerCenterX = player->getX() + player->getWidth() / 2;
    float playerCenterY = player->getY() + player->getHeight() / 2;
    float dx = worldMouseX - playerCenterX;
    float dy = worldMouseY - playerCenterY;
    float dist = sqrt(dx*dx + dy*dy);
    
    float angleRad = atan2(dy, dx);
    
    // Create multiple projectiles in a cone shooting toward mouse
    for (int i = -2; i <= 2; i++) {
        // Add spatial offset to prevent overlapping spawn positions
        float sideOffset = i * 8.0f; // 8 pixels apart
        float spawnX = playerCenterX + cos(angleRad + M_PI/2) * sideOffset; // Perpendicular offset
        float spawnY = playerCenterY + sin(angleRad + M_PI/2) * sideOffset;
        
        ActiveSpell spell = createSafeSpell(SpellType::FLAME_WAVE, spawnX, spawnY);
        
        float spread = i * 0.2f; // Slightly tighter cone
        spell.velocityX = cos(angleRad + spread) * 350.0f;
        spell.velocityY = sin(angleRad + spread) * 350.0f;
        spell.maxLifetime = 1.2f; // Longer duration
        spell.damage = spellDatabase[SpellType::FLAME_WAVE].damage;
        spell.radius = 8.0f; // Reduced size by half
        
        // Add time delay to stagger spawning (negative lifetime = delay)
        spell.lifetime = -(abs(i) * 0.05f); // Center fires first, outer ones delayed
        
        // Animation setup
        spell.animationSpeed = 15.0f; // Faster animation
        
        activeSpells.push_back(spell);
    }
}

void SpellSystem::castMeteorStrike(float targetX, float targetY) {
    // Create multiple meteors in a pattern around the target (instant cast)
    for (int i = 0; i < 5; i++) {
        // Spread meteors in a circle around target
        float angle = (i * 2.0f * M_PI) / 5.0f;
        float offset = 30.0f;
        float meteorX = targetX + cos(angle) * offset;
        float meteorY = targetY + sin(angle) * offset - 200; // Start above target
        
        ActiveSpell spell = createSafeSpell(SpellType::METEOR_STRIKE, meteorX, meteorY);
        spell.targetX = targetX + cos(angle) * offset;
        spell.targetY = targetY + sin(angle) * offset;
        
        spell.velocityX = 0;
        spell.velocityY = 300.0f; // Fall down
        spell.lifetime = -(i * 0.1f); // Stagger the meteors
        spell.maxLifetime = 0.7f;
        spell.damage = spellDatabase[SpellType::METEOR_STRIKE].damage;
        spell.radius = 40.0f;
        
        // Animation setup
        spell.animationSpeed = 15.0f; // 15 FPS for meteor
        
        activeSpells.push_back(spell);
    }
}


void SpellSystem::startDragonsBreath() {
    if (!isChanneling) {
        isChanneling = true;
        channeledSpell = SpellType::DRAGONS_BREATH;
        channelTimer = 0.0f;
        channelDuration = spellDatabase[SpellType::DRAGONS_BREATH].duration;
    }
}


void SpellSystem::startChanneling(SpellType spell) {
    if (!isChanneling) {
        isChanneling = true;
        channeledSpell = spell;
        channelTimer = 0.0f;
        channelDuration = spellDatabase[spell].duration;
    }
}

void SpellSystem::stopChanneling() {
    isChanneling = false;
    channelTimer = 0.0f;
}

void SpellSystem::castIceShard(float targetX, float targetY) {
    float playerCenterX = player->getX() + player->getWidth()/2;
    float playerCenterY = player->getY() + player->getHeight()/2;
    
    ActiveSpell spell = createSafeSpell(SpellType::ICE_SHARD, playerCenterX, playerCenterY);
    
    float dx = targetX - playerCenterX;
    float dy = targetY - playerCenterY;
    float dist = sqrt(dx*dx + dy*dy);
    
    // Safety check for division by zero
    if (dist > 0.1f) {
        spell.velocityX = (dx / dist) * 450.0f;
        spell.velocityY = (dy / dist) * 450.0f;
    } else {
        // Default direction if target is too close
        spell.velocityX = 450.0f;
        spell.velocityY = 0.0f;
    }
    spell.maxLifetime = 2.0f;
    spell.damage = spellDatabase[SpellType::ICE_SHARD].damage;
    spell.radius = 8.0f;
    
    activeSpells.push_back(spell);
}

void SpellSystem::castFrozenGround(float targetX, float targetY) {
    ActiveSpell spell = createSafeSpell(SpellType::FROZEN_GROUND, targetX, targetY);
    spell.velocityX = 0;
    spell.velocityY = 0;
    spell.maxLifetime = 5.0f;
    spell.damage = 0;
    spell.radius = 60.0f;
    
    activeSpells.push_back(spell);
}

void SpellSystem::castBlizzard(float targetX, float targetY) {
    // Create multiple ice particles over time
    for (int i = 0; i < 20; i++) {
        float spellX = targetX + (rand() % 100 - 50);
        float spellY = targetY + (rand() % 100 - 50);
        
        ActiveSpell spell = createSafeSpell(SpellType::BLIZZARD, spellX, spellY);
        spell.velocityX = rand() % 100 - 50;
        spell.velocityY = rand() % 50 + 50;
        spell.lifetime = -(i * 0.2f); // Stagger spawn
        spell.maxLifetime = 4.0f;
        spell.damage = spellDatabase[SpellType::BLIZZARD].damage;
        spell.radius = 10.0f;
        
        activeSpells.push_back(spell);
    }
}

void SpellSystem::castAbsoluteZero() {
    // TODO: Freeze all visible enemies
    auto& enemies = game->getEnemies();
    for (auto& enemy : enemies) {
        // Enhanced safety checks for absolute zero
        if (!enemy || enemy.get() == nullptr || enemy->isDead()) {
            continue; // Skip null or dead enemies
        }
        
        try {
            // Check if enemy is on screen
            float screenX = enemy->getX() - game->getCameraX();
            float screenY = enemy->getY() - game->getCameraY();
            
            if (screenX >= -100 && screenX <= 1380 && screenY >= -100 && screenY <= 820) {
                // Final validity check before applying effect
                if (enemy && !enemy->isDead()) {
                    // Apply freeze effect (will need to add freeze state to Enemy class)
                    enemy->takeDamage(0); // Just mark for now
                }
            }
        } catch (...) {
            // Catch any exceptions during absolute zero application
            std::cout << "Error: Exception during absolute zero - enemy may be invalid" << std::endl;
            continue;
        }
    }
}

void SpellSystem::castToxicDart(float targetX, float targetY) {
    float playerCenterX = player->getX() + player->getWidth()/2;
    float playerCenterY = player->getY() + player->getHeight()/2;
    
    ActiveSpell spell = createSafeSpell(SpellType::TOXIC_DART, playerCenterX, playerCenterY);
    
    float dx = targetX - playerCenterX;
    float dy = targetY - playerCenterY;
    float dist = sqrt(dx*dx + dy*dy);
    
    // Safety check for division by zero
    if (dist > 0.1f) {
        spell.velocityX = (dx / dist) * 500.0f;
        spell.velocityY = (dy / dist) * 500.0f;
    } else {
        // Default direction if target is too close
        spell.velocityX = 500.0f;
        spell.velocityY = 0.0f;
    }
    spell.maxLifetime = 1.5f;
    spell.damage = spellDatabase[SpellType::TOXIC_DART].damage;
    spell.radius = 6.0f;
    
    activeSpells.push_back(spell);
}

void SpellSystem::castPoisonCloud(float targetX, float targetY) {
    ActiveSpell spell = createSafeSpell(SpellType::POISON_CLOUD, targetX, targetY);
    spell.velocityX = 0;
    spell.velocityY = 0;
    spell.maxLifetime = 10.0f;
    spell.damage = spellDatabase[SpellType::POISON_CLOUD].damage;
    spell.radius = 80.0f;
    
    activeSpells.push_back(spell);
}

void SpellSystem::castPlagueBomb(float targetX, float targetY) {
    float playerCenterX = player->getX() + player->getWidth()/2;
    float playerCenterY = player->getY() + player->getHeight()/2;
    
    ActiveSpell spell = createSafeSpell(SpellType::PLAGUE_BOMB, playerCenterX, playerCenterY);
    
    float dx = targetX - playerCenterX;
    float dy = targetY - playerCenterY;
    float dist = sqrt(dx*dx + dy*dy);
    
    // Safety check for division by zero
    if (dist > 0.1f) {
        spell.velocityX = (dx / dist) * 300.0f;
        spell.velocityY = (dy / dist) * 300.0f;
    } else {
        // Default direction if target is too close
        spell.velocityX = 300.0f;
        spell.velocityY = 0.0f;
    }
    spell.maxLifetime = 1.0f;
    spell.damage = spellDatabase[SpellType::PLAGUE_BOMB].damage;
    spell.radius = 60.0f;
    
    activeSpells.push_back(spell);
}

void SpellSystem::castDeathsEmbrace(float targetX, float targetY) {
    // TODO: Mark specific enemy for increased damage
    ActiveSpell spell = createSafeSpell(SpellType::DEATHS_EMBRACE, targetX, targetY);
    spell.velocityX = 0;
    spell.velocityY = 0;
    spell.maxLifetime = 5.0f;
    spell.damage = spellDatabase[SpellType::DEATHS_EMBRACE].damage;
    spell.radius = 30.0f;
    
    activeSpells.push_back(spell);
}

std::vector<SpellType> SpellSystem::getAvailableSpells() const {
    std::vector<SpellType> available;
    
    if (activeElement == SpellElement::FIRE) {
        if (currentEnchantLevel >= 0) available.push_back(SpellType::FIRE_BOLT);
        if (currentEnchantLevel >= 10) available.push_back(SpellType::FLAME_WAVE);
        if (currentEnchantLevel >= 20) available.push_back(SpellType::METEOR_STRIKE);
        if (currentEnchantLevel >= 30) available.push_back(SpellType::DRAGONS_BREATH);
    } else if (activeElement == SpellElement::WATER) {
        if (currentEnchantLevel >= 0) available.push_back(SpellType::ICE_SHARD);
        if (currentEnchantLevel >= 10) available.push_back(SpellType::FROZEN_GROUND);
        if (currentEnchantLevel >= 20) available.push_back(SpellType::BLIZZARD);
        if (currentEnchantLevel >= 30) available.push_back(SpellType::ABSOLUTE_ZERO);
    } else if (activeElement == SpellElement::POISON) {
        if (currentEnchantLevel >= 0) available.push_back(SpellType::TOXIC_DART);
        if (currentEnchantLevel >= 10) available.push_back(SpellType::POISON_CLOUD);
        if (currentEnchantLevel >= 20) available.push_back(SpellType::PLAGUE_BOMB);
        if (currentEnchantLevel >= 30) available.push_back(SpellType::DEATHS_EMBRACE);
    }
    
    return available;
}

bool SpellSystem::isSpellReady(SpellType spell) const {
    auto it = cooldowns.find(spell);
    return it == cooldowns.end() || it->second <= 0;
}

float SpellSystem::getCooldownRemaining(SpellType spell) const {
    auto it = cooldowns.find(spell);
    return (it != cooldowns.end()) ? it->second : 0.0f;
}

float SpellSystem::getCooldownPercent(SpellType spell) const {
    auto it = cooldowns.find(spell);
    if (it == cooldowns.end() || it->second <= 0) return 0.0f;
    
    const auto& spellData = spellDatabase.at(spell);
    return it->second / spellData.cooldown;
}

std::string SpellSystem::getSpellName(SpellType spell) const {
    auto it = spellDatabase.find(spell);
    return (it != spellDatabase.end()) ? it->second.name : "Unknown";
}

std::string SpellSystem::getSpellDescription(SpellType spell) const {
    auto it = spellDatabase.find(spell);
    return (it != spellDatabase.end()) ? it->second.description : "";
}

int SpellSystem::getSpellManaCost(SpellType spell) const {
    auto it = spellDatabase.find(spell);
    return (it != spellDatabase.end()) ? it->second.manaCost : 0;
}

float SpellSystem::getBurnDuration() const {
    if (activeElement == SpellElement::FIRE && currentEnchantLevel >= 5) {
        return 2.0f;
    }
    return 0.0f;
}

float SpellSystem::getAttackSpeedMultiplier() const {
    if (activeElement == SpellElement::FIRE && currentEnchantLevel >= 15) {
        if (player->getHealth() < player->getMaxHealth() / 2) {
            return 1.2f;
        }
    }
    return 1.0f;
}

float SpellSystem::getDamageReduction() const {
    if (activeElement == SpellElement::WATER && currentEnchantLevel >= 5) {
        return 0.1f;
    }
    return 0.0f;
}

float SpellSystem::getRegenPerSecond() const {
    if (activeElement == SpellElement::WATER && currentEnchantLevel >= 15) {
        return player->getMaxHealth() * 0.02f;
    }
    return 0.0f;
}

bool SpellSystem::hasInfernoAura() const {
    return activeElement == SpellElement::FIRE && currentEnchantLevel >= 25;
}

float SpellSystem::getInfernoAuraDamage() const {
    return hasInfernoAura() ? 5.0f : 0.0f;
}

float SpellSystem::getInfernoAuraRadius() const {
    return hasInfernoAura() ? 100.0f : 0.0f;
}

void SpellSystem::applyInfernoAura(float deltaTime) {
    if (!hasInfernoAura()) return;
    
    static float auraTick = 0.0f;
    auraTick += deltaTime;
    
    if (auraTick >= 0.5f) { // Damage every 0.5 seconds
        auraTick = 0.0f;
        
        auto& enemies = game->getEnemies();
        float auraRadius = getInfernoAuraRadius();
        float auraDamage = getInfernoAuraDamage();
        
        for (auto& enemy : enemies) {
            // Enhanced safety checks for inferno aura
            if (!enemy || enemy.get() == nullptr || enemy->isDead()) {
                continue; // Skip null or dead enemies
            }
            
            try {
                float dx = enemy->getX() - player->getX();
                float dy = enemy->getY() - player->getY();
                float dist = sqrt(dx*dx + dy*dy);
                
                if (dist <= auraRadius) {
                    // Final validity check before damage
                    if (enemy && !enemy->isDead()) {
                        enemy->takeDamage(static_cast<int>(auraDamage));
                    }
                }
            } catch (...) {
                // Catch any exceptions during aura damage application
                std::cout << "Error: Exception during inferno aura damage - enemy may be invalid" << std::endl;
                continue;
            }
        }
    }
}

float SpellSystem::getChannelProgress() const {
    if (!isChanneling || channelDuration <= 0.0f) return 0.0f;
    return std::min(channelTimer / channelDuration, 1.0f);
}

void SpellSystem::renderIndicators(Renderer* renderer, int mouseX, int mouseY) {
    if (!isChanneling || channeledSpell != SpellType::METEOR_STRIKE) return;
    
    // Get world coordinates for mouse position
    float worldMouseX = mouseX + game->getCameraX();
    float worldMouseY = mouseY + game->getCameraY();
    
    // Player center position
    float playerCenterX = player->getX() + player->getWidth() / 2;
    float playerCenterY = player->getY() + player->getHeight() / 2;
    
    // Check distance from player to mouse
    float dx = worldMouseX - playerCenterX;
    float dy = worldMouseY - playerCenterY;
    float dist = sqrt(dx * dx + dy * dy);
    
    float spellRange = 300.0f; // Default range
    auto it = spellDatabase.find(SpellType::METEOR_STRIKE);
    if (it != spellDatabase.end()) {
        spellRange = it->second.range;
    } else {
        std::cout << "Error: METEOR_STRIKE not found in spell database" << std::endl;
    }
    
    // Constrain target to within range
    float targetX, targetY;
    if (dist <= spellRange) {
        targetX = worldMouseX;
        targetY = worldMouseY;
    } else {
        // Clamp to range boundary
        float ratio = spellRange / dist;
        targetX = playerCenterX + dx * ratio;
        targetY = playerCenterY + dy * ratio;
    }
    
    // Update the actual channeled target to the constrained position (only if we're actually channeling)
    if (isChanneling && channeledSpell == SpellType::METEOR_STRIKE) {
        const_cast<SpellSystem*>(this)->channelTargetX = targetX;
        const_cast<SpellSystem*>(this)->channelTargetY = targetY;
    }
    
    // Render range indicator (circle around player center)
    SDL_SetRenderDrawColor(renderer->getSDLRenderer(), 255, 255, 0, 100); // Yellow with transparency
    int numPoints = 64;
    int playerScreenX = static_cast<int>(playerCenterX - game->getCameraX());
    int playerScreenY = static_cast<int>(playerCenterY - game->getCameraY());
    
    for (int i = 0; i < numPoints; i++) {
        float angle1 = (i * 2.0f * M_PI) / numPoints;
        float angle2 = ((i + 1) * 2.0f * M_PI) / numPoints;
        
        int x1 = static_cast<int>(playerScreenX + cos(angle1) * spellRange);
        int y1 = static_cast<int>(playerScreenY + sin(angle1) * spellRange);
        int x2 = static_cast<int>(playerScreenX + cos(angle2) * spellRange);
        int y2 = static_cast<int>(playerScreenY + sin(angle2) * spellRange);
        
        SDL_RenderDrawLine(renderer->getSDLRenderer(), x1, y1, x2, y2);
    }
    
    // Render placement indicator at constrained target location
    SDL_SetRenderDrawColor(renderer->getSDLRenderer(), 0, 255, 0, 150); // Always green since it's constrained to range
    
    // Draw circle at target location
    float effectRadius = 60.0f; // AoE effect radius
    int targetScreenX = static_cast<int>(targetX - game->getCameraX());
    int targetScreenY = static_cast<int>(targetY - game->getCameraY());
    
    for (int i = 0; i < numPoints; i++) {
        float angle1 = (i * 2.0f * M_PI) / numPoints;
        float angle2 = ((i + 1) * 2.0f * M_PI) / numPoints;
        
        int x1 = static_cast<int>(targetScreenX + cos(angle1) * effectRadius);
        int y1 = static_cast<int>(targetScreenY + sin(angle1) * effectRadius);
        int x2 = static_cast<int>(targetScreenX + cos(angle2) * effectRadius);
        int y2 = static_cast<int>(targetScreenY + sin(angle2) * effectRadius);
        
        SDL_RenderDrawLine(renderer->getSDLRenderer(), x1, y1, x2, y2);
    }
}

void SpellSystem::renderChannelBar(Renderer* renderer) {
    if (!isChanneling) return;
    
    // Get screen dimensions
    int screenW, screenH;
    SDL_GetRendererOutputSize(renderer->getSDLRenderer(), &screenW, &screenH);
    
    // Position channel bar just above skill bar (skill bar is at screenH - skillBarHeight - 20)
    int barWidth = 200;
    int barHeight = 12;
    int barX = (screenW - barWidth) / 2;
    int skillBarHeight = 64; // Approximate skill bar height (2 rows * 32px)
    int barY = screenH - skillBarHeight - 40; // Just above skill bar
    
    // Background
    SDL_SetRenderDrawColor(renderer->getSDLRenderer(), 50, 50, 50, 200);
    SDL_Rect bgRect = {barX - 2, barY - 2, barWidth + 4, barHeight + 4};
    SDL_RenderFillRect(renderer->getSDLRenderer(), &bgRect);
    
    // Progress bar
    float progress = getChannelProgress();
    int progressWidth = static_cast<int>(barWidth * progress);
    
    // Color based on spell type
    if (channeledSpell == SpellType::METEOR_STRIKE) {
        SDL_SetRenderDrawColor(renderer->getSDLRenderer(), 255, 100, 0, 255); // Orange for meteor
    } else {
        SDL_SetRenderDrawColor(renderer->getSDLRenderer(), 255, 0, 0, 255); // Red for other spells
    }
    
    SDL_Rect progressRect = {barX, barY, progressWidth, barHeight};
    SDL_RenderFillRect(renderer->getSDLRenderer(), &progressRect);
    
    // Border
    SDL_SetRenderDrawColor(renderer->getSDLRenderer(), 255, 255, 255, 255);
    SDL_Rect borderRect = {barX, barY, barWidth, barHeight};
    SDL_RenderDrawRect(renderer->getSDLRenderer(), &borderRect);
}

void SpellSystem::createExplosionEffect(float x, float y) {
    // Randomly pick one of the explosion types
    const std::vector<std::string> explosionTypes = {
        "assets/Textures/Spells/explosion.png",
        "assets/Textures/Spells/explosion_02.png", 
        "assets/Textures/Spells/explosion_03.png"
    };
    
    int randomIndex = rand() % explosionTypes.size();
    std::string explosionPath = explosionTypes[randomIndex];
    
    ActiveSpell explosion = createSafeSpell(SpellType::EXPLOSION_EFFECT, x, y);
    explosion.velocityX = 0;
    explosion.velocityY = 0;
    explosion.maxLifetime = 1.0f; // 1 second explosion
    explosion.damage = 0; // Visual effect only
    explosion.radius = 25.0f;
    explosion.spritePath = explosionPath; // Store the chosen explosion sprite
    
    // Animation setup
    explosion.animationSpeed = 16.0f; // Fast explosion animation
    
    activeSpells.push_back(explosion);
    
    // Also create particle and smoke effects
    createParticleEffect(x, y);
    createSmokeEffect(x, y);
}

void SpellSystem::createParticleEffect(float x, float y) {
    ActiveSpell particles = createSafeSpell(SpellType::PARTICLE_EFFECT, x, y);
    particles.velocityX = 0;
    particles.velocityY = 0;
    particles.maxLifetime = 1.5f;
    particles.damage = 0;
    particles.radius = 20.0f;
    
    // Animation setup
    particles.animationSpeed = 12.0f;
    
    activeSpells.push_back(particles);
}

void SpellSystem::createSmokeEffect(float x, float y) {
    ActiveSpell smoke = createSafeSpell(SpellType::SMOKE_EFFECT, x, y);
    smoke.velocityX = 0;
    smoke.velocityY = -20.0f; // Smoke drifts upward
    smoke.maxLifetime = 2.0f;
    smoke.damage = 0;
    smoke.radius = 30.0f;
    
    // Animation setup
    smoke.animationSpeed = 8.0f; // Slower smoke animation
    
    activeSpells.push_back(smoke);
}

// Helper function to safely create spells with all fields initialized
ActiveSpell SpellSystem::createSafeSpell(SpellType type, float x, float y) {
    ActiveSpell spell;
    
    // Initialize ALL fields to prevent crashes
    spell.type = type;
    spell.x = x;
    spell.y = y;
    spell.targetX = 0.0f;
    spell.targetY = 0.0f;
    spell.lifetime = 0.0f;
    spell.maxLifetime = 1.0f;
    spell.damage = 0;
    spell.active = true;
    spell.isChanneling = false;
    spell.channelTime = 0.0f;
    spell.radius = 10.0f;
    spell.velocityX = 0.0f;
    spell.velocityY = 0.0f;
    spell.speed = 0.0f;
    spell.currentFrame = 0;
    spell.animationTimer = 0.0f;
    spell.animationSpeed = 10.0f;
    spell.spritePath = ""; // Safe default
    
    return spell;
}