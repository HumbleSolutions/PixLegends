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
class AutotileDemo;
class Database;

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

    // Magic Anvil UI state
    bool isAnvilOpen() const { return anvilOpen; }
    void openAnvil() { anvilOpen = true; inventoryOpen = true; }
    void closeAnvil() { anvilOpen = false; }
    int getAnvilSelectedSlot() const { return anvilSelectedSlot; }
    void setAnvilSelectedSlot(int idx) { anvilSelectedSlot = idx; }
    bool isInventoryOpen() const { return inventoryOpen; }
    void toggleInventory() { inventoryOpen = !inventoryOpen; }

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
    std::unique_ptr<AutotileDemo> autotileDemo;
    std::unique_ptr<Database> database;

    // Modal UI state
    bool anvilOpen = false;
    bool inventoryOpen = false;
    int anvilSelectedSlot = 3; // default sword
    std::string anvilStagedScrollKey; // scroll currently placed in the anvil scroll slot
    float anvilUpgradeAnimT = 0.0f; // 0..1 loading sweep for success/fail bar
    float anvilResultFlashTimer = 0.0f; // steady display duration for final result
    bool anvilLastSuccess = false; // last upgrade outcome
    // Drag state for inventory -> anvil
    bool draggingFromInventory = false;
    std::string draggingPayload;
    bool demoMode = false;
    
    // Game state
    bool isRunning;
    bool isPaused;
    bool optionsOpen = false;
    bool loginScreenActive = true;
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
    // Boss music post-death handling
    bool bossWasDead = false;
    float bossMusicHoldTimerSec = 0.0f;
    bool bossFadeOutPending = false;
    
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
