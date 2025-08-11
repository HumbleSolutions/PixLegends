#include "UISystem.h"
#include "Renderer.h"
#include "Player.h"
#include "AssetManager.h"
#include <iostream>
#include <sstream>
#include <algorithm>

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
void UISystem::renderInventory(const Player* player, int screenW, int screenH,
                               int mouseX, int mouseY, bool leftDown, bool rightDown,
                               InventoryHit& outHit) {
    outHit = InventoryHit{};
    if (!assetManager || !player) return;
    Texture* bag = assetManager->getTexture("assets/Textures/UI/Inventory.png");
    if (!bag) return;
    int bw = bag->getWidth();
    int bh = bag->getHeight();
    int spacing = 12;
    // Bottom-right positioning for the two bags
    int y = screenH - bh - 20;
    int x0 = screenW - (bw * 2 + 12 + 20);
    // Draw two bags side by side
    SDL_Rect d0{ x0, y, bw, bh };
    SDL_Rect d1{ x0 + bw + spacing, y, bw, bh };
    SDL_RenderCopy(renderer, bag->getTexture(), nullptr, &d0);
    SDL_RenderCopy(renderer, bag->getTexture(), nullptr, &d1);
    // Lay out a simple 3x3 grid per bag with icons and counts for known items
    const int cols = 3, rows = 3;
    int cell = bw / 3 - 10;
    static bool lastRightDown = false;
    auto drawStack = [&](int cx, int cy, const char* path, int count, const char* payloadKey){
        Texture* t = assetManager->getTexture(path);
        if (!t || count <= 0) return;
        SDL_Rect s{0,0,t->getWidth(),t->getHeight()};
        SDL_Rect d{ cx, cy, cell, cell };
        SDL_RenderCopy(renderer, t->getTexture(), &s, &d);
        renderText("x" + std::to_string(count), d.x + 4, d.y + cell - 14);
        if (leftDown && mouseX>=d.x && mouseX<=d.x+d.w && mouseY>=d.y && mouseY<=d.y+d.h) {
            outHit.startedDrag = true; outHit.dragPayload = payloadKey;
        }
        if (rightDown && !lastRightDown && mouseX>=d.x && mouseX<=d.x+d.w && mouseY>=d.y && mouseY<=d.y+d.h) {
            outHit.rightClicked = true; outHit.rightClickedPayload = payloadKey;
        }

        // Tooltip for stack
        if (mouseX>=d.x && mouseX<=d.x+d.w && mouseY>=d.y && mouseY<=d.y+d.h) {
            const char* name = payloadKey;
            if (std::strcmp(payloadKey, "upgrade_scroll") == 0) name = "Blessed Upgrade Scroll";
            else if (std::strcmp(payloadKey, "fire") == 0) name = "Fire Enchant Scroll";
            else if (std::strcmp(payloadKey, "water") == 0) name = "Water Enchant Scroll";
            else if (std::strcmp(payloadKey, "poison") == 0) name = "Poison Enchant Scroll";
            int w = 210, h = 56; SDL_Rect tip{ mouseX + 14, mouseY + 10, w, h };
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 20,20,24,220); SDL_RenderFillRect(renderer, &tip);
            SDL_SetRenderDrawColor(renderer, 200,200,210,255); SDL_RenderDrawRect(renderer, &tip);
            renderText(name, tip.x + 8, tip.y + 6, SDL_Color{255,255,180,255});
            renderText("Count: " + std::to_string(count), tip.x + 8, tip.y + 24);
            renderText("Right-click to use at Anvil", tip.x + 8, tip.y + 40, SDL_Color{150,200,255,255});
        }
    };
    // simple mapping: show upgrade + fire/water/poison stacks in fixed slots
    int sx0 = d0.x + 8, sy0 = d0.y + 8;
    // Upgrade scrolls appear as a stack in the first cell using the upgrade_scroll.png asset
    drawStack(sx0,             sy0,             "assets/Textures/Items/upgrade_scroll.png", player->getUpgradeScrolls(), "upgrade_scroll");
    drawStack(sx0 + cell + 8,  sy0,             "assets/Textures/Items/fire_dmg_scroll.png", player->getElementScrolls("fire"), "fire");
    drawStack(sx0 + 2*(cell+8),sy0,             "assets/Textures/Items/water_damage_scroll.png", player->getElementScrolls("water"), "water");
    drawStack(sx0,             sy0 + cell + 8,  "assets/Textures/Items/Poison_dmg_scroll.png", player->getElementScrolls("poison"), "poison");
    lastRightDown = rightDown;
}

