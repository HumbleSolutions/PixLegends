#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

// Forward declarations
class AssetManager;
class Texture;

enum class ItemType {
    EQUIPMENT,
    SCROLL,
    MATERIAL,
    CONSUMABLE
};

enum class ItemRarity {
    COMMON = 0,    // White
    UNCOMMON = 1,  // Green  
    RARE = 2,      // Blue
    EPIC = 3,      // Purple
    LEGENDARY = 4  // Orange/Gold
};

enum class EquipmentType {
    RING = 0,
    HELMET = 1,
    NECKLACE = 2,
    WEAPON = 3,     // Sword/Bow
    CHEST = 4,
    SHIELD = 5,
    GLOVES = 6,
    WAIST = 7,
    BOOTS = 8
};

struct ItemStats {
    int attack = 0;
    int defense = 0;
    int health = 0;
    int mana = 0;
    int strength = 0;
    int intelligence = 0;
    // Elemental stats
    int fireAttack = 0;
    int waterAttack = 0;
    int poisonAttack = 0;
    int fireResist = 0;
    int waterResist = 0;
    int poisonResist = 0;
};

struct Item {
    std::string id;           // Template identifier (e.g., "rusty_sword")
    std::string instanceId;   // Unique instance identifier for each item copy
    std::string name;         // Display name
    std::string description;  // Tooltip description
    std::string iconPath;     // Path to PNG icon
    ItemType type;
    ItemRarity rarity;
    int stackSize;           // Max stack size (1 for equipment, higher for consumables)
    int currentStack;        // Current stack amount
    
    // Equipment-specific
    EquipmentType equipmentType;
    int plusLevel;           // +0, +1, +2, etc.
    ItemStats stats;         // Base stats
    
    // Constructors
    Item() = default; // Default constructor for containers
    Item(const std::string& itemId, const std::string& itemName, ItemType itemType, 
         ItemRarity itemRarity = ItemRarity::COMMON, int maxStack = 1);
    
    // Get display color based on rarity
    SDL_Color getRarityColor() const;
    
    // Get full display name with +level for equipment
    std::string getDisplayName() const;
    
    // Get detailed tooltip text
    std::string getTooltipText() const;
    
    // Check if item can stack with another
    bool canStackWith(const Item& other) const;
};

// Inventory slot that can hold an item
struct InventorySlot {
    Item* item = nullptr;
    bool isEmpty() const { return item == nullptr; }
    void clear() { item = nullptr; }
    void setItem(Item* newItem) { item = newItem; }
};

class ItemSystem {
public:
    ItemSystem(AssetManager* assetManager);
    ~ItemSystem();
    
    // Item creation and management
    Item* createItem(const std::string& itemId, ItemRarity rarity = ItemRarity::COMMON, int stack = 1);
    Item* getItemTemplate(const std::string& itemId) const;
    
    // Inventory management (separate bags for items and scrolls)
    static constexpr int INVENTORY_ROWS = 6;
    static constexpr int INVENTORY_COLS = 8;
    static constexpr int INVENTORY_SIZE = INVENTORY_ROWS * INVENTORY_COLS;
    static constexpr int SCROLL_INVENTORY_SIZE = 20; // Separate scroll inventory
    
    // Add item to appropriate inventory
    bool addItem(const std::string& itemId, int amount = 1, ItemRarity rarity = ItemRarity::COMMON);
    bool addItemToSlot(Item* item, int slotIndex, bool isScrollInventory = false);
    
    // Remove item from inventory
    bool removeItem(const std::string& itemId, int amount = 1);
    bool removeItemFromSlot(int slotIndex, bool isScrollInventory = false);
    
    // Get inventory contents
    const std::vector<InventorySlot>& getItemInventory() const { return itemInventory; }
    const std::vector<InventorySlot>& getScrollInventory() const { return scrollInventory; }
    
    // Equipment management
    const std::vector<InventorySlot>& getEquipmentSlots() const { return equipmentSlots; }
    bool equipItem(int inventorySlot);
    bool unequipItem(int equipmentSlot);
    bool swapEquipment(int fromSlot, int toSlot); // Swap between inventory and equipment
    
    // Stats calculation
    ItemStats calculateTotalStats() const;
    
    // UI helpers
    Texture* getItemIcon(const std::string& itemId) const;
    
    // Initialize item templates
    void initializeItemTemplates();
    
    // Save/Load support
    void loadEquipmentFromSave(const std::string equipNames[9], const int equipPlus[9], 
                              const int equipFire[9], const int equipIce[9], 
                              const int equipLightning[9], const int equipPoison[9],
                              const int equipRarity[9]);
    void loadInventoryFromSave(const std::string invKey[2][9], const int invCnt[2][9], 
                              const int invRarity[2][9], const int invPlusLevel[2][9]);
    
private:
    AssetManager* assetManager;
    
    // Inventories
    std::vector<InventorySlot> itemInventory;      // Main item inventory (equipment, materials)
    std::vector<InventorySlot> scrollInventory;    // Scroll-only inventory
    std::vector<InventorySlot> equipmentSlots;     // Currently equipped items (9 slots)
    
    // Item templates and instances
    std::unordered_map<std::string, Item> itemTemplates; // Master templates
    std::vector<std::unique_ptr<Item>> itemInstances;    // Actual item instances
    std::unordered_map<std::string, Texture*> itemIcons; // Cached icons
    
    // Instance ID generation
    int nextInstanceId = 1;
    
    // Helper functions
    int findEmptySlot(bool isScrollInventory = false) const;
    int findItemSlot(const std::string& itemId, bool isScrollInventory = false) const;
    bool canAddToSlot(const Item* item, int slotIndex, bool isScrollInventory = false) const;
    float getRarityMultiplier(ItemRarity rarity) const;
    void scaleStatsForRarity(Item& item, float multiplier) const;
    void createItemTemplate(const std::string& id, const std::string& name, const std::string& desc,
                           const std::string& iconPath, ItemType type, EquipmentType equipType = EquipmentType::RING,
                           ItemRarity rarity = ItemRarity::COMMON);
};