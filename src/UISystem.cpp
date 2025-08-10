#include "UISystem.h"
#include "Renderer.h"
#include "Player.h"
#include "AssetManager.h"
#include <iostream>
#include <sstream>

UISystem::UISystem(SDL_Renderer* renderer) : renderer(renderer), defaultFont(nullptr), smallFont(nullptr) {
    initializeFonts();
    initializeColors();
}

void UISystem::initializeFonts() {
    // Prefer project font in assets/Fonts/
    const char* fontPaths[] = {
        "assets/Fonts/retganon.ttf",
        "assets/fonts/retganon.ttf", // case variant fallback
        // System fallbacks (only if project font is missing)
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/calibri.ttf"
    };
    
    defaultFont = nullptr;
    smallFont = nullptr;
    
    for (const char* path : fontPaths) {
        if (!defaultFont) {
            defaultFont = TTF_OpenFont(path, 16);
            if (defaultFont) {
                std::cout << "Loaded default font from: " << path << std::endl;
            }
        }
        if (!smallFont) {
            smallFont = TTF_OpenFont(path, 12);
            if (smallFont) {
                std::cout << "Loaded small font from: " << path << std::endl;
            }
        }
        if (defaultFont && smallFont) break;
    }
    
    if (!defaultFont) {
        std::cerr << "Warning: Could not load default font. Text rendering will be disabled." << std::endl;
    }
    if (!smallFont) {
        std::cerr << "Warning: Could not load small font. Small text rendering will be disabled." << std::endl;
    }
}

void UISystem::initializeColors() {
    healthColor = {255, 0, 0, 255};      // Red
    manaColor = {0, 0, 255, 255};        // Blue
    experienceColor = {255, 215, 0, 255}; // Gold
    backgroundColor = {50, 50, 50, 255};  // Dark gray
    textColor = {255, 255, 255, 255};     // White
}

void UISystem::update(float deltaTime) {
    // Advance simple UI animations (e.g., potion icon flicker)
    potionAnimTimer += deltaTime;
    if (potionAnimTimer >= potionFrameDuration) {
        potionAnimTimer = 0.0f;
        // Cycle 0..7 for animated potions with remaining charges
        potionAnimFrame = (potionAnimFrame + 1) % 8;
    }
}

void UISystem::render() {
    // Main UI rendering - this will be called by the game
}

void UISystem::renderHealthBar(int x, int y, int width, int height, int current, int max) {
    if (max <= 0) return;
    
    float progress = static_cast<float>(current) / static_cast<float>(max);
    
    // Create a simple renderer wrapper for this UI system
    Renderer rendererWrapper(renderer);
    rendererWrapper.renderProgressBar(x, y, width, height, progress, backgroundColor, healthColor);
    
    // Render text
    std::string text = std::to_string(current) + "/" + std::to_string(max);
    renderTextCentered(text, x + width/2, y + height/2, textColor);
}

void UISystem::renderManaBar(int x, int y, int width, int height, int current, int max) {
    if (max <= 0) return;
    
    float progress = static_cast<float>(current) / static_cast<float>(max);
    
    Renderer rendererWrapper(renderer);
    rendererWrapper.renderProgressBar(x, y, width, height, progress, backgroundColor, manaColor);
    
    // Render text
    std::string text = std::to_string(current) + "/" + std::to_string(max);
    renderTextCentered(text, x + width/2, y + height/2, textColor);
}

void UISystem::renderExperienceBar(int x, int y, int width, int height, int current, int max) {
    if (max <= 0) return;
    
    float progress = static_cast<float>(current) / static_cast<float>(max);
    
    Renderer rendererWrapper(renderer);
    rendererWrapper.renderProgressBar(x, y, width, height, progress, backgroundColor, experienceColor);
    
    // Render text
    std::string text = "XP: " + std::to_string(current) + "/" + std::to_string(max);
    renderTextCentered(text, x + width/2, y + height/2, textColor);
}

