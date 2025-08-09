#pragma once

#include <SDL2/SDL.h>
#include <unordered_map>
#include <array>

enum class InputAction {
    MOVE_UP,
    MOVE_DOWN,
    MOVE_LEFT,
    MOVE_RIGHT,
    ATTACK_MELEE,
    ATTACK_RANGED,
    INTERACT,
    INVENTORY,
    PAUSE,
    QUIT,
    USE_HP_POTION,
    USE_MP_POTION
};

enum class InputState {
    RELEASED,
    PRESSED,
    HELD
};

class InputManager {
public:
    InputManager();
    ~InputManager() = default;

    // Event handling
    void handleKeyDown(const SDL_KeyboardEvent& event);
    void handleKeyUp(const SDL_KeyboardEvent& event);
    void handleMouseDown(const SDL_MouseButtonEvent& event);
    void handleMouseUp(const SDL_MouseButtonEvent& event);
    void handleMouseMotion(const SDL_MouseMotionEvent& event);
    
    // Input state queries
    bool isActionPressed(InputAction action) const;
    bool isActionHeld(InputAction action) const;
    bool isActionReleased(InputAction action) const;
    
    // Mouse state
    int getMouseX() const { return mouseX; }
    int getMouseY() const { return mouseY; }
    bool isMouseButtonPressed(int button) const;
    bool isMouseButtonHeld(int button) const;
    
    // Movement vector (normalized)
    void getMovementVector(float& x, float& y) const;
    
    // Update (called each frame)
    void update();
    
    // Reset states
    void reset();

private:
    // Keyboard state
    std::array<InputState, 256> keyStates;
    std::array<InputState, 256> previousKeyStates;
    
    // Mouse state
    std::array<InputState, 5> mouseButtonStates;
    std::array<InputState, 5> previousMouseButtonStates;
    int mouseX, mouseY;
    int previousMouseX, previousMouseY;
    
    // Action mapping
    std::unordered_map<InputAction, SDL_Scancode> actionToKey;
    std::unordered_map<InputAction, int> actionToMouseButton;
    
    // Helper functions
    void initializeKeyBindings();
    InputState getKeyState(SDL_Scancode scancode) const;
    InputState getMouseButtonState(int button) const;
    void updateKeyState(SDL_Scancode scancode, bool pressed);
    void updateMouseButtonState(int button, bool pressed);
};
