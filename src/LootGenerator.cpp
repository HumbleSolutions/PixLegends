#include "LootGenerator.h"
#include <algorithm>

LootGenerator& LootGenerator::getInstance() {
    static LootGenerator instance;
    return instance;
}

LootGenerator::LootGenerator() : rng(std::random_device{}()) {
}

void LootGenerator::setSeed(unsigned int seed) {
    rng.seed(seed);
}

std::vector<Loot> LootGenerator::generateEnemyLoot(int enemyLevel, float rarityBonus) {
    std::vector<Loot> loot;
    LootTable table = getEnemyLootTable(enemyLevel);
    
    // Always drop some gold
    if (rollFloat() < table.goldChance) {
        int goldAmount = rollRange(table.minGold, table.maxGold) * (1 + enemyLevel * 0.1f);
        loot.emplace_back(LootType::GOLD, goldAmount, "Gold Coins");
    }
    
    // Roll for number of items
    int numItems = rollRange(table.minItems, table.maxItems);
    
    for (int i = 0; i < numItems; i++) {
        // Roll for loot type based on weights
        float totalWeight = 0.0f;
        for (const auto& entry : table.weights) {
            totalWeight += entry.second;
        }
        
        float roll = rollFloat() * totalWeight;
        float currentWeight = 0.0f;
        
        for (const auto& entry : table.weights) {
            currentWeight += entry.second;
            if (roll <= currentWeight) {
                LootType type = entry.first;
                LootRarity rarity = rollRarity(rarityBonus);
                
                switch (type) {
                    case LootType::EXPERIENCE:
                        loot.emplace_back(LootType::EXPERIENCE, 
                                        rollRange(10, 25) * (1 + enemyLevel), 
                                        "Experience");
                        break;
                    case LootType::HEALTH_POTION:
                        loot.emplace_back(LootType::HEALTH_POTION, 1, "Health Potion", rarity);
                        break;
                    case LootType::MANA_POTION:
                        loot.emplace_back(LootType::MANA_POTION, 1, "Mana Potion", rarity);
                        break;
                    case LootType::EQUIPMENT:
                        loot.push_back(generateEquipmentLoot(enemyLevel, rarity));
                        break;
                    case LootType::SCROLL:
                        loot.push_back(generateScrollLoot(rarity));
                        break;
                    case LootType::MATERIAL:
                        loot.push_back(generateMaterialLoot(rarity));
                        break;
                    default:
                        break;
                }
                break;
            }
        }
    }
    
    return loot;
}

std::vector<Loot> LootGenerator::generateContainerLoot(ObjectType containerType) {
    std::vector<Loot> loot;
    LootTable table = getContainerLootTable(containerType);
    
    // Gold chance
    if (rollFloat() < table.goldChance) {
        int goldAmount = rollRange(table.minGold, table.maxGold);
        loot.emplace_back(LootType::GOLD, goldAmount, "Gold Coins");
    }
    
    // Roll for items
    int numItems = rollRange(table.minItems, table.maxItems);
    
    for (int i = 0; i < numItems; i++) {
        float totalWeight = 0.0f;
        for (const auto& entry : table.weights) {
            totalWeight += entry.second;
        }
        
        float roll = rollFloat() * totalWeight;
        float currentWeight = 0.0f;
        
        for (const auto& entry : table.weights) {
            currentWeight += entry.second;
            if (roll <= currentWeight) {
                LootType type = entry.first;
                LootRarity rarity = rollRarity(0.1f); // Small bonus for containers
                
                switch (type) {
                    case LootType::HEALTH_POTION:
                        loot.emplace_back(LootType::HEALTH_POTION, 
                                        rollRange(1, 3), "Health Potion", rarity);
                        break;
                    case LootType::MANA_POTION:
                        loot.emplace_back(LootType::MANA_POTION, 
                                        rollRange(1, 3), "Mana Potion", rarity);
                        break;
                    case LootType::EQUIPMENT:
                        loot.push_back(generateEquipmentLoot(rollRange(1, 5), rarity));
                        break;
                    case LootType::SCROLL:
                        loot.push_back(generateScrollLoot(rarity));
                        break;
                    case LootType::MATERIAL:
                        loot.push_back(generateMaterialLoot(rarity));
                        break;
                    default:
                        break;
                }
                break;
            }
        }
    }
    
    return loot;
}