void UISystem::renderBossHealthBar(const std::string& name, int current, int max, int screenW) {
    if (max <= 0) return;
    int barWidth = std::min(640, std::max(240, screenW - 300));
    int barHeight = 18;
    int x = screenW / 2 - barWidth / 2;
    int y = 32;
    float progress = static_cast<float>(std::max(0, current)) / static_cast<float>(max);

    // Background
    SDL_Rect bg{ x, y, barWidth, barHeight };
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 220);
    SDL_RenderFillRect(renderer, &bg);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &bg);

    // Foreground (boss color)
    SDL_Color bossRed{200, 20, 20, 255};
    SDL_Rect fg{ x + 2, y + 2, static_cast<int>((barWidth - 4) * progress), barHeight - 4 };
    SDL_SetRenderDrawColor(renderer, bossRed.r, bossRed.g, bossRed.b, bossRed.a);
    SDL_RenderFillRect(renderer, &fg);

    // Text
    std::string text = name + "  " + std::to_string(current) + "/" + std::to_string(max);
    renderTextCentered(text, x + barWidth / 2, y + barHeight / 2, SDL_Color{255,255,255,255});
}

void UISystem::renderGoldDisplay(int x, int y, int gold) {
    // Create a gold-colored background
    SDL_Color goldBgColor = {255, 215, 0, 200}; // Semi-transparent gold
    SDL_Color goldTextColor = {0, 0, 0, 255}; // Black text for contrast
    
    // Get text dimensions
    std::string goldText = "Gold: " + std::to_string(gold);
    int textWidth, textHeight;
    if (defaultFont) {
        TTF_SizeText(defaultFont, goldText.c_str(), &textWidth, &textHeight);
    } else {
        textWidth = goldText.length() * 8; // Approximate width
        textHeight = 16;
    }
    
    // Add padding
    int padding = 6;
    int bgWidth = textWidth + padding * 2;
    int bgHeight = textHeight + padding * 2;
    
    // Render background rectangle
    SDL_Rect bgRect = {x, y, bgWidth, bgHeight};
    SDL_SetRenderDrawColor(renderer, goldBgColor.r, goldBgColor.g, goldBgColor.b, goldBgColor.a);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderFillRect(renderer, &bgRect);
    
    // Render border
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &bgRect);
    
    // Render text
    renderText(goldText, x + padding, y + padding, goldTextColor);
}

void UISystem::renderPlayerStats(const Player* player) {
    if (!player) return;
    
    int startX = 10;
    int startY = 10;
    int barWidth = 200;
    int barHeight = 20;
    int spacing = 25;
    
    // Health bar
    renderHealthBar(startX, startY, barWidth, barHeight, 
                   player->getHealth(), player->getMaxHealth());
    
    // Mana bar
    renderManaBar(startX, startY + spacing, barWidth, barHeight,
                 player->getMana(), player->getMaxMana());
    
    // Experience bar
    renderExperienceBar(startX, startY + spacing * 2, barWidth, barHeight,
                       player->getExperience(), player->getExperienceToNext());
    
    // Gold display
    renderGoldDisplay(startX, startY + spacing * 3, player->getGold());
    
    // Level and stats
    int textX = startX + barWidth + 10;
    int textY = startY;
    
    renderText("Level: " + std::to_string(player->getLevel()), textX, textY);
    renderText("STR: " + std::to_string(player->getStrength()), textX, textY + 20);
    renderText("INT: " + std::to_string(player->getIntelligence()), textX, textY + 40);

    // Render potions HUD beneath stats
    renderPotions(player);
}

void UISystem::renderDebugInfo(const Player* player) {
    if (!player) return;
    
    int startX = 10;
    int startY = 200;
    
    renderText("Debug Info:", startX, startY);
    renderText("Position: (" + std::to_string(static_cast<int>(player->getX())) + 
               ", " + std::to_string(static_cast<int>(player->getY())) + ")", startX, startY + 20);
    
    std::string stateStr;
    switch (player->getState()) {
        case PlayerState::IDLE: stateStr = "IDLE"; break;
        case PlayerState::WALKING: stateStr = "WALKING"; break;
        case PlayerState::ATTACKING_MELEE: stateStr = "MELEE ATTACK"; break;
        case PlayerState::ATTACKING_RANGED: stateStr = "RANGED ATTACK"; break;
        case PlayerState::HURT: stateStr = "HURT"; break;
        case PlayerState::DEAD: stateStr = "DEAD"; break;
        default: stateStr = "UNKNOWN"; break;
    }
    renderText("State: " + stateStr, startX, startY + 40);
}

