#pragma once

#include <SDL.h>
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

class Game;
class Renderer;
class Player;
class AssetManager;
class SpriteSheet;

// Forward declaration

enum class SpellElement {
    NONE,
    FIRE,
    WATER,
    POISON
};

enum class SpellType {
    // Fire Spells
    FIRE_BOLT,
    FLAME_WAVE,
    METEOR_STRIKE,
    DRAGONS_BREATH,
    
    // Fire Passives
    FIRE_MASTERY,
    BURNING_AURA,
    INFERNO_LORD,
    
    // Water Spells
    ICE_SHARD,
    FROZEN_GROUND,
    BLIZZARD,
    ABSOLUTE_ZERO,
    
    // Poison Spells
    TOXIC_DART,
    POISON_CLOUD,
    PLAGUE_BOMB,
    DEATHS_EMBRACE,
    
    // Visual Effects
    EXPLOSION_EFFECT,
    PARTICLE_EFFECT,
    SMOKE_EFFECT
};

struct SpellData {
    SpellType type;
    std::string name;
    int requiredEnchantLevel;
    float cooldown;
    int manaCost;
    int damage;
    float range;
    bool isChanneled;
    float duration;
    std::string description;
};

struct ActiveSpell {
    SpellType type;
    float x, y;
    float targetX, targetY;
    float lifetime;
    float maxLifetime;
    int damage;
    bool active;
    
    // For channeled spells
    bool isChanneling;
    float channelTime;
    
    // For AoE spells
    float radius;
    
    // For projectile spells
    float velocityX, velocityY;
    float speed;
    
    // Animation support
    int currentFrame;
    float animationTimer;
    float animationSpeed;
    
    // Effect-specific data
    std::string spritePath; // For dynamically chosen sprites like explosions
};

struct PassiveEffect {
    std::string name;
    int requiredEnchantLevel;
    std::string description;
    bool active;
    
    // Effect values
    float burnDuration;
    float attackSpeedBonus;
    float damageReduction;
    float regenPerSecond;
    float slowChance;
    float poisonSpreadChance;
    float auraRadius;
    float auraDamage;
};

class SpellSystem {
public:
    SpellSystem(Game* game, Player* player);
    ~SpellSystem() = default;
    
    void update(float deltaTime);
    void render(Renderer* renderer);
    void renderIndicators(Renderer* renderer, int mouseX, int mouseY);
    void renderChannelBar(Renderer* renderer);
    
    // Spell casting
    void castSpell(int spellSlot); // Maps to keys 3-6 or Q,E,R,F
    void castSpellAtPosition(SpellType spell, float targetX, float targetY);
    void startChanneling(SpellType spell);
    void stopChanneling();
    bool getIsChanneling() const { return isChanneling; }
    float getChannelProgress() const;
    
    // Spell tree management
    void updateActiveSpellTree();
    SpellElement getActiveElement() const { return activeElement; }
    std::vector<SpellType> getAvailableSpells() const;
    std::vector<PassiveEffect> getActivePassives() const;
    
    // Cooldown management
    bool isSpellReady(SpellType spell) const;
    float getCooldownRemaining(SpellType spell) const;
    float getCooldownPercent(SpellType spell) const;
    
    // Passive effects
    float getBurnDuration() const;
    float getAttackSpeedMultiplier() const;
    float getDamageReduction() const;
    float getRegenPerSecond() const;
    bool hasInfernoAura() const;
    float getInfernoAuraDamage() const;
    float getInfernoAuraRadius() const;
    bool canFreezeOnHit() const;
    float getFreezeChance() const;
    bool canPoisonSpread() const;
    float getPoisonSpreadChance() const;
    bool hasToxicBlood() const;
    float getToxicBloodDamage() const;
    
    // UI helpers
    std::string getSpellName(SpellType spell) const;
    std::string getSpellDescription(SpellType spell) const;
    int getSpellManaCost(SpellType spell) const;
    SDL_Rect getSpellIcon(SpellType spell) const;
    
    // Damage helpers for integration with combat
    void applyBurningStrike(float x, float y);
    void applyInfernoAura(float deltaTime);
    void applyToxicBlood(float attackerX, float attackerY);
    
private:
    Game* game;
    Player* player;
    AssetManager* assetManager;
    
    SpellElement activeElement;
    int currentEnchantLevel;
    
    // Active spells in world
    std::vector<ActiveSpell> activeSpells;
    
    // Spell definitions
    std::unordered_map<SpellType, SpellData> spellDatabase;
    
    // Cooldown tracking
    std::unordered_map<SpellType, float> cooldowns;
    
    // Passive effects
    std::vector<PassiveEffect> firePassives;
    std::vector<PassiveEffect> waterPassives;
    std::vector<PassiveEffect> poisonPassives;
    
    // Channeling state
    bool isChanneling;
    SpellType channeledSpell;
    float channelTimer;
    float channelDuration;
    float channelTargetX;
    float channelTargetY;
    
    // Helper functions
    void initializeSpellDatabase();
    void initializePassives();
    void updateActiveSpells(float deltaTime);
    void updateCooldowns(float deltaTime);
    void updatePassives();
    void renderActiveSpells(Renderer* renderer);
    void renderSpellEffects(Renderer* renderer, const ActiveSpell& spell);
    void renderSpellFallback(Renderer* renderer, const ActiveSpell& spell);
    
    // Spell implementations
    void castFireBolt(float targetX, float targetY);
    void castFlameWave();
    void castMeteorStrike(float targetX, float targetY);
    void startDragonsBreath();
    
    void castIceShard(float targetX, float targetY);
    void castFrozenGround(float targetX, float targetY);
    void castBlizzard(float targetX, float targetY);
    void castAbsoluteZero();
    
    void castToxicDart(float targetX, float targetY);
    void castPoisonCloud(float targetX, float targetY);
    void castPlagueBomb(float targetX, float targetY);
    void castDeathsEmbrace(float targetX, float targetY);
    
    // Collision detection
    bool checkSpellCollision(const ActiveSpell& spell, float targetX, float targetY, float targetWidth, float targetHeight);
    void handleSpellHit(const ActiveSpell& spell, float targetX, float targetY);
    
    // Effect creation
    void createExplosionEffect(float x, float y);
    void createParticleEffect(float x, float y);
    void createSmokeEffect(float x, float y);
    
    // Safe spell creation
    ActiveSpell createSafeSpell(SpellType type, float x, float y);
};