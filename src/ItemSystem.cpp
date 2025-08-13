#include "ItemSystem.h"
#include "AssetManager.h"
#include <iostream>
#include <algorithm>

// Item Implementation
Item::Item(const std::string& itemId, const std::string& itemName, ItemType itemType, 
           ItemRarity itemRarity, int maxStack)
    : id(itemId), name(itemName), type(itemType), rarity(itemRarity), 
      stackSize(maxStack), currentStack(1), plusLevel(0) {
    
    description = ""; // Will be set by templates
    iconPath = "";    // Will be set by templates
    equipmentType = EquipmentType::RING; // Default, will be overridden
}

SDL_Color Item::getRarityColor() const {
    switch (rarity) {
        case ItemRarity::COMMON:    return {255, 255, 255, 255}; // White
        case ItemRarity::UNCOMMON:  return {0, 255, 0, 255};     // Green
        case ItemRarity::RARE:      return {0, 100, 255, 255};   // Blue
        case ItemRarity::EPIC:      return {128, 0, 255, 255};   // Purple
        case ItemRarity::LEGENDARY: return {255, 165, 0, 255};   // Orange
        default:                    return {255, 255, 255, 255}; // White fallback
    }
}

std::string Item::getDisplayName() const {
    if (type == ItemType::EQUIPMENT) {
        return name + " (+" + std::to_string(plusLevel) + ")";
    }
    return name;
}

std::string Item::getTooltipText() const {
    std::string tooltip = getDisplayName() + "\n";
    
    // Add rarity
    switch (rarity) {
        case ItemRarity::COMMON:    tooltip += "Common"; break;
        case ItemRarity::UNCOMMON:  tooltip += "Uncommon"; break;
        case ItemRarity::RARE:      tooltip += "Rare"; break;
        case ItemRarity::EPIC:      tooltip += "Epic"; break;
        case ItemRarity::LEGENDARY: tooltip += "Legendary"; break;
    }
    
    if (type == ItemType::EQUIPMENT) {
        tooltip += " Equipment\n\n";
        
        // Add stats if any
        if (stats.attack > 0) tooltip += "Attack: +" + std::to_string(stats.attack) + "\n";
        if (stats.defense > 0) tooltip += "Defense: +" + std::to_string(stats.defense) + "\n";
        if (stats.health > 0) tooltip += "Health: +" + std::to_string(stats.health) + "\n";
        if (stats.mana > 0) tooltip += "Mana: +" + std::to_string(stats.mana) + "\n";
        if (stats.strength > 0) tooltip += "Strength: +" + std::to_string(stats.strength) + "\n";
        if (stats.intelligence > 0) tooltip += "Intelligence: +" + std::to_string(stats.intelligence) + "\n";
        
        // Elemental stats
        if (stats.fireAttack > 0) tooltip += "Fire Attack: +" + std::to_string(stats.fireAttack) + "\n";
        if (stats.waterAttack > 0) tooltip += "Water Attack: +" + std::to_string(stats.waterAttack) + "\n";
        if (stats.poisonAttack > 0) tooltip += "Poison Attack: +" + std::to_string(stats.poisonAttack) + "\n";
        if (stats.fireResist > 0) tooltip += "Fire Resist: +" + std::to_string(stats.fireResist) + "\n";
        if (stats.waterResist > 0) tooltip += "Water Resist: +" + std::to_string(stats.waterResist) + "\n";
        if (stats.poisonResist > 0) tooltip += "Poison Resist: +" + std::to_string(stats.poisonResist) + "\n";
    } else {
        tooltip += " Item\n\n";
    }
    
    tooltip += description;
    
    if (currentStack > 1) {
        tooltip += "\n\nStack: " + std::to_string(currentStack) + "/" + std::to_string(stackSize);
    }
    
    return tooltip;
}

bool Item::canStackWith(const Item& other) const {
    // For equipment items, each instance is unique and cannot stack
    if (type == ItemType::EQUIPMENT) {
        return false;
    }
    
    // For non-equipment items, they can stack if they have the same template ID and rarity
    return id == other.id && 
           rarity == other.rarity && 
           currentStack < stackSize;
}

