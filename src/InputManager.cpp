#include "InputManager.h"
#include <cmath>

InputManager::InputManager() : mouseX(0), mouseY(0), previousMouseX(0), previousMouseY(0) {
    // Initialize all states to RELEASED
    keyStates.fill(InputState::RELEASED);
    previousKeyStates.fill(InputState::RELEASED);
    mouseButtonStates.fill(InputState::RELEASED);
    previousMouseButtonStates.fill(InputState::RELEASED);
    
    initializeKeyBindings();
}

void InputManager::initializeKeyBindings() {
    // Movement keys (WASD)
    actionToKey[InputAction::MOVE_UP] = SDL_SCANCODE_W;
    actionToKey[InputAction::MOVE_DOWN] = SDL_SCANCODE_S;
    actionToKey[InputAction::MOVE_LEFT] = SDL_SCANCODE_A;
    actionToKey[InputAction::MOVE_RIGHT] = SDL_SCANCODE_D;
    
    // Combat keys
    actionToKey[InputAction::ATTACK_MELEE] = SDL_SCANCODE_UNKNOWN; // mouse left only
    actionToKey[InputAction::ATTACK_RANGED] = SDL_SCANCODE_LSHIFT;
    // Shield toggle/hold
    actionToKey[InputAction::SHIELD] = SDL_SCANCODE_SPACE;
    
    // Interaction keys
    actionToKey[InputAction::INTERACT] = SDL_SCANCODE_E;
    actionToKey[InputAction::INVENTORY] = SDL_SCANCODE_I;
    actionToKey[InputAction::PAUSE] = SDL_SCANCODE_P;
    actionToKey[InputAction::QUIT] = SDL_SCANCODE_ESCAPE;

    // Consumables
    actionToKey[InputAction::USE_HP_POTION] = SDL_SCANCODE_1;
    actionToKey[InputAction::USE_MP_POTION] = SDL_SCANCODE_2;
    
    // Mouse bindings
    actionToMouseButton[InputAction::ATTACK_MELEE] = SDL_BUTTON_LEFT;
    actionToMouseButton[InputAction::ATTACK_RANGED] = SDL_BUTTON_RIGHT;
    actionToMouseButton[InputAction::INTERACT] = SDL_BUTTON_MIDDLE;
}

void InputManager::handleKeyDown(const SDL_KeyboardEvent& event) {
    updateKeyState(event.keysym.scancode, true);
}

void InputManager::handleKeyUp(const SDL_KeyboardEvent& event) {
    updateKeyState(event.keysym.scancode, false);
}

void InputManager::handleMouseDown(const SDL_MouseButtonEvent& event) {
    updateMouseButtonState(event.button, true);
}

void InputManager::handleMouseUp(const SDL_MouseButtonEvent& event) {
    updateMouseButtonState(event.button, false);
}

void InputManager::handleMouseMotion(const SDL_MouseMotionEvent& event) {
    previousMouseX = mouseX;
    previousMouseY = mouseY;
    mouseX = event.x;
    mouseY = event.y;
}

void InputManager::updateKeyState(SDL_Scancode scancode, bool pressed) {
    if (scancode >= 0 && scancode < 256) {
        if (pressed) {
            if (keyStates[scancode] == InputState::RELEASED) {
                keyStates[scancode] = InputState::PRESSED;
            } else {
                keyStates[scancode] = InputState::HELD;
            }
        } else {
            keyStates[scancode] = InputState::RELEASED;
        }
    }
}

void InputManager::updateMouseButtonState(int button, bool pressed) {
    if (button >= 1 && button <= 5) {
        int index = button - 1;
        if (pressed) {
            if (mouseButtonStates[index] == InputState::RELEASED) {
                mouseButtonStates[index] = InputState::PRESSED;
            } else {
                mouseButtonStates[index] = InputState::HELD;
            }
        } else {
            mouseButtonStates[index] = InputState::RELEASED;
        }
    }
}

