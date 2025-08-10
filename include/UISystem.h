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
        int newMaster = 0;
        int newMusic = 0;
        int newSound = 0;
        int newTabIndex = -1; // -1 means unchanged
    };

    void renderOptionsMenu(int selectedIndex,
                           int masterVolume, int musicVolume, int soundVolume,
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
    // Dependency injection
public:
    void setAssetManager(AssetManager* am) { assetManager = am; }
};