// ItemSystem Implementation
ItemSystem::ItemSystem(AssetManager* assetManager) : assetManager(assetManager) {
    // Initialize inventories
    itemInventory.resize(INVENTORY_SIZE);
    scrollInventory.resize(SCROLL_INVENTORY_SIZE);
    equipmentSlots.resize(9); // 9 equipment slots
    
    // Initialize item templates
    initializeItemTemplates();
}

ItemSystem::~ItemSystem() {
    // Clean up item instances
    itemInstances.clear();
}

void ItemSystem::initializeItemTemplates() {
    // STARTER EQUIPMENT (Common rarity - low stats for new players)
    createItemTemplate("rusty_sword", "Rusty Sword", "A worn blade that's seen better days.", 
                      "assets/Textures/Items/sword_01.png", ItemType::EQUIPMENT, EquipmentType::WEAPON, ItemRarity::COMMON);
                      
    createItemTemplate("old_bow", "Old Bow", "A creaky wooden bow with frayed string.", 
                      "assets/Textures/Items/bow_01.png", ItemType::EQUIPMENT, EquipmentType::WEAPON, ItemRarity::COMMON);
                      
    createItemTemplate("cloth_cap", "Cloth Cap", "Basic head covering, provides minimal protection.", 
                      "assets/Textures/Items/helmet_01.png", ItemType::EQUIPMENT, EquipmentType::HELMET, ItemRarity::COMMON);
                      
    createItemTemplate("torn_shirt", "Torn Shirt", "A ragged shirt offering little defense.", 
                      "assets/Textures/Items/chestpeice_01.png", ItemType::EQUIPMENT, EquipmentType::CHEST, ItemRarity::COMMON);
                      
    createItemTemplate("wooden_shield", "Wooden Shield", "A basic shield made of cheap wood.", 
                      "assets/Textures/Items/bow_01.png", ItemType::EQUIPMENT, EquipmentType::SHIELD, ItemRarity::COMMON);
                      
    createItemTemplate("fabric_gloves", "Fabric Gloves", "Thin gloves that barely protect your hands.", 
                      "assets/Textures/Items/gloves_01.png", ItemType::EQUIPMENT, EquipmentType::GLOVES, ItemRarity::COMMON);
                      
    createItemTemplate("canvas_shoes", "Canvas Shoes", "Simple footwear with worn soles.", 
                      "assets/Textures/Items/boots_01.png", ItemType::EQUIPMENT, EquipmentType::BOOTS, ItemRarity::COMMON);
                      
    createItemTemplate("frayed_rope", "Frayed Rope", "A weathered rope belt.", 
                      "assets/Textures/Items/waist_01.png", ItemType::EQUIPMENT, EquipmentType::WAIST, ItemRarity::COMMON);
                      
    createItemTemplate("copper_ring", "Copper Ring", "A tarnished copper band.", 
                      "assets/Textures/Items/ring_01.png", ItemType::EQUIPMENT, EquipmentType::RING, ItemRarity::COMMON);
                      
    createItemTemplate("string_necklace", "String Necklace", "A simple string with a small charm.", 
                      "assets/Textures/Items/necklace_01.png", ItemType::EQUIPMENT, EquipmentType::NECKLACE, ItemRarity::COMMON);

    // BETTER EQUIPMENT (Uncommon and higher)
    createItemTemplate("iron_sword", "Iron Sword", "A sturdy iron blade.", 
                      "assets/Textures/Items/sword_01.png", ItemType::EQUIPMENT, EquipmentType::WEAPON, ItemRarity::UNCOMMON);
    
    createItemTemplate("recurve_bow", "Recurve Bow", "A flexible hunting bow.", 
                      "assets/Textures/Items/bow_01.png", ItemType::EQUIPMENT, EquipmentType::WEAPON);
    
    createItemTemplate("iron_helmet", "Iron Helmet", "Basic head protection.", 
                      "assets/Textures/Items/helmet_01.png", ItemType::EQUIPMENT, EquipmentType::HELMET);
    
    createItemTemplate("leather_chest", "Leather Chest", "Light body armor.", 
                      "assets/Textures/Items/chestpeice_01.png", ItemType::EQUIPMENT, EquipmentType::CHEST);
    
    createItemTemplate("simple_shield", "Simple Shield", "Basic wooden shield.", 
                      "assets/Textures/Items/sheild_01.png", ItemType::EQUIPMENT, EquipmentType::SHIELD);
    
    createItemTemplate("leather_gloves", "Leather Gloves", "Flexible hand protection.", 
                      "assets/Textures/Items/gloves_01.png", ItemType::EQUIPMENT, EquipmentType::GLOVES);
    
    createItemTemplate("leather_boots", "Leather Boots", "Comfortable footwear.", 
                      "assets/Textures/Items/boots_01.png", ItemType::EQUIPMENT, EquipmentType::BOOTS);
    
    createItemTemplate("rope_belt", "Rope Belt", "Simple waist accessory.", 
                      "assets/Textures/Items/waist_01.png", ItemType::EQUIPMENT, EquipmentType::WAIST);
    
    createItemTemplate("silver_ring", "Silver Ring", "A polished silver band.", 
                      "assets/Textures/Items/ring_01.png", ItemType::EQUIPMENT, EquipmentType::RING);
    
    createItemTemplate("gold_necklace", "Gold Necklace", "An ornate golden chain.", 
                      "assets/Textures/Items/necklace_01.png", ItemType::EQUIPMENT, EquipmentType::NECKLACE);
    
    // Scroll items
    createItemTemplate("upgrade_scroll", "Upgrade Scroll", "Enhances equipment +level.", 
                      "assets/Textures/Items/upgrade_scroll.png", ItemType::SCROLL);
    
    createItemTemplate("fire_scroll", "Fire Enchantment", "Adds fire damage to equipment.", 
                      "assets/Textures/Items/fire_dmg_scroll.png", ItemType::SCROLL);
    
    createItemTemplate("water_scroll", "Water Enchantment", "Adds water damage to equipment.", 
                      "assets/Textures/Items/water_damage_scroll.png", ItemType::SCROLL);
    
    createItemTemplate("poison_scroll", "Poison Enchantment", "Adds poison damage to equipment.", 
                      "assets/Textures/Items/Poison_dmg_scroll.png", ItemType::SCROLL);
    
    createItemTemplate("armor_scroll", "Armor Enchantment", "Enhances armor defense.", 
                      "assets/Textures/Items/armor_enchant_scroll.png", ItemType::SCROLL);
    
    // Set base stats for equipment items (BEFORE rarity scaling is applied)
    
    // STARTER ITEMS - Base stats (will be scaled down by Common rarity 0.5x multiplier)
    auto& rustySword = itemTemplates["rusty_sword"];
    rustySword.stats.attack = 10;  // Will become 5 after Common scaling
    rustySword.stats.strength = 2; // Will become 1 after Common scaling
    
    auto& oldBow = itemTemplates["old_bow"];
    oldBow.stats.attack = 8;       // Will become 4 after Common scaling
    oldBow.stats.intelligence = 1; // Will become 0 after Common scaling (minimum viable)
    
    auto& clothCap = itemTemplates["cloth_cap"];
    clothCap.stats.defense = 4;    // Will become 2 after Common scaling
    clothCap.stats.health = 10;    // Will become 5 after Common scaling
    
    auto& tornShirt = itemTemplates["torn_shirt"];
    tornShirt.stats.defense = 6;   // Will become 3 after Common scaling
    tornShirt.stats.health = 20;   // Will become 10 after Common scaling
    
    auto& woodenShield = itemTemplates["wooden_shield"];
    woodenShield.stats.defense = 6; // Will become 3 after Common scaling
    woodenShield.stats.health = 10; // Will become 5 after Common scaling
    
    auto& fabricGloves = itemTemplates["fabric_gloves"];
    fabricGloves.stats.defense = 2; // Will become 1 after Common scaling
    fabricGloves.stats.attack = 1;  // Will become 0 after Common scaling (minimum viable)
    
    auto& canvasShoes = itemTemplates["canvas_shoes"];
    canvasShoes.stats.defense = 4;  // Will become 2 after Common scaling
    canvasShoes.stats.health = 8;   // Will become 4 after Common scaling
    
    auto& frayedRope = itemTemplates["frayed_rope"];
    frayedRope.stats.health = 8;    // Will become 4 after Common scaling
    
    auto& copperRing = itemTemplates["copper_ring"];
    copperRing.stats.health = 6;    // Will become 3 after Common scaling
    copperRing.stats.mana = 4;      // Will become 2 after Common scaling
    
    auto& stringNecklace = itemTemplates["string_necklace"];
    stringNecklace.stats.health = 8; // Will become 4 after Common scaling
    stringNecklace.stats.mana = 6;   // Will become 3 after Common scaling

    // BETTER ITEMS
    auto& sword = itemTemplates["iron_sword"];
    sword.stats.attack = 15;
    sword.stats.strength = 3;
    
    auto& bow = itemTemplates["recurve_bow"];
    bow.stats.attack = 12;
    bow.stats.intelligence = 2;
    
    auto& helmet = itemTemplates["iron_helmet"];
    helmet.stats.defense = 8;
    helmet.stats.health = 20;
    
    auto& chest = itemTemplates["leather_chest"];
    chest.stats.defense = 15;
    chest.stats.health = 35;
    
    auto& shield = itemTemplates["simple_shield"];
    shield.stats.defense = 10;
    shield.stats.health = 15;
    
    auto& gloves = itemTemplates["leather_gloves"];
    gloves.stats.defense = 5;
    gloves.stats.attack = 2;
    
    auto& boots = itemTemplates["leather_boots"];
    boots.stats.defense = 6;
    boots.stats.health = 10;
    
    auto& belt = itemTemplates["rope_belt"];
    belt.stats.health = 15;
    belt.stats.mana = 10;
    
    auto& ring = itemTemplates["silver_ring"];
    ring.stats.strength = 5;
    ring.stats.intelligence = 5;
    
    auto& necklace = itemTemplates["gold_necklace"];
    necklace.stats.health = 25;
    necklace.stats.mana = 25;
}

