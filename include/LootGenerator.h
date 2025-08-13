#pragma once

#include "Object.h"
#include <random>
#include <vector>

class LootGenerator {
public:
    static LootGenerator& getInstance();
    
    // Generate loot for enemies based on level and rarity
    std::vector<Loot> generateEnemyLoot(int enemyLevel, float rarityBonus = 0.0f);
    
    // Generate loot for containers (chests, pots, etc)
    std::vector<Loot> generateContainerLoot(ObjectType containerType);
    
    // Generate specific loot types
    Loot generateEquipmentLoot(int level, LootRarity forceRarity = LootRarity::COMMON);
    Loot generateScrollLoot(LootRarity rarity = LootRarity::COMMON);
    Loot generateMaterialLoot(LootRarity rarity = LootRarity::COMMON);
    
    // Rarity roll functions
    LootRarity rollRarity(float rarityBonus = 0.0f);
    
    // Set seed for deterministic generation (testing)
    void setSeed(unsigned int seed);
    
private:
    LootGenerator();
    std::mt19937 rng;
    
    // Loot tables
    struct LootTable {
        std::vector<std::pair<LootType, float>> weights; // type, weight
        int minItems = 1;
        int maxItems = 1;
        float goldChance = 0.5f;
        int minGold = 10;
        int maxGold = 50;
    };
    
    LootTable getEnemyLootTable(int level);
    LootTable getContainerLootTable(ObjectType type);
    
    // Equipment generation
    struct EquipmentTemplate {
        std::string baseName;
        int minLevel;
        int maxLevel;
        float statRange;
    };
    
    std::vector<EquipmentTemplate> getEquipmentTemplates();
    
    // Material names
    std::vector<std::string> getMaterialNames(LootRarity rarity);
    
    // Utility functions
    int rollRange(int min, int max);
    float rollFloat(float min = 0.0f, float max = 1.0f);
};