Loot LootGenerator::generateEquipmentLoot(int level, LootRarity forceRarity) {
    auto templates = getEquipmentTemplates();
    
    // Filter templates by level
    std::vector<EquipmentTemplate> validTemplates;
    for (const auto& tmpl : templates) {
        if (level >= tmpl.minLevel && level <= tmpl.maxLevel) {
            validTemplates.push_back(tmpl);
        }
    }
    
    if (validTemplates.empty()) {
        validTemplates = templates; // Fallback to all templates
    }
    
    // Pick random template
    const auto& chosen = validTemplates[rollRange(0, validTemplates.size() - 1)];
    
    LootRarity rarity = (forceRarity != LootRarity::COMMON) ? forceRarity : rollRarity();
    
    // Generate name with rarity prefix
    std::string rarityPrefix;
    switch (rarity) {
        case LootRarity::UNCOMMON:  rarityPrefix = "Fine "; break;
        case LootRarity::RARE:      rarityPrefix = "Superior "; break;
        case LootRarity::EPIC:      rarityPrefix = "Masterwork "; break;
        case LootRarity::LEGENDARY: rarityPrefix = "Legendary "; break;
        default: break;
    }
    
    std::string fullName = rarityPrefix + chosen.baseName;
    std::string description = "Level " + std::to_string(level) + " " + chosen.baseName;
    
    return Loot(LootType::EQUIPMENT, 1, fullName, rarity, description);
}

Loot LootGenerator::generateScrollLoot(LootRarity rarity) {
    std::vector<std::string> scrollTypes = {
        "upgrade_scroll", "fire_dmg_scroll", "water_damage_scroll", 
        "poison_dmg_scroll", "armor_enchant_scroll"
    };
    
    std::string scrollType = scrollTypes[rollRange(0, scrollTypes.size() - 1)];
    std::string name = "Enchantment Scroll";
    
    if (scrollType == "upgrade_scroll") name = "Upgrade Scroll";
    else if (scrollType == "fire_dmg_scroll") name = "Fire Enchantment";
    else if (scrollType == "water_damage_scroll") name = "Water Enchantment";
    else if (scrollType == "poison_dmg_scroll") name = "Poison Enchantment";
    else if (scrollType == "armor_enchant_scroll") name = "Armor Enchantment";
    
    return Loot(LootType::SCROLL, 1, name, rarity, "Magical scroll for equipment enhancement");
}

Loot LootGenerator::generateMaterialLoot(LootRarity rarity) {
    auto materials = getMaterialNames(rarity);
    std::string name = materials[rollRange(0, materials.size() - 1)];
    
    int amount = 1;
    switch (rarity) {
        case LootRarity::COMMON: amount = rollRange(1, 3); break;
        case LootRarity::UNCOMMON: amount = rollRange(1, 2); break;
        default: amount = 1; break;
    }
    
    return Loot(LootType::MATERIAL, amount, name, rarity, "Crafting material");
}

LootRarity LootGenerator::rollRarity(float rarityBonus) {
    float roll = rollFloat() + rarityBonus;
    
    if (roll >= 0.95f) return LootRarity::LEGENDARY;      // 5% + bonus
    if (roll >= 0.85f) return LootRarity::EPIC;          // 10% + bonus  
    if (roll >= 0.65f) return LootRarity::RARE;          // 20% + bonus
    if (roll >= 0.35f) return LootRarity::UNCOMMON;      // 30% + bonus
    return LootRarity::COMMON;                           // 35% - bonus
}

LootGenerator::LootTable LootGenerator::getEnemyLootTable(int level) {
    LootTable table;
    
    // Base enemy loot chances
    table.weights = {
        {LootType::EXPERIENCE, 3.0f},
        {LootType::HEALTH_POTION, 1.5f},
        {LootType::MANA_POTION, 1.0f},
        {LootType::EQUIPMENT, 0.8f},
        {LootType::SCROLL, 0.5f},
        {LootType::MATERIAL, 1.2f}
    };
    
    table.minItems = 1;
    table.maxItems = std::min(2, 1 + level / 3); // More items for higher level enemies
    table.goldChance = 0.7f;
    table.minGold = 5 + level * 2;
    table.maxGold = 15 + level * 5;
    
    return table;
}