void UISystem::renderPotions(const Player* player) {
    if (!player) return;
    Renderer rendererWrapper(renderer);
    const int iconSize = 32;
    const int margin = 10;
    int outW = 0, outH = 0;
    if (renderer) {
        SDL_GetRendererOutputSize(renderer, &outW, &outH);
    }
    const int startX = margin; // left margin
    const int startY = (outH > 0 ? outH : 720) - iconSize - margin; // bottom-left anchor
    
    // Determine which sprite sheet to show based on remaining charges
    auto pickPotionSprite = [&](int charges, int maxCharges, bool isHealth, SDL_Texture*& outTexture, SDL_Rect& outSrcRect) {
        if (!assetManager) return;
        float ratio = maxCharges > 0 ? static_cast<float>(charges) / static_cast<float>(maxCharges) : 0.0f;
        if (charges <= 0) {
            // Empty is a single-frame image
            Texture* t = assetManager->getTexture(isHealth ? "assets/Textures/All Potions/HP potions/empty.png"
                                                           : "assets/Textures/All Potions/Mana potion/empty.png");
            if (t) {
                outTexture = t->getTexture();
                outSrcRect = {0,0,t->getWidth(), t->getHeight()};
            }
            return;
        }
        // Use 8-frame sheet depending on ratio: low/half/full variants
        const char* path = nullptr;
        if (ratio <= 0.3f) {
            path = isHealth ? "assets/Textures/All Potions/HP potions/low_hp_potion.png"
                            : "assets/Textures/All Potions/Mana potion/low_mana_potion.png";
        } else if (ratio <= 0.6f) {
            path = isHealth ? "assets/Textures/All Potions/HP potions/half_hp_potion.png"
                            : "assets/Textures/All Potions/Mana potion/half_mana_potion.png";
        } else {
            path = isHealth ? "assets/Textures/All Potions/HP potions/full_hp_potion.png"
                            : "assets/Textures/All Potions/Mana potion/full_mana_potion.png";
        }
        SpriteSheet* sheet = assetManager->loadSpriteSheetAuto(path, 8, 8);
        if (sheet) {
            outTexture = sheet->getTexture()->getTexture();
            SDL_Rect frameRect = sheet->getFrameRect(potionAnimFrame);
            outSrcRect = frameRect;
        }
    };
    
    // Load textures via SDL directly using AssetManager for caching if available
    auto renderPotion = [&](bool isHealth, int x, int y, const char* label, int charges, int maxCharges) {
        SDL_Texture* tex = nullptr;
        SDL_Rect src{0,0,0,0};
        pickPotionSprite(charges, maxCharges, isHealth, tex, src);
        SDL_Rect dst{ x, y, iconSize, iconSize };
        if (tex) {
            SDL_RenderCopy(renderer, tex, (src.w > 0 ? &src : nullptr), &dst);
        } else {
            SDL_Color c = isHealth ? SDL_Color{200,0,0,255} : SDL_Color{0,0,200,255};
            rendererWrapper.renderRect(dst, c, true);
        }
        // draw label and charges
        renderText(std::string(label), x + iconSize + 6, y + 4, textColor);
        renderText("x" + std::to_string(charges), x + iconSize + 6, y + 18, textColor);
    };
    
    renderPotion(true, startX, startY, "[1] HP", player->getEffectiveHealthPotionCharges(), player->getMaxPotionCharges());
    renderPotion(false, startX + 160, startY, "[2] MP", player->getEffectiveManaPotionCharges(), player->getMaxPotionCharges());
}

void UISystem::renderText(const std::string& text, int x, int y, SDL_Color color) {
    if (!defaultFont) return;
    
    Renderer rendererWrapper(renderer);
    rendererWrapper.renderText(text, defaultFont, x, y, color);
}

