#pragma once

#include <SDL2/SDL.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <deque>

// Forward declarations
class Renderer;
class InputManager;
class AssetManager;
class Player;
class World;
class UISystem;
class AudioManager;
class Database;

// Forward declarations
class Item;

enum class AnvilItemSource {
    NONE,
    EQUIPPED_SLOT,
    INVENTORY_ITEM
};

class Game {
public:
    Game();
    ~Game();

    // Main game loop
    void run();
    
    // Game state management
    void update(float deltaTime);
    void render();
    void handleEvents();
    
    // System access
    Renderer* getRenderer() const { return renderer.get(); }
    InputManager* getInputManager() const { return inputManager.get(); }
    AssetManager* getAssetManager() const { return assetManager.get(); }
    Player* getPlayer() const { return player.get(); }
    World* getWorld() const { return world.get(); }
    UISystem* getUISystem() const { return uiSystem.get(); }
    AudioManager* getAudioManager() const { return audioManager.get(); }
    Database* getDatabase() const { return database.get(); }
    bool isOptionsOpen() const { return optionsOpen; }

    // Magic Anvil UI state
    bool isAnvilOpen() const { return anvilOpen; }
    void openAnvil() { 
        anvilOpen = true; 
        inventoryOpen = true; 
        // Clear anvil state when opening
        anvilSelectedSlot = -1;  // Clear item slot
        anvilStagedScrollKey.clear();  // Clear scroll slot
        anvilTargetItem = nullptr;  // Clear target item
        anvilItemSource = AnvilItemSource::NONE;  // Clear source
    }
    void closeAnvil() { anvilOpen = false; }
    int getAnvilSelectedSlot() const { return anvilSelectedSlot; }
    void setAnvilSelectedSlot(int idx) { anvilSelectedSlot = idx; }
    Item* getAnvilTargetItem() const { return anvilTargetItem; }
    bool isInventoryOpen() const { return inventoryOpen; }
    void toggleInventory() { inventoryOpen = !inventoryOpen; }
    
    // UI position getters/setters for moveable panels
    int getInventoryPosX() const { return inventoryPosX; }
    int getInventoryPosY() const { return inventoryPosY; }
    void setInventoryPos(int x, int y) { inventoryPosX = x; inventoryPosY = y; }
    int getEquipmentPosX() const { return equipmentPosX; }
    int getEquipmentPosY() const { return equipmentPosY; }
    void setEquipmentPos(int x, int y) { equipmentPosX = x; equipmentPosY = y; }
    int getAnvilPosX() const { return anvilPosX; }
    int getAnvilPosY() const { return anvilPosY; }
    void setAnvilPos(int x, int y) { anvilPosX = x; anvilPosY = y; }

    // Game configuration
    static constexpr int WINDOW_WIDTH = 1280;
    static constexpr int WINDOW_HEIGHT = 720;
    static constexpr int TARGET_FPS = 60;
    static constexpr float TARGET_FRAME_TIME = 1.0f / TARGET_FPS;

    // Debug toggles
    void setDebugHitboxes(bool enabled) { debugHitboxes = enabled; }
    bool getDebugHitboxes() const { return debugHitboxes; }

    // Testing/cheat: infinite potions toggle
    void setInfinitePotions(bool enabled) { infinitePotions = enabled; }
    bool getInfinitePotions() const { return infinitePotions; }

    // Performance monitoring
    float getCurrentFPS() const { return currentFPS; }
    float getAverageFPS() const { return averageFPS; }
    Uint32 getFrameTime() const { return frameTime; }

    // World transitions
    void enterUnderworld();
    void exitUnderworld();
    bool isInUnderworld() const { return inUnderworld; }

    // Spawner control
    void setStopMonsterSpawns(bool stop) { stopMonsterSpawns = stop; }
    bool getStopMonsterSpawns() const { return stopMonsterSpawns; }

private:
    // SDL objects
    SDL_Window* window;
    SDL_Renderer* sdlRenderer;
    
    // Game systems
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<InputManager> inputManager;
    std::unique_ptr<AssetManager> assetManager;
    std::unique_ptr<Player> player;
    std::unique_ptr<World> world;
    std::unique_ptr<UISystem> uiSystem;
    std::unique_ptr<AudioManager> audioManager;
    std::unique_ptr<Database> database;

    // Modal UI state
    bool anvilOpen = false;
    bool inventoryOpen = false;
    int anvilSelectedSlot = 3; // default sword
    std::string anvilStagedScrollKey; // scroll currently placed in the anvil scroll slot
    float anvilUpgradeAnimT = 0.0f; // 0..1 loading sweep for success/fail bar
    float anvilResultFlashTimer = 0.0f; // steady display duration for final result
    bool anvilLastSuccess = false; // last upgrade outcome
    // Anvil target item tracking
    Item* anvilTargetItem = nullptr; // specific item instance being upgraded
    AnvilItemSource anvilItemSource = AnvilItemSource::NONE; // where the item comes from
    // Drag state for inventory -> anvil
    bool draggingFromInventory = false;
    std::string draggingPayload;
    // Prevent double processing of equipment events
    bool processingEquipmentEvent = false;
    
    // Game state
    bool isRunning;
    bool isPaused;
    bool optionsOpen = false;
    bool loginScreenActive = true;
    bool inUnderworld = false;
    
    // Enhanced UI state
    bool equipmentOpen = false;
    // UI panel positions for moveable interface
    int inventoryPosX = -1, inventoryPosY = -1;  // -1 means use default positioning
    int equipmentPosX = -1, equipmentPosY = -1;
    int anvilPosX = -1, anvilPosY = -1;
    enum class LoginField { Username, Password, None };
    LoginField loginActiveField = LoginField::Username;
    std::string loginUsername;
    std::string loginPassword;
    std::string loginError;
    bool loginRemember = false;
    bool loginIsAdmin = false;
    int loggedInUserId = -1;
    int optionsSelectedIndex = 0; // 0..5
    std::string currentMusicTrack;
    std::string backgroundMusicName = "main_theme"; // user-selected background theme
    // Boss music post-death handling
    bool bossWasDead = false;
    float bossMusicHoldTimerSec = 0.0f;
    bool bossFadeOutPending = false;
    bool stopMonsterSpawns = false; // options toggle to stop periodic monster spawns
    
    // Timing and performance monitoring
    Uint32 lastFrameTime;
    float accumulator;
    Uint32 frameTime;
    float currentFPS;
    float averageFPS;
    std::deque<float> fpsHistory;
    static constexpr size_t FPS_HISTORY_SIZE = 60; // Store 1 second of FPS data at 60 FPS
    bool debugHitboxes = false;
    bool infinitePotions = false;
    
    // Initialize systems
    void initializeSystems();
    void initializeObjects();
    void cleanup();
    void updatePerformanceMetrics();
    void loadOrCreateDefaultUserAndSave();
    void saveCurrentUserState();

    void renderOptionsMenuOverlay();
    void handleOptionsInput(const SDL_Event& event);
};