void ItemSystem::createItemTemplate(const std::string& id, const std::string& name, const std::string& desc,
                                   const std::string& iconPath, ItemType type, EquipmentType equipType, ItemRarity rarity) {
    Item item(id, name, type, rarity);
    item.description = desc;
    item.iconPath = iconPath;
    item.equipmentType = equipType;
    
    if (type == ItemType::SCROLL) {
        item.stackSize = 99; // Scrolls can stack
    } else if (type == ItemType::EQUIPMENT) {
        item.stackSize = 1;  // Equipment doesn't stack
        
        // Apply rarity scaling to base stats
        float rarityMultiplier = getRarityMultiplier(rarity);
        scaleStatsForRarity(item, rarityMultiplier);
    }
    
    itemTemplates[id] = item;
    
    // Load and cache the icon
    if (assetManager) {
        Texture* icon = assetManager->getTexture(iconPath);
        if (icon) {
            itemIcons[id] = icon;
        }
    }
}

float ItemSystem::getRarityMultiplier(ItemRarity rarity) const {
    switch (rarity) {
        case ItemRarity::COMMON:    return 0.5f;  // Starter gear - weak stats
        case ItemRarity::UNCOMMON:  return 1.0f;  // Normal baseline
        case ItemRarity::RARE:      return 1.5f;  // 50% better
        case ItemRarity::EPIC:      return 2.0f;  // Double
        case ItemRarity::LEGENDARY: return 3.0f;  // Triple
        default:                    return 1.0f;
    }
}