void UISystem::renderTextCentered(const std::string& text, int x, int y, SDL_Color color) {
    if (!defaultFont) return;
    
    Renderer rendererWrapper(renderer);
    rendererWrapper.renderTextCentered(text, defaultFont, x, y, color);
}

void UISystem::renderInteractionPrompt(const std::string& prompt, int x, int y) {
    if (prompt.empty() || !defaultFont) return;
    
    // Create a background rectangle for the prompt
    SDL_Color promptBgColor = {0, 0, 0, 180}; // Semi-transparent black
    SDL_Color promptTextColor = {255, 255, 255, 255}; // White text
    
    // Get text dimensions
    int textWidth, textHeight;
    TTF_SizeText(defaultFont, prompt.c_str(), &textWidth, &textHeight);
    
    // Add padding
    int padding = 8;
    int bgWidth = textWidth + padding * 2;
    int bgHeight = textHeight + padding * 2;
    
    // Render background rectangle
    SDL_Rect bgRect = {x - bgWidth/2, y - bgHeight/2, bgWidth, bgHeight};
    SDL_SetRenderDrawColor(renderer, promptBgColor.r, promptBgColor.g, promptBgColor.b, promptBgColor.a);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderFillRect(renderer, &bgRect);
    
    // Render border
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &bgRect);
    
    // Render text
    renderTextCentered(prompt, x, y, promptTextColor);
}

void UISystem::renderLootNotification(const std::string& lootText, int x, int y) {
    if (lootText.empty() || !defaultFont) return;
    
    // Create a green background for loot notifications
    SDL_Color lootBgColor = {0, 255, 0, 200}; // Semi-transparent green
    SDL_Color lootTextColor = {0, 0, 0, 255}; // Black text for contrast
    
    // Get text dimensions
    int textWidth, textHeight;
    TTF_SizeText(defaultFont, lootText.c_str(), &textWidth, &textHeight);
    
    // Add padding
    int padding = 6;
    int bgWidth = textWidth + padding * 2;
    int bgHeight = textHeight + padding * 2;
    
    // Render background rectangle
    SDL_Rect bgRect = {x - bgWidth/2, y - bgHeight/2, bgWidth, bgHeight};
    SDL_SetRenderDrawColor(renderer, lootBgColor.r, lootBgColor.g, lootBgColor.b, lootBgColor.a);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderFillRect(renderer, &bgRect);
    
    // Render border
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &bgRect);
    
    // Render text
    renderTextCentered(lootText, x, y, lootTextColor);
}

std::string UISystem::formatStatText(const std::string& label, int value, int maxValue) {
    if (maxValue > 0) {
        return label + ": " + std::to_string(value) + "/" + std::to_string(maxValue);
    } else {
        return label + ": " + std::to_string(value);
    }
}

void UISystem::renderFPSCounter(float currentFPS, float averageFPS, Uint32 frameTime) {
    // Position the FPS counter in the top-right corner
    int x = 1200; // Right side of screen
    int y = 20;   // Top of screen
    
    // Create FPS text
    std::string fpsText = "FPS: " + std::to_string(static_cast<int>(currentFPS)) + 
                          " | Avg: " + std::to_string(static_cast<int>(averageFPS)) + 
                          " | Frame: " + std::to_string(frameTime) + "ms";
    
    // Choose color based on performance
    SDL_Color color;
    if (currentFPS >= 55) {
        color = {0, 255, 0, 255}; // Green for good performance
    } else if (currentFPS >= 30) {
        color = {255, 255, 0, 255}; // Yellow for acceptable performance
    } else {
        color = {255, 0, 0, 255}; // Red for poor performance
    }
    
    // Render the FPS counter
    renderText(fpsText, x, y, color);
}

