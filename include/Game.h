#pragma once

#include <SDL2/SDL.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

// Forward declarations
class Renderer;
class InputManager;
class AssetManager;
class Player;
class World;
class UISystem;
class AudioManager;

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

    // Game configuration
    static constexpr int WINDOW_WIDTH = 1280;
    static constexpr int WINDOW_HEIGHT = 720;
    static constexpr int TARGET_FPS = 60;
    static constexpr float TARGET_FRAME_TIME = 1.0f / TARGET_FPS;

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
    
    // Game state
    bool isRunning;
    bool isPaused;
    
    // Timing
    Uint32 lastFrameTime;
    float accumulator;
    
    // Initialize systems
    void initializeSystems();
    void initializeObjects();
    void cleanup();
};