void ItemSystem::scaleStatsForRarity(Item& item, float multiplier) const {
    // Scale all numeric stats by the rarity multiplier
    item.stats.attack = static_cast<int>(item.stats.attack * multiplier);
    item.stats.defense = static_cast<int>(item.stats.defense * multiplier);
    item.stats.health = static_cast<int>(item.stats.health * multiplier);
    item.stats.mana = static_cast<int>(item.stats.mana * multiplier);
    item.stats.strength = static_cast<int>(item.stats.strength * multiplier);
    item.stats.intelligence = static_cast<int>(item.stats.intelligence * multiplier);
    item.stats.fireAttack = static_cast<int>(item.stats.fireAttack * multiplier);
    item.stats.waterAttack = static_cast<int>(item.stats.waterAttack * multiplier);
    item.stats.poisonAttack = static_cast<int>(item.stats.poisonAttack * multiplier);
    item.stats.fireResist = static_cast<int>(item.stats.fireResist * multiplier);
    item.stats.waterResist = static_cast<int>(item.stats.waterResist * multiplier);
    item.stats.poisonResist = static_cast<int>(item.stats.poisonResist * multiplier);
}

Item* ItemSystem::createItem(const std::string& itemId, ItemRarity rarity, int stack) {
    auto templateIt = itemTemplates.find(itemId);
    if (templateIt == itemTemplates.end()) {
        std::cout << "Warning: Unknown item ID: " << itemId << std::endl;
        return nullptr;
    }
    
    auto item = std::make_unique<Item>(templateIt->second);
    item->rarity = rarity;
    item->currentStack = std::min(stack, item->stackSize);
    
    // Generate unique instance ID
    item->instanceId = itemId + "_" + std::to_string(nextInstanceId++);
    
    Item* itemPtr = item.get();
    itemInstances.push_back(std::move(item));
    
    return itemPtr;
}