void UISystem::renderOptionsMenu(int selectedIndex,
                           int masterVolume, int musicVolume, int soundVolume,
                           bool fullscreenEnabled, bool vsyncEnabled,
                           OptionsTab activeTab,
                           int mouseX, int mouseY, bool mouseDown,
                           MenuHitResult& outResult) {
    if (!defaultFont) return;
    int outW = 0, outH = 0;
    if (renderer) SDL_GetRendererOutputSize(renderer, &outW, &outH);
    if (outW <= 0) { outW = 1280; outH = 720; }

    // Dim background
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
    SDL_Rect dim{0,0,outW,outH};
    SDL_RenderFillRect(renderer, &dim);

    // Panel
    const int panelW = 600, panelH = 400;
    SDL_Rect panel{ outW/2 - panelW/2, outH/2 - panelH/2, panelW, panelH };
    SDL_SetRenderDrawColor(renderer, 32, 32, 48, 235);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 200, 200, 220, 255);
    SDL_RenderDrawRect(renderer, &panel);

    // Title
    renderTextCentered("Options", outW/2, panel.y + 30, SDL_Color{255,255,255,255});

    // Tabs: Main | Sound | Video
    const int tabsY = panel.y + 58;
    const int tabW = 110, tabH = 24;
    const char* tabNames[3] = { "Main", "Sound", "Video" };
    for (int t = 0; t < 3; ++t) {
        SDL_Rect tr{ panel.x + 20 + t * (tabW + 8), tabsY, tabW, tabH };
        bool hovered = (mouseX >= tr.x && mouseX <= tr.x + tr.w && mouseY >= tr.y && mouseY <= tr.y + tr.h);
        bool active = (static_cast<int>(activeTab) == t);
        SDL_SetRenderDrawColor(renderer, active ? 80 : 50, active ? 140 : 70, active ? 200 : 90, 230);
        SDL_RenderFillRect(renderer, &tr);
        SDL_SetRenderDrawColor(renderer, 220,220,220,255);
        SDL_RenderDrawRect(renderer, &tr);
        renderTextCentered(tabNames[t], tr.x + tr.w/2, tr.y + tr.h/2, SDL_Color{255,255,255,255});
        if (hovered && mouseDown && !active) outResult.newTabIndex = t;
    }

    const int startY = panel.y + 100;
    const int rowH = 44;
    if (activeTab == OptionsTab::Main) {
        // Main: Resume, Reset, Logout
        for (int i = 0; i < 3; ++i) {
            SDL_Rect rowRect{ panel.x + 30, startY + i * rowH, panelW - 60, rowH - 6 };
            bool selected = (mouseX >= rowRect.x && mouseX <= rowRect.x + rowRect.w && mouseY >= rowRect.y && mouseY <= rowRect.y + rowRect.h);
            SDL_SetRenderDrawColor(renderer, selected ? 70 : 50, selected ? 120 : 50, selected ? 180 : 70, 200);
            SDL_RenderFillRect(renderer, &rowRect);
            SDL_SetRenderDrawColor(renderer, 220,220,220,255);
            SDL_RenderDrawRect(renderer, &rowRect);
            SDL_Rect action{ rowRect.x + rowRect.w - 220, rowRect.y + 8, 200, rowRect.h - 16 };
            if (i == 0) {
                renderText("Resume", rowRect.x + 10, rowRect.y + 12, SDL_Color{255,255,255,255});
                SDL_SetRenderDrawColor(renderer, 90, 160, 90, 255);
                SDL_RenderFillRect(renderer, &action);
                SDL_SetRenderDrawColor(renderer, 255,255,255,255);
                SDL_RenderDrawRect(renderer, &action);
                renderText("Resume", action.x + 12, action.y + 8, SDL_Color{255,255,255,255});
                if (mouseDown && mouseX >= action.x && mouseX <= action.x + action.w && mouseY >= action.y && mouseY <= action.y + action.h) outResult.clickedResume = true;
            } else if (i == 1) {
                renderText("Reset", rowRect.x + 10, rowRect.y + 12, SDL_Color{255,255,255,255});
                SDL_SetRenderDrawColor(renderer, 200, 140, 60, 255);
                SDL_RenderFillRect(renderer, &action);
                SDL_SetRenderDrawColor(renderer, 255,255,255,255);
                SDL_RenderDrawRect(renderer, &action);
                renderText("Reset", action.x + 12, action.y + 8, SDL_Color{255,255,255,255});
                if (mouseDown && mouseX >= action.x && mouseX <= action.x + action.w && mouseY >= action.y && mouseY <= action.y + action.h) outResult.clickedReset = true;
            } else {
                renderText("Logout", rowRect.x + 10, rowRect.y + 12, SDL_Color{255,255,255,255});
                SDL_SetRenderDrawColor(renderer, 170, 70, 70, 255);
                SDL_RenderFillRect(renderer, &action);
                SDL_SetRenderDrawColor(renderer, 255,255,255,255);
                SDL_RenderDrawRect(renderer, &action);
                renderText("Logout", action.x + 12, action.y + 8, SDL_Color{255,255,255,255});
                if (mouseDown && mouseX >= action.x && mouseX <= action.x + action.w && mouseY >= action.y && mouseY <= action.y + action.h) outResult.clickedLogout = true;
            }
        }
    } else if (activeTab == OptionsTab::Sound) {
        // Sound: Master, Music, Sound sliders
        for (int i = 0; i < 3; ++i) {
            SDL_Rect rowRect{ panel.x + 30, startY + i * rowH, panelW - 60, rowH - 6 };
            bool selected = (mouseX >= rowRect.x && mouseX <= rowRect.x + rowRect.w && mouseY >= rowRect.y && mouseY <= rowRect.y + rowRect.h);
            SDL_SetRenderDrawColor(renderer, selected ? 70 : 50, selected ? 120 : 50, selected ? 180 : 70, 200);
            SDL_RenderFillRect(renderer, &rowRect);
            SDL_SetRenderDrawColor(renderer, 220,220,220,255);
            SDL_RenderDrawRect(renderer, &rowRect);
            const char* labels[3] = { "Master Volume", "Music Volume", "Sound Volume" };
            renderText(labels[i], rowRect.x + 10, rowRect.y + 12);
            SDL_Rect valueArea{ rowRect.x + rowRect.w - 320, rowRect.y + 8, 300, rowRect.h - 16 };
            // Draw slider
            int v = (i == 0 ? masterVolume : (i == 1 ? musicVolume : soundVolume));
            v = std::max(0, std::min(100, v));
            SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
            SDL_RenderFillRect(renderer, &valueArea);
            SDL_SetRenderDrawColor(renderer, 120, 120, 120, 255);
            SDL_RenderDrawRect(renderer, &valueArea);
            int knobW = 12;
            float t = v / 100.0f;
            int knobX = valueArea.x + static_cast<int>(t * (valueArea.w - knobW));
            SDL_Rect knob{ knobX, valueArea.y, knobW, valueArea.h };
            SDL_SetRenderDrawColor(renderer, 200, 200, 240, 255);
            SDL_RenderFillRect(renderer, &knob);
            bool hovering = (mouseX >= valueArea.x && mouseX <= valueArea.x + valueArea.w && mouseY >= valueArea.y && mouseY <= valueArea.y + valueArea.h);
            if (hovering && mouseDown) {
                float rel = (mouseX - valueArea.x) / static_cast<float>(valueArea.w - knobW);
                int newVal = std::max(0, std::min(100, static_cast<int>(rel * 100.0f)));
                if (i == 0) { outResult.changedMaster = true; outResult.newMaster = newVal; }
                if (i == 1) { outResult.changedMusic = true;  outResult.newMusic  = newVal; }
                if (i == 2) { outResult.changedSound = true;  outResult.newSound  = newVal; }
            }
        }
    } else if (activeTab == OptionsTab::Video) {
        // Video: Fullscreen and VSync toggles
        const int rows = 2;
        for (int i = 0; i < rows; ++i) {
            SDL_Rect rowRect{ panel.x + 30, startY + i * rowH, panelW - 60, rowH - 6 };
            bool selected = (mouseX >= rowRect.x && mouseX <= rowRect.x + rowRect.w && mouseY >= rowRect.y && mouseY <= rowRect.y + rowRect.h);
            SDL_SetRenderDrawColor(renderer, selected ? 70 : 50, selected ? 120 : 50, selected ? 180 : 70, 200);
            SDL_RenderFillRect(renderer, &rowRect);
            SDL_SetRenderDrawColor(renderer, 220,220,220,255);
            SDL_RenderDrawRect(renderer, &rowRect);
            const char* labels[2] = { "Fullscreen", "VSync" };
            renderText(labels[i], rowRect.x + 10, rowRect.y + 12);
            SDL_Rect valueArea{ rowRect.x + rowRect.w - 220, rowRect.y + 8, 200, rowRect.h - 16 };
            SDL_SetRenderDrawColor(renderer, 80, 120, 160, 255);
            SDL_RenderFillRect(renderer, &valueArea);
            SDL_SetRenderDrawColor(renderer, 255,255,255,255);
            SDL_RenderDrawRect(renderer, &valueArea);
            renderText((i == 0 ? (fullscreenEnabled ? "On" : "Off") : (vsyncEnabled ? "On" : "Off")), valueArea.x + 12, valueArea.y + 8);
            if (mouseDown && mouseX >= valueArea.x && mouseX <= valueArea.x + valueArea.w && mouseY >= valueArea.y && mouseY <= valueArea.y + valueArea.h) {
                if (i == 0) outResult.clickedFullscreen = true;
                // VSync is fixed at creation with present vsync, so no toggle here for now
            }
        }
    }

    // Place helper text near the bottom, below the last row
    renderTextCentered("Use mouse to drag sliders and click buttons. Press Esc to close.",
                       outW/2, panel.y + panelH - 12, SDL_Color{200,200,200,255});
}