void UISystem::renderTexturedBar(const char* texturePath,
                           int x, int y,
                           int width, int height,
                           float progress) {
    if (!assetManager || !texturePath) return;
    Texture* barTex = assetManager->getTexture(texturePath);
    if (!barTex) return;

    progress = std::max(0.0f, std::min(1.0f, progress));

    // Only render the filled section of the bar (no background track)
    SDL_Texture* sdlTex = barTex->getTexture();
    int tw = barTex->getWidth();
    int th = barTex->getHeight();
    if (tw <= 0 || th <= 0) return;
    // If width/height are not specified, render at native size (no scaling)
    if (width <= 0) width = tw;
    if (height <= 0) height = th;

    // Render the foreground filled section using the texture, clipped by progress.
    // Do NOT resize bars: use native texture size for both source and destination.
    // Exclude the decorative tip for partial fills by clipping to body width; draw full texture only at 100%.
    const int srcTip = static_cast<int>(std::round(tw * 0.12f)); // approximate tip width in source
    const int bodySrc = std::max(0, tw - srcTip);
    const int bodyDst = bodySrc; // no scaling

    int filledDstW = 0;
    int filledSrcW = 0;
    if (progress >= 0.999f) {
        filledDstW = tw;
        filledSrcW = tw;
    } else {
        filledDstW = static_cast<int>(std::round(bodyDst * progress));
        filledSrcW = static_cast<int>(std::round(bodySrc * progress));
    }
    if (filledDstW > 0 && filledSrcW > 0) {
        SDL_Rect srcFG{0, 0, filledSrcW, th};
        SDL_Rect dstFG{ x, y, filledDstW, th };
        SDL_RenderCopy(renderer, sdlTex, &srcFG, &dstFG);
    }
}

void UISystem::renderHealthBar(int x, int y, int width, int height, int current, int max) {
    if (max <= 0) return;
    
    float progress = static_cast<float>(current) / static_cast<float>(max);
    // Textured version using new HP bar asset
    renderTexturedBar("assets/Textures/UI/hp_bar.png", x, y, width, height, progress);
    
    // Compute actual drawn size for text centering
    int usedW = width;
    int usedH = height;
    if ((usedW <= 0 || usedH <= 0) && assetManager) {
        if (Texture* t = assetManager->getTexture("assets/Textures/UI/hp_bar.png")) {
            usedW = t->getWidth();
            usedH = t->getHeight();
        }
    }
    // Render text
    std::string text = std::to_string(current) + "/" + std::to_string(max);
    renderTextCentered(text, x + usedW/2, y + usedH/2, textColor);
}

void UISystem::renderManaBar(int x, int y, int width, int height, int current, int max) {
    if (max <= 0) return;
    
    float progress = static_cast<float>(current) / static_cast<float>(max);
    renderTexturedBar("assets/Textures/UI/mana_bar.png", x, y, width, height, progress);
    
    int usedW = width;
    int usedH = height;
    if ((usedW <= 0 || usedH <= 0) && assetManager) {
        if (Texture* t = assetManager->getTexture("assets/Textures/UI/mana_bar.png")) {
            usedW = t->getWidth();
            usedH = t->getHeight();
        }
    }
    // Render text
    std::string text = std::to_string(current) + "/" + std::to_string(max);
    renderTextCentered(text, x + usedW/2, y + usedH/2, textColor);
}