Item* ItemSystem::getItemTemplate(const std::string& itemId) const {
    auto it = itemTemplates.find(itemId);
    if (it != itemTemplates.end()) {
        return const_cast<Item*>(&it->second);
    }
    return nullptr;
}

bool ItemSystem::addItem(const std::string& itemId, int amount, ItemRarity rarity) {
    auto templateIt = itemTemplates.find(itemId);
    if (templateIt == itemTemplates.end()) {
        return false;
    }
    
    bool isScroll = (templateIt->second.type == ItemType::SCROLL);
    
    // Try to stack with existing items first
    for (int i = 0; i < (isScroll ? SCROLL_INVENTORY_SIZE : INVENTORY_SIZE); i++) {
        auto& inventory = isScroll ? scrollInventory : itemInventory;
        if (!inventory[i].isEmpty() && 
            inventory[i].item->id == itemId && 
            inventory[i].item->rarity == rarity &&
            inventory[i].item->canStackWith(*inventory[i].item)) {
            
            int canAdd = inventory[i].item->stackSize - inventory[i].item->currentStack;
            int toAdd = std::min(amount, canAdd);
            inventory[i].item->currentStack += toAdd;
            amount -= toAdd;
            
            if (amount <= 0) return true;
        }
    }
    
    // Create new stacks for remaining amount
    while (amount > 0) {
        int emptySlot = findEmptySlot(isScroll);
        if (emptySlot == -1) {
            std::cout << "Inventory full!" << std::endl;
            return false;
        }
        
        int stackAmount = std::min(amount, templateIt->second.stackSize);
        Item* newItem = createItem(itemId, rarity, stackAmount);
        if (!newItem) return false;
        
        auto& inventory = isScroll ? scrollInventory : itemInventory;
        inventory[emptySlot].setItem(newItem);
        amount -= stackAmount;
    }
    
    return true;
}

bool ItemSystem::addItemToSlot(Item* item, int slotIndex, bool isScrollInventory) {
    if (!item) return false;
    
    auto& inventory = isScrollInventory ? scrollInventory : itemInventory;
    int maxSize = isScrollInventory ? SCROLL_INVENTORY_SIZE : INVENTORY_SIZE;
    
    if (slotIndex < 0 || slotIndex >= maxSize) return false;
    
    // Check if slot is empty
    if (!inventory[slotIndex].isEmpty()) return false;
    
    // Add the item to the specified slot
    inventory[slotIndex].setItem(item);
    return true;
}

int ItemSystem::findEmptySlot(bool isScrollInventory) const {
    const auto& inventory = isScrollInventory ? scrollInventory : itemInventory;
    int size = isScrollInventory ? SCROLL_INVENTORY_SIZE : INVENTORY_SIZE;
    
    for (int i = 0; i < size; i++) {
        if (inventory[i].isEmpty()) {
            return i;
        }
    }
    return -1;
}

ItemStats ItemSystem::calculateTotalStats() const {
    ItemStats total;
    
    for (const auto& slot : equipmentSlots) {
        if (!slot.isEmpty() && slot.item->type == ItemType::EQUIPMENT) {
            const ItemStats& stats = slot.item->stats;
            total.attack += stats.attack;
            total.defense += stats.defense;
            total.health += stats.health;
            total.mana += stats.mana;
            total.strength += stats.strength;
            total.intelligence += stats.intelligence;
            total.fireAttack += stats.fireAttack;
            total.waterAttack += stats.waterAttack;
            total.poisonAttack += stats.poisonAttack;
            total.fireResist += stats.fireResist;
            total.waterResist += stats.waterResist;
            total.poisonResist += stats.poisonResist;
        }
    }
    
    return total;
}