LootGenerator::LootTable LootGenerator::getContainerLootTable(ObjectType type) {
    LootTable table;
    
    switch (type) {
        case ObjectType::CHEST_UNOPENED:
            table.weights = {
                {LootType::EQUIPMENT, 2.0f},
                {LootType::SCROLL, 1.5f},
                {LootType::HEALTH_POTION, 1.0f},
                {LootType::MANA_POTION, 1.0f},
                {LootType::MATERIAL, 1.0f}
            };
            table.minItems = 2;
            table.maxItems = 4;
            table.goldChance = 0.8f;
            table.minGold = 20;
            table.maxGold = 100;
            break;
            
        case ObjectType::CLAY_POT:
            table.weights = {
                {LootType::HEALTH_POTION, 2.0f},
                {LootType::MANA_POTION, 1.5f},
                {LootType::MATERIAL, 1.0f}
            };
            table.minItems = 1;
            table.maxItems = 2;
            table.goldChance = 0.3f;
            table.minGold = 5;
            table.maxGold = 25;
            break;
            
        case ObjectType::WOOD_CRATE:
        case ObjectType::STEEL_CRATE:
            table.weights = {
                {LootType::EQUIPMENT, 1.0f},
                {LootType::MATERIAL, 2.0f},
                {LootType::SCROLL, 0.8f}
            };
            table.minItems = 1;
            table.maxItems = 3;
            table.goldChance = 0.5f;
            table.minGold = 10;
            table.maxGold = 50;
            break;
            
        default:
            // Default container
            table.weights = {{LootType::HEALTH_POTION, 1.0f}};
            table.minItems = 1;
            table.maxItems = 1;
            table.goldChance = 0.2f;
            table.minGold = 5;
            table.maxGold = 15;
            break;
    }
    
    return table;
}

std::vector<LootGenerator::EquipmentTemplate> LootGenerator::getEquipmentTemplates() {
    return {
        {"Iron Sword", 1, 3, 1.0f},
        {"Steel Blade", 3, 6, 1.2f},
        {"Silver Sword", 5, 8, 1.5f},
        {"Enchanted Blade", 7, 10, 1.8f},
        
        {"Leather Armor", 1, 3, 1.0f},
        {"Chain Mail", 3, 6, 1.2f},
        {"Plate Armor", 5, 8, 1.5f},
        {"Dragon Scale", 7, 10, 1.8f},
        
        {"Simple Ring", 1, 5, 0.8f},
        {"Magic Ring", 3, 8, 1.2f},
        {"Power Ring", 6, 10, 1.6f},
        
        {"Cloth Robes", 1, 4, 0.9f},
        {"Wizard Hat", 2, 6, 0.7f},
        {"Battle Gloves", 1, 7, 0.8f},
        {"Swift Boots", 2, 8, 1.0f}
    };
}

std::vector<std::string> LootGenerator::getMaterialNames(LootRarity rarity) {
    switch (rarity) {
        case LootRarity::COMMON:
            return {"Iron Ore", "Cloth Scraps", "Leather Hide", "Wood Planks"};
        case LootRarity::UNCOMMON:
            return {"Silver Ore", "Fine Silk", "Thick Leather", "Hardwood"};
        case LootRarity::RARE:
            return {"Gold Ore", "Enchanted Thread", "Dragon Leather", "Ancient Wood"};
        case LootRarity::EPIC:
            return {"Platinum Ore", "Starweave Silk", "Phoenix Feather", "World Tree Branch"};
        case LootRarity::LEGENDARY:
            return {"Mithril Ore", "Void Silk", "Dragon Heart", "Yggdrasil Wood"};
        default:
            return {"Iron Ore"};
    }
}

int LootGenerator::rollRange(int min, int max) {
    if (min >= max) return min;
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

float LootGenerator::rollFloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}