void UISystem::renderExperienceBar(int x, int y, int width, int height, int current, int max) {
    if (max <= 0) return;
    
    float progress = static_cast<float>(current) / static_cast<float>(max);
    renderTexturedBar("assets/Textures/UI/exp_bar.png", x, y, width, height, progress);
    
    int usedW = width;
    int usedH = height;
    if ((usedW <= 0 || usedH <= 0) && assetManager) {
        if (Texture* t = assetManager->getTexture("assets/Textures/UI/exp_bar.png")) {
            usedW = t->getWidth();
            usedH = t->getHeight();
        }
    }
    // Render text
    std::string text = "XP: " + std::to_string(current) + "/" + std::to_string(max);
    renderTextCentered(text, x + usedW/2, y + usedH/2, textColor);
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
    
    // Try to render the new composite HUD frame first
    bool renderedFrame = false;
    int frameX = 10;
    int frameY = 10;
    int barHeight = 14; // fits visual thickness of bars in the UI image
    int barWidth = 260;
    int spacingY = 18;
    float scale = 1.0f;
    if (assetManager) {
        if (Texture* uiFrame = assetManager->getTexture("assets/Textures/UI/hp_mana_bar_ui.png")) {
            int texW = uiFrame->getWidth();
            int texH = uiFrame->getHeight();
            // Fractions tuned to align native-sized bar textures inside the frame
            // These values match the track positions in hp_mana_bar_ui.png so scaling keeps alignment.
            // Note: These align with the provided Aseprite mockup so tips land within the track caps
            const float BX_FRAC = 0.305f;      // bar start X as fraction of width
            const float BW_FRAC = 0.665f;      // bar width fraction (trim slightly to sit inside tip)
            const float BY_STEP_FRAC = 0.158f; // vertical spacing between bars
            const float BH_FRAC = 0.083f;      // bar height fraction (unused when height=0)
            // Start a little lower so bars sit within tracks
            const float BY1_FRAC = 0.385f;

            // Draw the frame at native size
            SDL_Rect dst{ frameX, frameY, texW, texH };
            SDL_RenderCopy(renderer, uiFrame->getTexture(), nullptr, &dst);

            int bx = frameX + static_cast<int>(texW * BX_FRAC);
            int by1 = frameY + static_cast<int>(texH * BY1_FRAC);
            spacingY = static_cast<int>(texH * BY_STEP_FRAC);
            int by2 = by1 + spacingY;
            int by3 = by2 + spacingY;
            barWidth = static_cast<int>(texW * BW_FRAC);
            barHeight = static_cast<int>(texH * BH_FRAC);

            // Render bars aligned to the frame top-to-bottom:
            // 1) HP (red), 2) Mana (blue), 3) EXP (green)
            // Add a tiny vertical inset so the thin native-height bars center in the track
            int innerOffsetY = static_cast<int>(texH * 0.035f); // nudge bars slightly lower inside tracks
            // Additional precise pixel nudges: HP +5, Mana +6, EXP +7
            // Use native bar widths/heights (no scaling): pass width=0, height=0
            renderHealthBar(bx, by1 + innerOffsetY + 7, 0, 0, player->getHealth(), player->getMaxHealth());
            renderManaBar(bx, by2 + innerOffsetY + 8, 0, 0, player->getMana(), player->getMaxMana());
            renderExperienceBar(bx, by3 + innerOffsetY + 9, 0, 0, player->getExperience(), player->getExperienceToNext());

            // Move gold icon to the left gap under the circular portrait
            int goldX = frameX + static_cast<int>(texW * 0.330f);
            int goldY = by3 + barHeight + 6 + 10;
            renderGoldDisplay(goldX, goldY, player->getGold());

            // Level/Stats to the right of the bars
            int textX = bx + barWidth + 10;
            int textY = by1 - 4;
            renderText("Level: " + std::to_string(player->getLevel()), textX, textY);
            renderText("STR: " + std::to_string(player->getStrength()), textX, textY + 20);
            renderText("INT: " + std::to_string(player->getIntelligence()), textX, textY + 40);
            renderedFrame = true;
        }
    }

    // Fallback: draw simple bars if frame is unavailable
    if (!renderedFrame) {
        int startX = 10;
        int startY = 10;
        barWidth = 260;
        barHeight = 20;
        int spacing = 28;
        renderHealthBar(startX, startY, barWidth, barHeight, player->getHealth(), player->getMaxHealth());
        renderManaBar(startX, startY + spacing, barWidth, barHeight, player->getMana(), player->getMaxMana());
        renderExperienceBar(startX, startY + spacing * 2, barWidth, barHeight, player->getExperience(), player->getExperienceToNext());
        renderGoldDisplay(startX, startY + spacing * 3, player->getGold());
        int textX = startX + barWidth + 10;
        int textY = startY;
        renderText("Level: " + std::to_string(player->getLevel()), textX, textY);
        renderText("STR: " + std::to_string(player->getStrength()), textX, textY + 20);
        renderText("INT: " + std::to_string(player->getIntelligence()), textX, textY + 40);
    }

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

// --- Magic Anvil UI ---
void UISystem::renderMagicAnvil(const Player* player,
                                int screenW, int screenH,
                                int mouseX, int mouseY, bool mouseDown,
                                AnvilHit& outHit,
                                const std::string& externalDragPayload,
                                int selectedSlotIdx,
                                const std::string& selectedScrollKey) {
    outHit = AnvilHit{};
    if (!assetManager) return;
    Texture* panel = assetManager->getTexture("assets/Textures/UI/Item_inv.png");
    if (!panel) return;

    // Position the panel to the right so it doesn't cover the big anvil sprite
    int pw = panel->getWidth();
    int ph = panel->getHeight();
    int margin = 20;
    int desiredX = screenW / 2 + 260; // push further right of center
    if (desiredX + pw > screenW - margin) desiredX = std::max(margin, screenW - pw - margin);
    SDL_Rect dst{ desiredX, (screenH - ph) / 2, pw, ph };
    SDL_RenderCopy(renderer, panel->getTexture(), nullptr, &dst);

    // Define 3x3 grid within panel with approximate padding inferred from art
    const int gridX = dst.x + 20;
    const int gridY = dst.y + 20;
    const int cell = (std::min(pw, ph) - 40) / 3; // square cells

    // Slot art per equipment
    const char* slotArt[9] = {
        "assets/Textures/UI/ring_upgrade_slot.png",
        "assets/Textures/UI/helm_upgrade_slot.png",
        "assets/Textures/UI/necklace_upgrade_slot.png",
        "assets/Textures/UI/sword_upgrade_slot.png",
        "assets/Textures/UI/chest_upgrade_slot.png",
        "assets/Textures/UI/sheild_upgrade_slot.png",
        "assets/Textures/UI/gloves_upgrade_slot.png",
        "assets/Textures/UI/belt_upgrade_slot.png",
        "assets/Textures/UI/boots_upgrade_slot.png"
    };
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            int idx = r*3 + c; Texture* t = assetManager->getTexture(slotArt[idx]);
            SDL_Rect d{ gridX + c*cell, gridY + r*cell, cell, cell };
            if (t) {
                SDL_Rect s{0,0,t->getWidth(),t->getHeight()};
                SDL_RenderCopy(renderer, t->getTexture(), &s, &d);
            } else {
                SDL_SetRenderDrawColor(renderer, 60,60,80,200); SDL_RenderFillRect(renderer, &d);
            }
            const char* iconMap[9] = {
                "assets/Textures/Items/ring_01.png",
                "assets/Textures/Items/helmet_01.png",
                "assets/Textures/Items/necklace_01.png",
                "assets/Textures/Items/sword_01.png",
                "assets/Textures/Items/chestpeice_01.png",
                "assets/Textures/Items/sheild_01.png",
                "assets/Textures/Items/gloves_01.png",
                "assets/Textures/Items/waist_01.png",
                "assets/Textures/Items/boots_01.png"
            };
            if (Texture* it = assetManager->getTexture(iconMap[idx])) {
                int pad = std::max(6, cell / 6);
                SDL_Rect si{0,0,it->getWidth(),it->getHeight()};
                SDL_Rect di{ d.x + pad, d.y + pad, d.w - pad*2, d.h - pad*2 };
                SDL_RenderCopy(renderer, it->getTexture(), &si, &di);
                if (mouseDown && mouseX>=di.x && mouseX<=di.x+di.w && mouseY>=di.y && mouseY<=di.y+di.h) {
                    outHit.clickedSlot = idx;
                }
            }
            if (player) {
                const Player::EquipmentItem& eq = player->getEquipment(static_cast<Player::EquipmentSlot>(idx));
                renderText(std::string("+") + std::to_string(eq.plusLevel), d.x + d.w - 20, d.y + d.h - 18, SDL_Color{200,255,200,255});
            }
        }
    }

    // Hit test slots 0..8
    int hovered = -1;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            int idx = r * 3 + c;
            SDL_Rect slot{ gridX + c * cell, gridY + r * cell, cell, cell };
            // Draw subtle hover outline
            if (mouseX >= slot.x && mouseX <= slot.x + slot.w && mouseY >= slot.y && mouseY <= slot.y + slot.h) {
                hovered = idx;
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 255, 215, 100, 130);
                SDL_RenderDrawRect(renderer, &slot);
                if (mouseDown) { outHit.clickedSlot = idx; }
            }
        }
    }

    // Buttons at bottom: Upgrade icon + element scroll icons
    // Close button removed; use Esc to close

    // Side panel (to the right): item slot, scroll slot, and Upgrade button
    int sideX = dst.x + pw + 16;
    int sideY = dst.y + 8;
    int slotSize = 48;
    // Item slot (use art and selected slot icon if any)
    SDL_Rect sideItem{ sideX, sideY, slotSize, slotSize };
    if (Texture* t = assetManager->getTexture("assets/Textures/UI/item_upgrade_slot.png")) {
        SDL_Rect s{0,0,t->getWidth(),t->getHeight()}; SDL_RenderCopy(renderer, t->getTexture(), &s, &sideItem);
    } else { SDL_SetRenderDrawColor(renderer, 90,90,120,200); SDL_RenderFillRect(renderer, &sideItem); SDL_SetRenderDrawColor(renderer, 255,255,255,255); SDL_RenderDrawRect(renderer, &sideItem); }
    // Scroll slot art
    SDL_Rect sideScroll{ sideX + slotSize + 16, sideY, slotSize, slotSize };
    if (Texture* t = assetManager->getTexture("assets/Textures/UI/scroll_slot.png")) {
        SDL_Rect s{0,0,t->getWidth(),t->getHeight()}; SDL_RenderCopy(renderer, t->getTexture(), &s, &sideScroll);
    } else { SDL_SetRenderDrawColor(renderer, 90,90,120,200); SDL_RenderFillRect(renderer, &sideScroll); SDL_SetRenderDrawColor(renderer, 255,255,255,255); SDL_RenderDrawRect(renderer, &sideScroll); }
    // Upgrade button art with text
    SDL_Rect sideBtn{ sideX, sideY + slotSize + 28, slotSize*2 + 16, 28 };
    if (Texture* t = assetManager->getTexture("assets/Textures/UI/upgrade_button.png")) {
        SDL_Rect s{0,0,t->getWidth(),t->getHeight()}; SDL_RenderCopy(renderer, t->getTexture(), &s, &sideBtn);
    } else { SDL_SetRenderDrawColor(renderer, 80,60,40,230); SDL_RenderFillRect(renderer, &sideBtn); SDL_SetRenderDrawColor(renderer, 255,255,255,255); SDL_RenderDrawRect(renderer, &sideBtn); }
    renderTextCentered("UPGRADE", sideBtn.x + sideBtn.w/2, sideBtn.y + sideBtn.h/2, SDL_Color{255,255,255,255});
    if (mouseDown && mouseX>=sideBtn.x && mouseX<=sideBtn.x+sideBtn.w && mouseY>=sideBtn.y && mouseY<=sideBtn.y+sideBtn.h) {
        outHit.clickedSideUpgrade = true;
    }

    // Remove scroll icons from anvil UI (scrolls live in bags)
    // If dragging ends over a slot, convert to an action; allow external payload (from inventory)
    static bool wasDown = false;
    if (!mouseDown && wasDown) {
        outHit.endedDrag = true;
        // Check drop over a slot
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                int idx = r * 3 + c;
                SDL_Rect slot{ gridX + c * cell, gridY + r * cell, cell, cell };
                if (mouseX>=slot.x && mouseX<=slot.x+slot.w && mouseY>=slot.y && mouseY<=slot.y+slot.h) {
                    outHit.clickedSlot = idx;
                    std::string payload = externalDragPayload.empty()? outHit.dragPayload : externalDragPayload;
                    if (payload == "upgrade_scroll") outHit.clickedUpgrade = true;
                    else if (!payload.empty()) outHit.clickedElement = payload;
                }
            }
        }
        // Also allow dropping into side specific slots
        std::string payload = externalDragPayload.empty()? outHit.dragPayload : externalDragPayload;
        if (mouseX>=sideItem.x && mouseX<=sideItem.x+sideItem.w && mouseY>=sideItem.y && mouseY<=sideItem.y+sideItem.h) {
            outHit.droppedItem = true;
        }
        if (mouseX>=sideScroll.x && mouseX<=sideScroll.x+sideScroll.w && mouseY>=sideScroll.y && mouseY<=sideScroll.y+sideScroll.h) {
            if (payload == "upgrade_scroll" || payload=="fire" || payload=="water" || payload=="poison" || payload=="lightning") {
                outHit.droppedScroll = true; outHit.droppedScrollKey = payload;
                if (payload == "upgrade_scroll") outHit.clickedUpgrade = true; else outHit.clickedElement = payload;
            }
        }
        outHit.dragPayload.clear();
        outHit.dragRect = SDL_Rect{0,0,0,0};
    }
    wasDown = mouseDown;

    // No explicit close button

    // Tooltip / result indicator area (floating panel near mouse with stats and chance)
    if (hovered >= 0 && player) {
        const Player::EquipmentItem& heq = player->getEquipment(static_cast<Player::EquipmentSlot>(hovered));
        // Use the custom upgrade table to preview chance
        auto chanceForNext = [&](int currentPlus) -> float {
            static const float table[31] = {
                0.0f, 100.0f, 95.0f, 90.0f, 80.0f, 75.0f, 70.0f, 65.0f, 55.0f, 50.0f,
                45.0f, 40.0f, 35.0f, 30.0f, 28.0f, 25.0f, 20.0f, 18.0f, 15.0f, 13.0f,
                12.0f, 10.0f, 8.0f, 7.0f, 6.0f, 5.0f, 4.5f, 4.0f, 3.5f, 3.0f, 2.5f
            };
            int next = currentPlus + 1; if (next < 1) next = 1; if (next > 30) next = 30; return table[next];
        };
        int chance = static_cast<int>(std::round(chanceForNext(heq.plusLevel)));
        bool hasPreview = !selectedScrollKey.empty();
        // Dynamically size tooltip height based on number of lines we will render
        int lineH = 16;
        int lines = 0;
        lines += 1; // name
        lines += 1; // +level/base
        lines += 1; // chance
        if (hovered == static_cast<int>(Player::EquipmentSlot::SWORD)) {
            lines += 4; // ATK, ASPD, Crit, DUR
        }
        // Element lines: place Fire last, so count all present
        int elemLines = 0;
        if (heq.ice > 0) elemLines++;
        if (heq.poison > 0) elemLines++;
        if (heq.fire > 0) elemLines++;
        lines += elemLines;
        if (hasPreview) lines += 1; // preview row
        int w = 260;
        int h = 12 + lines * lineH + 8; // top+bottom padding
        SDL_Rect tip{ mouseX + 16, mouseY + 12, w, h };
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 25,25,28,220); SDL_RenderFillRect(renderer, &tip);
        SDL_SetRenderDrawColor(renderer, 220,220,230,255); SDL_RenderDrawRect(renderer, &tip);
        // Show actual item name when available, else generic slot name
        static const char* slotNames[9] = {"Ring","Helm","Necklace","Sword","Chest","Shield","Glove","Waist","Feet"};
        std::string displayName = heq.name.empty() ? std::string(slotNames[hovered]) : heq.name;
        int ycur = tip.y + 6;
        renderText(displayName, tip.x + 8, ycur, SDL_Color{255,255,180,255});
        ycur += lineH;
        renderText(std::string("+") + std::to_string(heq.plusLevel) + "  Base:" + std::to_string(heq.basePower), tip.x + 8, ycur);
        ycur += lineH;
        // Show sword-specific stats if available
        if (hovered == static_cast<int>(Player::EquipmentSlot::SWORD)) {
            renderText("ATK: " + std::to_string(heq.attack), tip.x + 8, ycur);
            ycur += lineH;
            std::ostringstream aspd; aspd.setf(std::ios::fixed); aspd.precision(2);
            aspd << "ASPD " << heq.attackSpeedMultiplier << "x";
            renderText(aspd.str(), tip.x + 8, ycur);
            ycur += lineH;
            std::ostringstream crit; crit.setf(std::ios::fixed); crit.precision(1);
            crit << "Crit " << heq.critChancePercent << "%";
            renderText(crit.str(), tip.x + 8, ycur);
            ycur += lineH;
            renderText("DUR " + std::to_string(heq.durability) + "/" + std::to_string(heq.maxDurability), tip.x + 8, ycur);
            ycur += lineH;
        }
        renderText("Chance:" + std::to_string(chance) + "%", tip.x + 8, ycur, SDL_Color{140,200,255,255});
        ycur += lineH;
        // Optional preview next so Fire can stay at the very bottom
        if (hasPreview) {
            std::string pv;
            SDL_Color green{120, 255, 120, 255};
            if (selectedScrollKey == std::string("upgrade_scroll")) pv = "+1 level";
            else if (selectedScrollKey == std::string("fire")) pv = "+1 fire";
            else if (selectedScrollKey == std::string("water")) pv = "+1 water";
            else if (selectedScrollKey == std::string("poison")) pv = "+1 poison";
            if (!pv.empty()) { renderText("Preview: " + pv, tip.x + 8, ycur, green); ycur += lineH; }
        }
        // Element lines: Water, Poison, then Fire last
        if (heq.ice>0) { renderText("Water: +" + std::to_string(heq.ice), tip.x + 8, ycur, SDL_Color{140,180,255,255}); ycur += lineH; }
        if (heq.poison>0) { renderText("Poison: +" + std::to_string(heq.poison), tip.x + 8, ycur, SDL_Color{160,255,160,255}); ycur += lineH; }
        if (heq.fire>0) { renderText("Fire: +" + std::to_string(heq.fire), tip.x + 8, ycur, SDL_Color{255,140,80,255}); ycur += lineH; }
    }

    // Draw the selected slot icon in the side item slot for feedback
    int selectedIdx = (selectedSlotIdx >= 0 ? selectedSlotIdx : hovered);
    if (selectedIdx >= 0) {
        const char* iconMap[9] = {
            "assets/Textures/Items/ring_01.png",
            "assets/Textures/Items/helmet_01.png",
            "assets/Textures/Items/necklace_01.png",
            "assets/Textures/Items/sword_01.png",
            "assets/Textures/Items/chestpeice_01.png",
            "assets/Textures/Items/sheild_01.png",
            "assets/Textures/Items/gloves_01.png",
            "assets/Textures/Items/waist_01.png",
            "assets/Textures/Items/boots_01.png"
        };
        if (Texture* t = assetManager->getTexture(iconMap[selectedIdx])) {
            SDL_Rect s{0,0,t->getWidth(),t->getHeight()}; SDL_Rect d{ sideX+4, sideY+4, slotSize-8, slotSize-8 };
            SDL_RenderCopy(renderer, t->getTexture(), &s, &d);
        }
    }

    // Draw staged scroll icon in the scroll slot if provided via selectedScrollKey
    if (!selectedScrollKey.empty()) {
        const char* icon = nullptr;
        if (selectedScrollKey == std::string("upgrade_scroll")) icon = "assets/Textures/Items/upgrade_scroll.png";
        else if (selectedScrollKey == std::string("fire")) icon = "assets/Textures/Items/fire_dmg_scroll.png";
        else if (selectedScrollKey == std::string("water")) icon = "assets/Textures/Items/water_damage_scroll.png";
        else if (selectedScrollKey == std::string("poison")) icon = "assets/Textures/Items/Poison_dmg_scroll.png";
        if (icon) {
            if (Texture* t = assetManager->getTexture(icon)) {
                SDL_Rect s{0,0,t->getWidth(),t->getHeight()};
                SDL_Rect d{ sideScroll.x + 6, sideScroll.y + 6, sideScroll.w - 12, sideScroll.h - 12 };
                SDL_RenderCopy(renderer, t->getTexture(), &s, &d);
            }
        }
    }

    // Upgrade indicator anchored between item/scroll slots
    SDL_Rect indicatorRect{ sideX + 8, sideY + slotSize + 4, sideBtn.w - 16, 22 };
    if (Texture* ind = assetManager->getTexture("assets/Textures/UI/upgrade_indicator.png")) {
        SDL_Rect s{0,0,ind->getWidth(), ind->getHeight()};
        SDL_RenderCopy(renderer, ind->getTexture(), &s, &indicatorRect);
    }
    // Optionally overlay success/fail bars here when Game requests (Game renders them by computing the same rect)
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