bool InputManager::isActionPressed(InputAction action) const {
    // Check keyboard
    auto keyIt = actionToKey.find(action);
    if (keyIt != actionToKey.end()) {
        SDL_Scancode scancode = keyIt->second;
        if (scancode >= 0 && scancode < 256) {
            // Check if key was just pressed this frame (current state is PRESSED and previous state was RELEASED)
            if (keyStates[scancode] == InputState::PRESSED && 
                previousKeyStates[scancode] == InputState::RELEASED) {
                return true;
            }
        }
    }
    
    // Check mouse
    auto mouseIt = actionToMouseButton.find(action);
    if (mouseIt != actionToMouseButton.end()) {
        int button = mouseIt->second;
        if (button >= 1 && button <= 5) {
            int index = button - 1;
            // Check if mouse button was just pressed this frame
            if (mouseButtonStates[index] == InputState::PRESSED && 
                previousMouseButtonStates[index] == InputState::RELEASED) {
                return true;
            }
        }
    }
    
    return false;
}

bool InputManager::isActionHeld(InputAction action) const {
    // Check keyboard
    auto keyIt = actionToKey.find(action);
    if (keyIt != actionToKey.end()) {
        InputState state = getKeyState(keyIt->second);
        if (state == InputState::PRESSED || state == InputState::HELD) {
            return true;
        }
    }
    
    // Check mouse
    auto mouseIt = actionToMouseButton.find(action);
    if (mouseIt != actionToMouseButton.end()) {
        InputState state = getMouseButtonState(mouseIt->second);
        if (state == InputState::PRESSED || state == InputState::HELD) {
            return true;
        }
    }
    
    return false;
}

bool InputManager::isActionReleased(InputAction action) const {
    // Check keyboard
    auto keyIt = actionToKey.find(action);
    if (keyIt != actionToKey.end()) {
        SDL_Scancode scancode = keyIt->second;
        if (scancode >= 0 && scancode < 256) {
            if (previousKeyStates[scancode] != InputState::RELEASED && 
                keyStates[scancode] == InputState::RELEASED) {
                return true;
            }
        }
    }
    
    // Check mouse
    auto mouseIt = actionToMouseButton.find(action);
    if (mouseIt != actionToMouseButton.end()) {
        int button = mouseIt->second;
        if (button >= 1 && button <= 5) {
            int index = button - 1;
            if (previousMouseButtonStates[index] != InputState::RELEASED && 
                mouseButtonStates[index] == InputState::RELEASED) {
                return true;
            }
        }
    }
    
    return false;
}

bool InputManager::isMouseButtonPressed(int button) const {
    return getMouseButtonState(button) == InputState::PRESSED;
}

bool InputManager::isMouseButtonHeld(int button) const {
    InputState state = getMouseButtonState(button);
    return state == InputState::PRESSED || state == InputState::HELD;
}

void InputManager::getMovementVector(float& x, float& y) const {
    x = 0.0f;
    y = 0.0f;
    
    if (isActionHeld(InputAction::MOVE_LEFT)) x -= 1.0f;
    if (isActionHeld(InputAction::MOVE_RIGHT)) x += 1.0f;
    if (isActionHeld(InputAction::MOVE_UP)) y -= 1.0f;
    if (isActionHeld(InputAction::MOVE_DOWN)) y += 1.0f;
    
    // Normalize diagonal movement
    if (x != 0.0f && y != 0.0f) {
        float length = std::sqrt(x * x + y * y);
        x /= length;
        y /= length;
    }
}

void InputManager::update() {
    // Update previous states
    previousKeyStates = keyStates;
    previousMouseButtonStates = mouseButtonStates;
    
    // Update pressed states to held
    for (auto& state : keyStates) {
        if (state == InputState::PRESSED) {
            state = InputState::HELD;
        }
    }
    
    for (auto& state : mouseButtonStates) {
        if (state == InputState::PRESSED) {
            state = InputState::HELD;
        }
    }
}

void InputManager::reset() {
    keyStates.fill(InputState::RELEASED);
    previousKeyStates.fill(InputState::RELEASED);
    mouseButtonStates.fill(InputState::RELEASED);
    previousMouseButtonStates.fill(InputState::RELEASED);
}

InputState InputManager::getKeyState(SDL_Scancode scancode) const {
    if (scancode >= 0 && scancode < 256) {
        return keyStates[scancode];
    }
    return InputState::RELEASED;
}

InputState InputManager::getMouseButtonState(int button) const {
    if (button >= 1 && button <= 5) {
        return mouseButtonStates[button - 1];
    }
    return InputState::RELEASED;
}