bool ItemSystem::equipItem(int inventorySlot) {
    if (inventorySlot < 0 || inventorySlot >= INVENTORY_SIZE) return false;
    if (itemInventory[inventorySlot].isEmpty()) return false;
    
    Item* item = itemInventory[inventorySlot].item;
    if (item->type != ItemType::EQUIPMENT) return false;
    
    int equipSlot = static_cast<int>(item->equipmentType);
    if (equipSlot < 0 || equipSlot >= 9) return false;
    
    // If equipment slot is occupied, swap items
    if (!equipmentSlots[equipSlot].isEmpty()) {
        Item* currentEquipped = equipmentSlots[equipSlot].item;
        equipmentSlots[equipSlot].setItem(item);
        itemInventory[inventorySlot].setItem(currentEquipped);
    } else {
        equipmentSlots[equipSlot].setItem(item);
        itemInventory[inventorySlot].clear();
    }
    
    return true;
}

bool ItemSystem::unequipItem(int equipmentSlot) {
    if (equipmentSlot < 0 || equipmentSlot >= 9) return false;
    if (equipmentSlots[equipmentSlot].isEmpty()) return false;
    
    int emptySlot = findEmptySlot(false);
    if (emptySlot == -1) return false; // Inventory full
    
    Item* item = equipmentSlots[equipmentSlot].item;
    itemInventory[emptySlot].setItem(item);
    equipmentSlots[equipmentSlot].clear();
    
    return true;
}

Texture* ItemSystem::getItemIcon(const std::string& itemId) const {
    auto it = itemIcons.find(itemId);
    if (it != itemIcons.end()) {
        return it->second;
    }
    return nullptr;
}

void ItemSystem::loadEquipmentFromSave(const std::string equipNames[9], const int equipPlus[9], 
                                      const int equipFire[9], const int equipIce[9], 
                                      const int equipLightning[9], const int equipPoison[9],
                                      const int equipRarity[9]) {
    // Clear current equipment
    for (auto& slot : equipmentSlots) {
        slot.clear();
    }
    
    // Load saved equipment
    for (int i = 0; i < 9; ++i) {
        const std::string& itemId = equipNames[i];
        if (!itemId.empty()) {
            // Create item from template with saved rarity
            ItemRarity rarity = static_cast<ItemRarity>(std::clamp(equipRarity[i], 0, 4));
            Item* item = createItem(itemId, rarity, 1);
            if (item) {
                // Set saved +level and elemental stats
                item->plusLevel = std::max(0, equipPlus[i]);
                item->stats.fireAttack += std::max(0, equipFire[i]);
                item->stats.waterAttack += std::max(0, equipIce[i]);
                item->stats.poisonAttack += std::max(0, equipPoison[i]);
                // Note: Lightning not implemented yet
                
                // Equip the item
                equipmentSlots[i].setItem(item);
            }
        }
    }
}

void ItemSystem::loadInventoryFromSave(const std::string invKey[2][9], const int invCnt[2][9], 
                                      const int invRarity[2][9], const int invPlusLevel[2][9]) {
    // Clear current inventories
    for (auto& slot : itemInventory) {
        slot.clear();
    }
    for (auto& slot : scrollInventory) {
        slot.clear();
    }
    
    // Load saved inventory items
    for (int b = 0; b < 2; ++b) {
        for (int i = 0; i < 9; ++i) {
            const std::string& itemId = invKey[b][i];
            int count = invCnt[b][i];
            ItemRarity rarity = static_cast<ItemRarity>(std::clamp(invRarity[b][i], 0, 4));
            
            if (!itemId.empty() && count > 0) {
                // Determine if it's a scroll or regular item
                auto templateIt = itemTemplates.find(itemId);
                if (templateIt != itemTemplates.end()) {
                    bool isScroll = (templateIt->second.type == ItemType::SCROLL);
                    
                    // Create item with correct count and rarity
                    Item* item = createItem(itemId, rarity, count);
                    if (item) {
                        // Set the saved +level
                        item->plusLevel = std::max(0, invPlusLevel[b][i]);
                        
                        // Add to appropriate inventory
                        if (isScroll) {
                            int emptySlot = findEmptySlot(true); // true for scroll inventory
                            if (emptySlot != -1) {
                                scrollInventory[emptySlot].setItem(item);
                            }
                        } else {
                            int emptySlot = findEmptySlot(false); // false for item inventory
                            if (emptySlot != -1) {
                                itemInventory[emptySlot].setItem(item);
                            }
                        }
                    }
                }
            }
        }
    }
}