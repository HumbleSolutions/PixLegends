#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <memory>
#include <string>

// Forward declarations
class Renderer;
class Player;
class AssetManager;

class UISystem {
public:
    explicit UISystem(SDL_Renderer* renderer);
    ~UISystem() = default;

    // Core functions
    void update(float deltaTime);
    void render();
    
    // UI elements
    void renderHealthBar(int x, int y, int width, int height, int current, int max);
    void renderManaBar(int x, int y, int width, int height, int current, int max);
    void renderExperienceBar(int x, int y, int width, int height, int current, int max);
    void renderBossHealthBar(const std::string& name, int current, int max, int screenW);
    void renderGoldDisplay(int x, int y, int gold);
    void renderPlayerStats(const Player* player);
    void renderDashCooldown(const Player* player);
    void renderDebugInfo(const Player* player);
    void renderPotions(const Player* player);
    
    // Text rendering
    void renderText(const std::string& text, int x, int y, SDL_Color color = {255, 255, 255, 255});
    void renderTextCentered(const std::string& text, int x, int y, SDL_Color color = {255, 255, 255, 255});
    void renderInteractionPrompt(const std::string& prompt, int x, int y);
    void renderLootNotification(const std::string& lootText, int x, int y);
    void renderFPSCounter(float currentFPS, float averageFPS, Uint32 frameTime);
    // Death popup
    void renderDeathPopup(bool& outClickedRespawn, float fadeAlpha01);
    
    // Enhanced inventory UI
    struct InventoryHit {
        bool clickedClose = false;
        int clickedItemSlot = -1;      // Item inventory slot
        int clickedScrollSlot = -1;    // Scroll inventory slot  
        int clickedEquipSlot = -1;     // Equipment slot
        bool rightClicked = false;     // For tooltips/context menus
        bool dragging = false;         // Drag operation in progress
        bool startedDrag = false;      // For legacy compatibility
        std::string dragPayload;       // For legacy compatibility
        std::string rightClickedPayload; // For legacy compatibility
        // Panel dragging
        bool titleBarClicked = false;  // True if title bar was clicked for dragging panel
        int dragOffsetX = 0, dragOffsetY = 0;  // Offset from panel top-left to mouse
    };
    
    InventoryHit renderEnhancedInventory(class Player* player, bool& isOpen, bool anvilOpen = false, class Game* game = nullptr, bool equipmentOpen = false);
    
    // Equipment/Stats UI (U key)
    struct EquipmentHit {
        bool clickedClose = false;
        int clickedEquipSlot = -1;
        bool rightClicked = false;
        // Panel dragging
        bool titleBarClicked = false;  // True if title bar was clicked for dragging panel
        int dragOffsetX = 0, dragOffsetY = 0;  // Offset from panel top-left to mouse
    };
    
    EquipmentHit renderEquipmentUI(class Player* player, bool& isOpen, bool anvilOpen = false, class Game* game = nullptr, bool inventoryOpen = false);
    void renderTooltip(const std::string& tooltipText, int mouseX, int mouseY);

    // Equipment upgrade UI (Magic Anvil)
    struct AnvilHit {
        bool clickedClose = false;
        int clickedSlot = -1; // 0..8 corresponds to ring..feet
        bool clickedUpgrade = false; // upgrade button or drop action
        std::string clickedElement; // e.g., "fire", "ice", "lightning", "poison"
        // Drag/drop
        bool startedDrag = false;
        bool endedDrag = false;
        SDL_Rect dragRect{0,0,0,0};
        std::string dragPayload; // "upgrade_scroll" or element key
        // Side panel hit tests
        bool clickedSideUpgrade = false;
        bool droppedItem = false; // dropped item into side item slot
        bool droppedScroll = false; // dropped scroll into side scroll slot
        std::string droppedScrollKey; // which scroll was dropped ("upgrade_scroll" or element)
        // Panel dragging
        bool titleBarClicked = false;  // True if title bar was clicked for dragging panel
        int dragOffsetX = 0, dragOffsetY = 0;  // Offset from panel top-left to mouse
    };
    // Renders the item grid and returns what was clicked based on mouse state
    void renderMagicAnvil(const class Player* player,
                          int screenW, int screenH,
                          int mouseX, int mouseY, bool mouseDown,
                          AnvilHit& outHit,
                          const std::string& externalDragPayload = std::string(),
                          int selectedSlotIdx = -1,
                          const std::string& selectedScrollKey = std::string(),
                          class Game* game = nullptr);

    void setAssetManager(AssetManager* am) { assetManager = am; }

    void renderInventory(const class Player* player, int screenW, int screenH,
                         int mouseX, int mouseY, bool leftDown, bool rightDown,
                         InventoryHit& outHit);

    // Options menu (simple keyboard-driven UI). selectedIndex highlights current row.
    enum class OptionsTab { Main = 0, Sound = 1, Video = 2 };
    struct MenuHitResult {
        bool clickedResume = false;
        bool clickedFullscreen = false;
        bool clickedLogout = false;
        bool clickedReset = false;
        bool changedMaster = false;
        bool changedMusic = false;
        bool changedSound = false;
        bool changedMonster = false;
        bool changedPlayer = false;
        int newMaster = 0;
        int newMusic = 0;
        int newSound = 0;
        int newMonster = 0;
        int newPlayer = 0;
        int newThemeIndex = -1; // 0 = main, 1 = fast tempo
        int newTabIndex = -1; // -1 means unchanged
        bool toggledStopSpawns = false;
        bool stopSpawnsNewValue = false;
    };

    void renderOptionsMenu(int selectedIndex,
                           int masterVolume, int musicVolume, int soundVolume,
                           int monsterVolume, int playerMeleeVolume,
                           bool fullscreenEnabled, bool vsyncEnabled,
                           OptionsTab activeTab,
                           // mouse for hit testing
                           int mouseX, int mouseY, bool mouseDown,
                           // output
                           MenuHitResult& outResult);
    
private:
    SDL_Renderer* renderer;
    TTF_Font* defaultFont;
    TTF_Font* smallFont;
    AssetManager* assetManager = nullptr;
    // Potion icon animation state
    float potionAnimTimer = 0.0f;
    int potionAnimFrame = 0;
    float potionFrameDuration = 0.15f; // seconds per frame
    
    // UI colors
    SDL_Color healthColor;
    SDL_Color manaColor;
    SDL_Color experienceColor;
    SDL_Color backgroundColor;
    SDL_Color textColor;
    
    // Helper functions
    void initializeFonts();
    void initializeColors();
    std::string formatStatText(const std::string& label, int value, int maxValue = 0);
    void renderTexturedBar(const char* texturePath,
                           int x, int y,
                           int width, int height,
                           float progress);
    // Dependency injection
    // setAssetManager declared above
};