void UISystem::renderDeathPopup(bool& outClickedRespawn, float fadeAlpha01) {
    outClickedRespawn = false;
    if (!defaultFont) return;
    int outW = 0, outH = 0;
    if (renderer) {
        SDL_GetRendererOutputSize(renderer, &outW, &outH);
    } else {
        outW = 1280; outH = 720;
    }
    // Darken background
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    Uint8 overlayAlpha = static_cast<Uint8>(std::max(0.0f, std::min(1.0f, fadeAlpha01)) * 200.0f);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, overlayAlpha);
    SDL_Rect dim = {0,0,outW,outH};
    SDL_RenderFillRect(renderer, &dim);

    // Popup panel
    int panelW = 420, panelH = 220;
    SDL_Rect panel = { outW/2 - panelW/2, outH/2 - panelH/2, panelW, panelH };
    Uint8 panelAlpha = static_cast<Uint8>(std::max(0.0f, std::min(1.0f, fadeAlpha01)) * 240.0f);
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, panelAlpha);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderDrawRect(renderer, &panel);

    // Title
    renderTextCentered("You died", outW/2, panel.y + 40, SDL_Color{255, 80, 80, static_cast<Uint8>(255 * fadeAlpha01)});
    renderTextCentered("Click to respawn", outW/2, panel.y + 80, SDL_Color{230, 230, 230, static_cast<Uint8>(255 * fadeAlpha01)});

    // Button
    int btnW = 180, btnH = 44;
    SDL_Rect btn = { outW/2 - btnW/2, panel.y + panelH - 70, btnW, btnH };
    // Hover effect via mouse pos - InputManager not injected here, so render static button
    SDL_SetRenderDrawColor(renderer, 70, 130, 180, static_cast<Uint8>(255 * fadeAlpha01));
    SDL_RenderFillRect(renderer, &btn);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &btn);
    renderTextCentered("Respawn", outW/2, btn.y + btn.h/2, SDL_Color{255,255,255, static_cast<Uint8>(255 * fadeAlpha01)});

    // Click detection: poll current mouse state
    int mx, my;
    Uint32 mouse = SDL_GetMouseState(&mx, &my);
    bool leftDown = (mouse & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    if (fadeAlpha01 >= 1.0f && leftDown && mx >= btn.x && mx <= btn.x + btn.w && my >= btn.y && my <= btn.y + btn.h) {
        outClickedRespawn = true;
    }
}
