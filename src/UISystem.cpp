#include "UISystem.h"
#include "Renderer.h"
#include "Player.h"
#include "AssetManager.h"
#include "ItemSystem.h"
#include "Game.h"
#include "SpellSystem.h"
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

void UISystem::renderDashCooldown(const Player* player) {
    if (!player) return;
    int outW = 0, outH = 0; if (renderer) SDL_GetRendererOutputSize(renderer, &outW, &outH);
    if (outW <= 0) { outW = 1280; outH = 720; }
    float remain = player->getDashCooldownRemaining();
    float maxCd = player->getDashCooldownMax();
    float t = std::max(0.0f, std::min(1.0f, (maxCd > 0.0f ? remain / maxCd : 0.0f)));
    // Bottom center bar
    int barW = 160;
    int barH = 14;
    int x = outW/2 - barW/2;
    int y = outH - 48; // bottom middle
    // Background
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 20,20,24,180);
    SDL_Rect bg{ x, y, barW, barH };
    SDL_RenderFillRect(renderer, &bg);
    SDL_SetRenderDrawColor(renderer, 220,220,230,255);
    SDL_RenderDrawRect(renderer, &bg);
    // Fill amount (cooldown remaining)
    SDL_Color fillCol = player->isDashing() ? SDL_Color{255,255,255,220} : SDL_Color{80,160,255,220};
    SDL_SetRenderDrawColor(renderer, fillCol.r, fillCol.g, fillCol.b, fillCol.a);
    int fillW = static_cast<int>(std::round(barW * t));
    SDL_Rect fg{ x, y, fillW, barH };
    SDL_RenderFillRect(renderer, &fg);
    // Text label and seconds
    std::string label = "Dash";
    if (remain > 0.05f) {
        int secs = static_cast<int>(std::ceil(remain));
        label += " (" + std::to_string(secs) + ")";
    }
    renderTextCentered(label, x + barW/2, y + barH/2, SDL_Color{255,255,255,255});
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
            // Replace shield slot icon with bow_item.png under portrait area
            // Commented out to avoid texture not found spam
            // if (assetManager) {
            //     if (Texture* bowIcon = assetManager->getTexture("assets/Textures/UI/bow_item.png")) {
            //         int iw = 28, ih = 28; // scaled icon size
            //         int iconX = frameX + static_cast<int>(texW * 0.16f);
            //         int iconY = by3 + barHeight + 6 + 10; // near gold
            //         SDL_Rect s{0,0,bowIcon->getWidth(), bowIcon->getHeight()};
            //         SDL_Rect d{ iconX, iconY, iw, ih };
            //         SDL_RenderCopy(renderer, bowIcon->getTexture(), &s, &d);
            //         renderText("Bow", iconX + iw + 6, iconY + ih/2 - 8);
            //     }
            // }
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
                                const std::string& selectedScrollKey,
                                Game* game) {
    outHit = AnvilHit{};
    if (!assetManager) return;
    
    // Use the new AnvilUI.png background
    Texture* anvilBG = assetManager->getTexture("assets/Textures/UI/AnvilUI.png");
    if (!anvilBG) return;

    // Use custom position if set, otherwise use default positioning
    int pw = anvilBG->getWidth();
    int ph = anvilBG->getHeight();
    int anvilX, anvilY;
    
    // Check for custom position from game
    if (game && game->getAnvilPosX() >= 0 && game->getAnvilPosY() >= 0) {
        anvilX = game->getAnvilPosX();
        anvilY = game->getAnvilPosY();
    } else {
        // Default positioning - left quarter of screen to allow room for inventory
        anvilX = screenW / 4 - pw / 2;
        anvilY = (screenH - ph) / 2;
    }
    SDL_Rect anvilRect{ anvilX, anvilY, pw, ph };
    SDL_RenderCopy(renderer, anvilBG->getTexture(), nullptr, &anvilRect);

    // New layout based on concept: two slots at top, upgrade button below, progress bar at bottom
    
    // Define slot positions based on the concept image
    int slotSize = 48;
    int slotSpacing = 16;
    
    // Item upgrade slot (left slot at top)
    SDL_Rect itemSlot = {
        anvilX + (pw / 2) - slotSize - (slotSpacing / 2),
        anvilY + 40,
        slotSize, slotSize
    };
    
    // Scroll slot (right slot at top)  
    SDL_Rect scrollSlot = {
        anvilX + (pw / 2) + (slotSpacing / 2),
        anvilY + 40,
        slotSize, slotSize
    };
    
    // Draw the item upgrade slot
    if (Texture* t = assetManager->getTexture("assets/Textures/UI/item_upgrade_slot.png")) {
        SDL_Rect s{0,0,t->getWidth(),t->getHeight()};
        SDL_RenderCopy(renderer, t->getTexture(), &s, &itemSlot);
    } else {
        SDL_SetRenderDrawColor(renderer, 90, 90, 120, 200);
        SDL_RenderFillRect(renderer, &itemSlot);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &itemSlot);
    }
    
    // Draw the scroll slot
    if (Texture* t = assetManager->getTexture("assets/Textures/UI/scroll_slot.png")) {
        SDL_Rect s{0,0,t->getWidth(),t->getHeight()};
        SDL_RenderCopy(renderer, t->getTexture(), &s, &scrollSlot);
    } else {
        SDL_SetRenderDrawColor(renderer, 90, 90, 120, 200);
        SDL_RenderFillRect(renderer, &scrollSlot);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &scrollSlot);
    }
    
    // Show item in upgrade slot - prioritize target item over selected slot
    Item* targetItem = (game ? game->getAnvilTargetItem() : nullptr);
    
    if (targetItem && targetItem->type == ItemType::EQUIPMENT) {
        // Use the specific dragged item
        if (Texture* itemIcon = assetManager->getTexture(targetItem->iconPath)) {
            int pad = 4;
            SDL_Rect iconRect = {itemSlot.x + pad, itemSlot.y + pad, itemSlot.w - pad*2, itemSlot.h - pad*2};
            SDL_RenderCopy(renderer, itemIcon->getTexture(), nullptr, &iconRect);
            
            // Show +level of target item
            renderText("+" + std::to_string(targetItem->plusLevel), itemSlot.x + itemSlot.w - 20, itemSlot.y + itemSlot.h - 18, {200, 255, 200, 255});
        }
    } else if (selectedSlotIdx >= 0 && selectedSlotIdx <= 8 && player) {
        // Fallback to equipment slot method when no target item
        const Player::EquipmentItem& eq = player->getEquipment(static_cast<Player::EquipmentSlot>(selectedSlotIdx));
        
        // Item icon paths for display
        const char* iconMap[9] = {
            "assets/Textures/Items/ring_01.png",
            "assets/Textures/Items/helmet_01.png", 
            "assets/Textures/Items/necklace_01.png",
            "assets/Textures/Items/sword_01.png",
            "assets/Textures/Items/chestpeice_01.png",
            "assets/Textures/Items/bow_01.png",
            "assets/Textures/Items/gloves_01.png",
            "assets/Textures/Items/waist_01.png",
            "assets/Textures/Items/boots_01.png"
        };
        
        if (Texture* itemIcon = assetManager->getTexture(iconMap[selectedSlotIdx])) {
            int pad = 4;
            SDL_Rect iconRect = {itemSlot.x + pad, itemSlot.y + pad, itemSlot.w - pad*2, itemSlot.h - pad*2};
            SDL_RenderCopy(renderer, itemIcon->getTexture(), nullptr, &iconRect);
            
            // Show +level
            renderText("+" + std::to_string(eq.plusLevel), itemSlot.x + itemSlot.w - 20, itemSlot.y + itemSlot.h - 18, {200, 255, 200, 255});
        }
    }
    
    // Show scroll in appropriate slot based on type
    if (!selectedScrollKey.empty()) {
        std::string scrollIconPath;
        SDL_Rect targetSlot = scrollSlot;  // Default to scroll slot
        
        if (selectedScrollKey == "upgrade_scroll") {
            // Upgrade scrolls go in the right slot (empty slot in screenshot)
            scrollIconPath = "assets/Textures/Items/upgrade_scroll.png";
            targetSlot = scrollSlot;
        } else if (selectedScrollKey == "fire_scroll" || selectedScrollKey == "water_scroll" || 
                   selectedScrollKey == "lightning_scroll" || selectedScrollKey == "poison_scroll") {
            // Element scrolls also go in the right slot
            if (selectedScrollKey == "fire_scroll") {
                scrollIconPath = "assets/Textures/Items/fire_dmg_scroll.png";
            } else if (selectedScrollKey == "water_scroll") {
                scrollIconPath = "assets/Textures/Items/water_damage_scroll.png";
            } else if (selectedScrollKey == "lightning_scroll") {
                scrollIconPath = "assets/Textures/Items/lightning_scroll.png"; // Need to check this exists
            } else if (selectedScrollKey == "poison_scroll") {
                scrollIconPath = "assets/Textures/Items/Poison_dmg_scroll.png";
            }
            targetSlot = scrollSlot;
        }
        
        if (!scrollIconPath.empty()) {
            if (Texture* scrollIcon = assetManager->getTexture(scrollIconPath)) {
                int pad = 4;
                SDL_Rect iconRect = {targetSlot.x + pad, targetSlot.y + pad, targetSlot.w - pad*2, targetSlot.h - pad*2};
                SDL_RenderCopy(renderer, scrollIcon->getTexture(), nullptr, &iconRect);
            }
        }
    }
    // Upgrade button (horizontal bar below slots)
    SDL_Rect upgradeButton = {
        anvilX + (pw / 2) - 60,
        anvilY + 120,
        120, 24
    };
    
    if (Texture* t = assetManager->getTexture("assets/Textures/UI/upgrade_button.png")) {
        SDL_RenderCopy(renderer, t->getTexture(), nullptr, &upgradeButton);
    } else {
        SDL_SetRenderDrawColor(renderer, 80, 60, 40, 230);
        SDL_RenderFillRect(renderer, &upgradeButton);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &upgradeButton);
    }
    renderTextCentered("UPGRADE", upgradeButton.x + upgradeButton.w/2, upgradeButton.y + upgradeButton.h/2, {255, 255, 255, 255});
    
    // Upgrade indicator with background texture
    SDL_Rect indicatorRect = {
        anvilX + (pw / 2) - 55,  // Center horizontally
        anvilY + ph - 50,        // Near bottom, moved up from previous position
        110, 20
    };
    
    // Draw the upgrade indicator background
    if (Texture* indicator = assetManager->getTexture("assets/Textures/UI/upgrade_indicator.png")) {
        SDL_RenderCopy(renderer, indicator->getTexture(), nullptr, &indicatorRect);
    } else {
        // Fallback if texture not found
        SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
        SDL_RenderFillRect(renderer, &indicatorRect);
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        SDL_RenderDrawRect(renderer, &indicatorRect);
    }

    // Handle mouse interactions
    static bool wasDown = false;
    
    // Check title bar for dragging (top 40 pixels of panel, avoiding slots and button areas)
    SDL_Rect titleBar = {anvilX, anvilY, pw - 40, 40};  // Leave some space at the right edge
    if (mouseDown && mouseX >= titleBar.x && mouseX <= titleBar.x + titleBar.w && 
        mouseY >= titleBar.y && mouseY <= titleBar.y + titleBar.h) {
        outHit.titleBarClicked = true;
        outHit.dragOffsetX = mouseX - anvilX;
        outHit.dragOffsetY = mouseY - anvilY;
    }
    
    // Check for clicks on upgrade button
    if (mouseDown && mouseX >= upgradeButton.x && mouseX <= upgradeButton.x + upgradeButton.w && 
        mouseY >= upgradeButton.y && mouseY <= upgradeButton.y + upgradeButton.h) {
        outHit.clickedSideUpgrade = true;
    }
    
    // Handle drag and drop for item and scroll slots
    if (!mouseDown && wasDown) {
        outHit.endedDrag = true;
        std::string payload = externalDragPayload.empty() ? outHit.dragPayload : externalDragPayload;
        
        // Check drop over item slot
        if (mouseX >= itemSlot.x && mouseX <= itemSlot.x + itemSlot.w && 
            mouseY >= itemSlot.y && mouseY <= itemSlot.y + itemSlot.h) {
            outHit.droppedItem = true;
        }
        
        // Check drop over scroll slot
        if (mouseX >= scrollSlot.x && mouseX <= scrollSlot.x + scrollSlot.w && 
            mouseY >= scrollSlot.y && mouseY <= scrollSlot.y + scrollSlot.h) {
            if (payload == "upgrade_scroll" || payload == "fire" || payload == "water" || 
                payload == "poison" || payload == "lightning") {
                outHit.droppedScroll = true;
                outHit.droppedScrollKey = payload;
                if (payload == "upgrade_scroll") outHit.clickedUpgrade = true;
                else outHit.clickedElement = payload;
            }
        }
        
        outHit.dragPayload.clear();
        outHit.dragRect = SDL_Rect{0, 0, 0, 0};
    }
    wasDown = mouseDown;

    // Simple tooltip for item in upgrade slot
    if (mouseX >= itemSlot.x && mouseX <= itemSlot.x + itemSlot.w && 
        mouseY >= itemSlot.y && mouseY <= itemSlot.y + itemSlot.h) {
        
        // Use target item if available, otherwise use selected slot
        if (targetItem) {
            // Calculate upgrade chance for target item
            auto chanceForNext = [&](int currentPlus) -> float {
                static const float table[31] = {
                    0.0f, 100.0f, 95.0f, 90.0f, 80.0f, 75.0f, 70.0f, 65.0f, 55.0f, 50.0f,
                    45.0f, 40.0f, 35.0f, 30.0f, 28.0f, 25.0f, 20.0f, 18.0f, 15.0f, 13.0f,
                    12.0f, 10.0f, 8.0f, 7.0f, 6.0f, 5.0f, 4.5f, 4.0f, 3.5f, 3.0f, 2.5f
                };
                int next = currentPlus + 1;
                if (next < 1) next = 1;
                if (next > 30) next = 30;
                return table[next];
            };
            int chance = static_cast<int>(std::round(chanceForNext(targetItem->plusLevel)));
            
            // Tooltip using target item data
            std::string tooltipText = targetItem->getDisplayName();
            tooltipText += "\nUpgrade chance: " + std::to_string(chance) + "%";
            
            renderTooltip(tooltipText, mouseX, mouseY);
        } else if (selectedSlotIdx >= 0 && player) {
            // Fallback to equipment slot tooltip
            const Player::EquipmentItem& heq = player->getEquipment(static_cast<Player::EquipmentSlot>(selectedSlotIdx));
            
            auto chanceForNext = [&](int currentPlus) -> float {
                static const float table[31] = {
                    0.0f, 100.0f, 95.0f, 90.0f, 80.0f, 75.0f, 70.0f, 65.0f, 55.0f, 50.0f,
                    45.0f, 40.0f, 35.0f, 30.0f, 28.0f, 25.0f, 20.0f, 18.0f, 15.0f, 13.0f,
                    12.0f, 10.0f, 8.0f, 7.0f, 6.0f, 5.0f, 4.5f, 4.0f, 3.5f, 3.0f, 2.5f
                };
                int next = currentPlus + 1;
                if (next < 1) next = 1;
                if (next > 30) next = 30;
                return table[next];
            };
            int chance = static_cast<int>(std::round(chanceForNext(heq.plusLevel)));
            
            // Simple tooltip
            std::string tooltipText = heq.name.empty() ? "Equipment" : heq.name;
            tooltipText += "\n+" + std::to_string(heq.plusLevel);
            tooltipText += "\nUpgrade chance: " + std::to_string(chance) + "%";
            
            renderTooltip(tooltipText, mouseX, mouseY);
        }
    }
}

void UISystem::renderOptionsMenu(int selectedIndex,
                           int masterVolume, int musicVolume, int soundVolume,
                           int monsterVolume, int playerMeleeVolume,
                           bool fullscreenEnabled, bool vsyncEnabled,
                           OptionsTab activeTab,
                           int mouseX, int mouseY, bool mouseDown,
                           MenuHitResult& outResult) {
    if (!defaultFont) return;
    static bool lastMouseDown = false; // edge-trigger for clicks
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
        if (hovered && mouseDown && !lastMouseDown) outResult.newTabIndex = t;
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
                if (mouseDown && !lastMouseDown && mouseX >= action.x && mouseX <= action.x + action.w && mouseY >= action.y && mouseY <= action.y + action.h) outResult.clickedResume = true;
            } else if (i == 1) {
                renderText("Reset", rowRect.x + 10, rowRect.y + 12, SDL_Color{255,255,255,255});
                SDL_SetRenderDrawColor(renderer, 200, 140, 60, 255);
                SDL_RenderFillRect(renderer, &action);
                SDL_SetRenderDrawColor(renderer, 255,255,255,255);
                SDL_RenderDrawRect(renderer, &action);
                renderText("Reset", action.x + 12, action.y + 8, SDL_Color{255,255,255,255});
                if (mouseDown && !lastMouseDown && mouseX >= action.x && mouseX <= action.x + action.w && mouseY >= action.y && mouseY <= action.y + action.h) outResult.clickedReset = true;
            } else {
                renderText("Logout", rowRect.x + 10, rowRect.y + 12, SDL_Color{255,255,255,255});
                SDL_SetRenderDrawColor(renderer, 170, 70, 70, 255);
                SDL_RenderFillRect(renderer, &action);
                SDL_SetRenderDrawColor(renderer, 255,255,255,255);
                SDL_RenderDrawRect(renderer, &action);
                renderText("Logout", action.x + 12, action.y + 8, SDL_Color{255,255,255,255});
                if (mouseDown && !lastMouseDown && mouseX >= action.x && mouseX <= action.x + action.w && mouseY >= action.y && mouseY <= action.y + action.h) outResult.clickedLogout = true;
            }
        }
    } else if (activeTab == OptionsTab::Sound) {
        // Sound: Master, Music, Sound, Monster, Player sliders (with mute checkboxes)
        const int sliders = 5;
        for (int i = 0; i < sliders; ++i) {
            SDL_Rect rowRect{ panel.x + 30, startY + i * rowH, panelW - 60, rowH - 6 };
            bool selected = (mouseX >= rowRect.x && mouseX <= rowRect.x + rowRect.w && mouseY >= rowRect.y && mouseY <= rowRect.y + rowRect.h);
            SDL_SetRenderDrawColor(renderer, selected ? 70 : 50, selected ? 120 : 50, selected ? 180 : 70, 200);
            SDL_RenderFillRect(renderer, &rowRect);
            SDL_SetRenderDrawColor(renderer, 220,220,220,255);
            SDL_RenderDrawRect(renderer, &rowRect);
            const char* labels[5] = { "Master Volume", "Music Volume", "Sound Volume", "Monster Volume", "Player Melee Volume" };
            renderText(labels[i], rowRect.x + 10, rowRect.y + 12);
            SDL_Rect valueArea{ rowRect.x + rowRect.w - 320, rowRect.y + 8, 300, rowRect.h - 16 };
            // Draw slider
            static int monsterVolCache = 100; // temporary cache; Game will read and apply
            static int playerVolCache = 100;
            int v = (i == 0 ? masterVolume : (i == 1 ? musicVolume : (i == 2 ? soundVolume : (i == 3 ? monsterVolume : playerMeleeVolume))));
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
                if (i == 3) { outResult.changedMonster = true; outResult.newMonster = newVal; monsterVolCache = newVal; }
                if (i == 4) { outResult.changedPlayer = true; outResult.newPlayer = newVal; playerVolCache = newVal; }
            }
            // Mute checkbox between label and slider
            SDL_Rect chk{ rowRect.x + 180, rowRect.y + 10, 18, 18 };
            SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255); SDL_RenderDrawRect(renderer, &chk);
            if (mouseDown && !lastMouseDown && mouseX >= chk.x && mouseX <= chk.x + chk.w && mouseY >= chk.y && mouseY <= chk.y + chk.h) {
                if (i == 0) { outResult.changedMaster = true; outResult.newMaster = 0; }
                if (i == 1) { outResult.changedMusic  = true; outResult.newMusic  = 0; }
                if (i == 2) { outResult.changedSound  = true; outResult.newSound  = 0; }
                if (i == 3) { outResult.changedMonster= true; outResult.newMonster= 0; monsterVolCache = 0; }
                if (i == 4) { outResult.changedPlayer = true; outResult.newPlayer = 0; playerVolCache = 0; }
            }
        }
        // Theme selection row as dropdown
        SDL_Rect rowRect2{ panel.x + 30, startY + sliders * rowH, panelW - 60, rowH - 6 };
        SDL_SetRenderDrawColor(renderer, 50, 80, 120, 200);
        SDL_RenderFillRect(renderer, &rowRect2);
        SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
        SDL_RenderDrawRect(renderer, &rowRect2);
        renderText("Music Theme", rowRect2.x + 10, rowRect2.y + 12);
        static bool themeOpen = false;
        static int selectedThemeIndexUI = 0; // 0 Main, 1 Fast
        SDL_Rect ddl{ rowRect2.x + rowRect2.w - 320, rowRect2.y + 8, 260, rowRect2.h - 16 };
        SDL_SetRenderDrawColor(renderer, 60, 60, 100, 255); SDL_RenderFillRect(renderer, &ddl);
        SDL_SetRenderDrawColor(renderer, 255,255,255,255); SDL_RenderDrawRect(renderer, &ddl);
        renderText(selectedThemeIndexUI == 0 ? "Main" : "Fast", ddl.x + 10, ddl.y + 8);
        if (mouseDown && !lastMouseDown && mouseX>=ddl.x && mouseX<=ddl.x+ddl.w && mouseY>=ddl.y && mouseY<=ddl.y+ddl.h) themeOpen = !themeOpen;
        if (themeOpen) {
            SDL_Rect opt1{ ddl.x, ddl.y + ddl.h + 4, ddl.w, ddl.h };
            SDL_Rect opt2{ ddl.x, ddl.y + 2*ddl.h + 6, ddl.w, ddl.h };
            SDL_SetRenderDrawColor(renderer, 70,120,170,255); SDL_RenderFillRect(renderer, &opt1);
            SDL_SetRenderDrawColor(renderer, 255,255,255,255); SDL_RenderDrawRect(renderer, &opt1);
            renderText("Main", opt1.x + 10, opt1.y + 8);
            SDL_SetRenderDrawColor(renderer, 70,120,170,255); SDL_RenderFillRect(renderer, &opt2);
            SDL_SetRenderDrawColor(renderer, 255,255,255,255); SDL_RenderDrawRect(renderer, &opt2);
            renderText("Fast", opt2.x + 10, opt2.y + 8);
            if (mouseDown && !lastMouseDown) {
                if (mouseX>=opt1.x && mouseX<=opt1.x+opt1.w && mouseY>=opt1.y && mouseY<=opt1.y+opt1.h) { outResult.newThemeIndex = 0; selectedThemeIndexUI = 0; themeOpen = false; }
                if (mouseX>=opt2.x && mouseX<=opt2.x+opt2.w && mouseY>=opt2.y && mouseY<=opt2.y+opt2.h) { outResult.newThemeIndex = 1; selectedThemeIndexUI = 1; themeOpen = false; }
            }
        } else {
            // Ensure dropdown state doesn't block other input
            if (mouseDown && !lastMouseDown && !(mouseX>=ddl.x && mouseX<=ddl.x+ddl.w && mouseY>=ddl.y && mouseY<=ddl.y+ddl.h)) {
                themeOpen = false;
            }
        }
        // Update global mouse edge state at end of function
        lastMouseDown = mouseDown;
    } else if (activeTab == OptionsTab::Video) {
        // Video: Fullscreen and VSync toggles
        const int rows = 3;
        for (int i = 0; i < rows; ++i) {
            SDL_Rect rowRect{ panel.x + 30, startY + i * rowH, panelW - 60, rowH - 6 };
            bool selected = (mouseX >= rowRect.x && mouseX <= rowRect.x + rowRect.w && mouseY >= rowRect.y && mouseY <= rowRect.y + rowRect.h);
            SDL_SetRenderDrawColor(renderer, selected ? 70 : 50, selected ? 120 : 50, selected ? 180 : 70, 200);
            SDL_RenderFillRect(renderer, &rowRect);
            SDL_SetRenderDrawColor(renderer, 220,220,220,255);
            SDL_RenderDrawRect(renderer, &rowRect);
            const char* labels[3] = { "Fullscreen", "VSync", "Stop Monster Spawns" };
            renderText(labels[i], rowRect.x + 10, rowRect.y + 12);
            SDL_Rect valueArea{ rowRect.x + rowRect.w - 220, rowRect.y + 8, 200, rowRect.h - 16 };
            SDL_SetRenderDrawColor(renderer, 80, 120, 160, 255);
            SDL_RenderFillRect(renderer, &valueArea);
            SDL_SetRenderDrawColor(renderer, 255,255,255,255);
            SDL_RenderDrawRect(renderer, &valueArea);
            if (i == 0) {
                renderText(fullscreenEnabled ? "On" : "Off", valueArea.x + 12, valueArea.y + 8);
                if (mouseDown && mouseX >= valueArea.x && mouseX <= valueArea.x + valueArea.w && mouseY >= valueArea.y && mouseY <= valueArea.y + valueArea.h) {
                    outResult.clickedFullscreen = true;
                }
            } else if (i == 1) {
                renderText(vsyncEnabled ? "On" : "Off", valueArea.x + 12, valueArea.y + 8);
                // VSync toggle disabled (renderer created with vsync)
            } else if (i == 2) {
                static bool stopSpawnUi = false;
                renderText(stopSpawnUi ? "On" : "Off", valueArea.x + 12, valueArea.y + 8);
                if (mouseDown && mouseX >= valueArea.x && mouseX <= valueArea.x + valueArea.w && mouseY >= valueArea.y && mouseY <= valueArea.y + valueArea.h && !lastMouseDown) {
                    stopSpawnUi = !stopSpawnUi;
                    outResult.toggledStopSpawns = true; outResult.stopSpawnsNewValue = stopSpawnUi;
                }
            }
        }
    }

    // Place helper text near the bottom, below the last row
    renderTextCentered("Use mouse to drag sliders and click buttons. Press Esc to close.",
                       outW/2, panel.y + panelH - 12, SDL_Color{200,200,200,255});

    // Always update edge-trigger memory regardless of active tab to avoid input lock-ups
    lastMouseDown = mouseDown;
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

// Enhanced inventory UI implementation
UISystem::InventoryHit UISystem::renderEnhancedInventory(Player* player, bool& isOpen, bool anvilOpen, Game* game, bool equipmentOpen) {
    InventoryHit hit;
    if (!player || !player->getItemSystem()) return hit;
    
    ItemSystem* itemSystem = player->getItemSystem();
    
    // Get screen dimensions
    int screenW, screenH;
    SDL_GetRendererOutputSize(renderer, &screenW, &screenH);
    
    // Inventory panel - use custom position if set, otherwise use default positioning
    int panelW = 800;
    int panelH = 600;
    int panelX, panelY;
    
    // Check for custom position from game
    if (game && game->getInventoryPosX() >= 0 && game->getInventoryPosY() >= 0) {
        panelX = game->getInventoryPosX();
        panelY = game->getInventoryPosY();
    } else {
        // Default positioning - adjust based on what else is open
        if (anvilOpen) {
            // Position on right side of screen when anvil is open
            panelX = screenW * 3 / 4 - panelW / 2;
            panelY = (screenH - panelH) / 2;
        } else if (equipmentOpen) {
            // Position on left side when equipment UI is open
            panelX = 50;
            panelY = (screenH - panelH) / 2;
        } else {
            // Center when nothing else is open
            panelX = (screenW - panelW) / 2;
            panelY = (screenH - panelH) / 2;
        }
    }
    
    // Background panel
    SDL_Rect panel = {panelX, panelY, panelW, panelH};
    SDL_SetRenderDrawColor(renderer, 40, 40, 60, 240);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &panel);
    
    // Title
    renderTextCentered("Inventory", panelX + panelW/2, panelY + 20);
    
    // Close button (X in top right)
    SDL_Rect closeBtn = {panelX + panelW - 40, panelY + 10, 30, 30};
    SDL_SetRenderDrawColor(renderer, 160, 40, 40, 255);
    SDL_RenderFillRect(renderer, &closeBtn);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &closeBtn);
    renderTextCentered("X", closeBtn.x + closeBtn.w/2, closeBtn.y + closeBtn.h/2);
    
    // Split into three sections: Equipment (left), Scrolls (middle), Resources (right)
    int sectionW = (panelW - 80) / 3;
    int equipmentX = panelX + 20;
    int scrollsX = panelX + 30 + sectionW;
    int resourcesX = panelX + 40 + 2 * sectionW;
    int sectionY = panelY + 60;
    int sectionH = panelH - 100;
    
    // Equipment section
    SDL_Rect equipmentSection = {equipmentX, sectionY, sectionW, sectionH};
    SDL_SetRenderDrawColor(renderer, 30, 30, 45, 255);
    SDL_RenderFillRect(renderer, &equipmentSection);
    SDL_SetRenderDrawColor(renderer, 120, 120, 140, 255);
    SDL_RenderDrawRect(renderer, &equipmentSection);
    
    renderText("Equipment", equipmentX + 10, sectionY + 10);
    
    // Draw equipment grid (4 columns, 12 rows) - adjusted for narrower section
    int slotSize = 36;  // Smaller slots for three-column layout
    int slotSpacing = 4;  // Reduced spacing
    int equipmentCols = 4;  // Fewer columns due to narrower section
    int equipmentRows = 12;  // More rows to maintain slot count
    int equipmentGridStartX = equipmentX + 10;  // Better left margin  
    int equipmentGridStartY = sectionY + 35;  // Better top margin
    
    const auto& itemInventory = itemSystem->getItemInventory();
    
    for (int row = 0; row < equipmentRows; row++) {
        for (int col = 0; col < equipmentCols; col++) {
            int slotIndex = row * equipmentCols + col;
            if (slotIndex >= ItemSystem::INVENTORY_SIZE) break;
            
            int slotX = equipmentGridStartX + col * (slotSize + slotSpacing);
            int slotY = equipmentGridStartY + row * (slotSize + slotSpacing);
            SDL_Rect slotRect = {slotX, slotY, slotSize, slotSize};
            
            // Slot background
            SDL_SetRenderDrawColor(renderer, 50, 50, 70, 255);
            SDL_RenderFillRect(renderer, &slotRect);
            SDL_SetRenderDrawColor(renderer, 100, 100, 120, 255);
            SDL_RenderDrawRect(renderer, &slotRect);
            
            // Item icon and info
            if (!itemInventory[slotIndex].isEmpty()) {
                Item* item = itemInventory[slotIndex].item;
                
                // Draw rarity border
                SDL_Color rarityColor = item->getRarityColor();
                SDL_SetRenderDrawColor(renderer, rarityColor.r, rarityColor.g, rarityColor.b, 255);
                SDL_RenderDrawRect(renderer, &slotRect);
                
                // Draw item icon if available
                Texture* icon = itemSystem->getItemIcon(item->id);
                if (icon) {
                    SDL_Rect srcRect = {0, 0, 0, 0};
                    SDL_QueryTexture(icon->getTexture(), nullptr, nullptr, &srcRect.w, &srcRect.h);
                    SDL_Rect dstRect = {slotX + 2, slotY + 2, slotSize - 4, slotSize - 4};
                    SDL_RenderCopy(renderer, icon->getTexture(), &srcRect, &dstRect);
                }
                
                // Draw stack count if > 1
                if (item->currentStack > 1) {
                    std::string stackText = std::to_string(item->currentStack);
                    renderText(stackText, slotX + slotSize - 15, slotY + slotSize - 15, {255, 255, 255, 255});
                }
            }
            
            // Check for mouse clicks on this slot
            int mx, my;
            Uint32 mouseState = SDL_GetMouseState(&mx, &my);
            bool leftClick = (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
            bool rightClick = (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
            
            if (mx >= slotX && mx <= slotX + slotSize && my >= slotY && my <= slotY + slotSize) {
                if (leftClick) {
                    hit.clickedItemSlot = slotIndex;
                }
                if (rightClick) {
                    hit.rightClicked = true;
                    hit.clickedItemSlot = slotIndex;
                }
                
                // Show tooltip on hover
                if (!itemInventory[slotIndex].isEmpty()) {
                    Item* item = itemInventory[slotIndex].item;
                    renderTooltip(item->getTooltipText(), mx, my);
                }
            }
        }
    }
    
    // Scrolls section
    SDL_Rect scrollsSection = {scrollsX, sectionY, sectionW, sectionH};
    SDL_SetRenderDrawColor(renderer, 45, 30, 30, 255);
    SDL_RenderFillRect(renderer, &scrollsSection);
    SDL_SetRenderDrawColor(renderer, 140, 120, 120, 255);
    SDL_RenderDrawRect(renderer, &scrollsSection);
    
    renderText("Scrolls", scrollsX + 10, sectionY + 10);
    
    // Draw scroll grid (3 columns, 7 rows) with improved spacing for narrower section
    int scrollCols = 3;
    int scrollRows = 7;
    int scrollGridStartX = scrollsX + 10;  // Better left margin
    int scrollGridStartY = sectionY + 35;  // Better top margin
    
    const auto& scrollInventory = itemSystem->getScrollInventory();
    
    for (int row = 0; row < scrollRows; row++) {
        for (int col = 0; col < scrollCols; col++) {
            int slotIndex = row * scrollCols + col;
            if (slotIndex >= ItemSystem::SCROLL_INVENTORY_SIZE) break;
            
            int slotX = scrollGridStartX + col * (slotSize + slotSpacing);
            int slotY = scrollGridStartY + row * (slotSize + slotSpacing);
            SDL_Rect slotRect = {slotX, slotY, slotSize, slotSize};
            
            // Slot background
            SDL_SetRenderDrawColor(renderer, 70, 50, 50, 255);
            SDL_RenderFillRect(renderer, &slotRect);
            SDL_SetRenderDrawColor(renderer, 120, 100, 100, 255);
            SDL_RenderDrawRect(renderer, &slotRect);
            
            // Scroll icon and info
            if (!scrollInventory[slotIndex].isEmpty()) {
                Item* scroll = scrollInventory[slotIndex].item;
                
                // Draw rarity border
                SDL_Color rarityColor = scroll->getRarityColor();
                SDL_SetRenderDrawColor(renderer, rarityColor.r, rarityColor.g, rarityColor.b, 255);
                SDL_RenderDrawRect(renderer, &slotRect);
                
                // Draw scroll icon if available
                Texture* icon = itemSystem->getItemIcon(scroll->id);
                if (icon) {
                    SDL_Rect srcRect = {0, 0, 0, 0};
                    SDL_QueryTexture(icon->getTexture(), nullptr, nullptr, &srcRect.w, &srcRect.h);
                    SDL_Rect dstRect = {slotX + 2, slotY + 2, slotSize - 4, slotSize - 4};
                    SDL_RenderCopy(renderer, icon->getTexture(), &srcRect, &dstRect);
                }
                
                // Draw stack count if > 1
                if (scroll->currentStack > 1) {
                    std::string stackText = std::to_string(scroll->currentStack);
                    renderText(stackText, slotX + slotSize - 15, slotY + slotSize - 15, {255, 255, 255, 255});
                }
            }
            
            // Check for mouse clicks on this scroll slot
            int mx, my;
            Uint32 mouseState = SDL_GetMouseState(&mx, &my);
            bool leftClick = (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
            bool rightClick = (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
            
            if (mx >= slotX && mx <= slotX + slotSize && my >= slotY && my <= slotY + slotSize) {
                if (leftClick) {
                    hit.clickedScrollSlot = slotIndex;
                }
                if (rightClick) {
                    hit.rightClicked = true;
                    hit.clickedScrollSlot = slotIndex;
                }
                
                // Show tooltip on hover for scrolls
                if (!scrollInventory[slotIndex].isEmpty()) {
                    Item* scroll = scrollInventory[slotIndex].item;
                    renderTooltip(scroll->getTooltipText(), mx, my);
                }
            }
        }
    }
    
    // Resources section
    SDL_Rect resourcesSection = {resourcesX, sectionY, sectionW, sectionH};
    SDL_SetRenderDrawColor(renderer, 30, 45, 30, 255);
    SDL_RenderFillRect(renderer, &resourcesSection);
    SDL_SetRenderDrawColor(renderer, 120, 140, 120, 255);
    SDL_RenderDrawRect(renderer, &resourcesSection);
    
    renderText("Resources", resourcesX + 10, sectionY + 10);
    
    // Draw resource grid (3 columns, 10 rows) 
    int resourceCols = 3;
    int resourceRows = 10;
    int resourceGridStartX = resourcesX + 10;  // Better left margin
    int resourceGridStartY = sectionY + 35;  // Better top margin
    
    const auto& resourceInventory = itemSystem->getResourceInventory();
    
    for (int row = 0; row < resourceRows; row++) {
        for (int col = 0; col < resourceCols; col++) {
            int slotIndex = row * resourceCols + col;
            if (slotIndex >= ItemSystem::RESOURCE_INVENTORY_SIZE) break;
            
            int slotX = resourceGridStartX + col * (slotSize + slotSpacing);
            int slotY = resourceGridStartY + row * (slotSize + slotSpacing);
            SDL_Rect slotRect = {slotX, slotY, slotSize, slotSize};
            
            // Slot background
            SDL_SetRenderDrawColor(renderer, 50, 70, 50, 255);
            SDL_RenderFillRect(renderer, &slotRect);
            SDL_SetRenderDrawColor(renderer, 100, 120, 100, 255);
            SDL_RenderDrawRect(renderer, &slotRect);
            
            // Resource icon and info
            if (!resourceInventory[slotIndex].isEmpty()) {
                Item* resource = resourceInventory[slotIndex].item;
                
                // Draw rarity border
                SDL_Color rarityColor = resource->getRarityColor();
                SDL_SetRenderDrawColor(renderer, rarityColor.r, rarityColor.g, rarityColor.b, 255);
                SDL_RenderDrawRect(renderer, &slotRect);
                
                // Draw resource icon if available
                Texture* icon = itemSystem->getItemIcon(resource->id);
                if (icon) {
                    SDL_Rect srcRect = {0, 0, 0, 0};
                    SDL_QueryTexture(icon->getTexture(), nullptr, nullptr, &srcRect.w, &srcRect.h);
                    SDL_Rect dstRect = {slotX + 2, slotY + 2, slotSize - 4, slotSize - 4};
                    SDL_RenderCopy(renderer, icon->getTexture(), &srcRect, &dstRect);
                }
                
                // Draw stack count (resources can stack high)
                if (resource->currentStack > 1) {
                    std::string stackText = std::to_string(resource->currentStack);
                    renderText(stackText, slotX + slotSize - 15, slotY + slotSize - 15, {255, 255, 255, 255});
                }
            }
            
            // Check for mouse clicks on this resource slot
            int mx, my;
            Uint32 mouseState = SDL_GetMouseState(&mx, &my);
            bool leftClick = (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
            bool rightClick = (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
            
            if (mx >= slotX && mx <= slotX + slotSize && my >= slotY && my <= slotY + slotSize) {
                if (leftClick) {
                    hit.clickedResourceSlot = slotIndex;
                }
                if (rightClick) {
                    hit.rightClicked = true;
                    hit.clickedResourceSlot = slotIndex;
                }
                
                // Show tooltip on hover for resources
                if (!resourceInventory[slotIndex].isEmpty()) {
                    Item* resource = resourceInventory[slotIndex].item;
                    renderTooltip(resource->getTooltipText(), mx, my);
                }
            }
        }
    }
    
    // Handle close button and ESC key
    int mx, my;
    Uint32 mouseState = SDL_GetMouseState(&mx, &my);
    bool leftClick = (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    
    // Check title bar for dragging (top 40 pixels of panel, excluding close button)
    SDL_Rect titleBar = {panelX, panelY, panelW - 60, 40};  // Leave space for close button
    if (leftClick && mx >= titleBar.x && mx <= titleBar.x + titleBar.w && 
        my >= titleBar.y && my <= titleBar.y + titleBar.h) {
        hit.titleBarClicked = true;
        hit.dragOffsetX = mx - panelX;
        hit.dragOffsetY = my - panelY;
    }
    
    // Check close button
    if (leftClick && mx >= closeBtn.x && mx <= closeBtn.x + closeBtn.w && 
        my >= closeBtn.y && my <= closeBtn.y + closeBtn.h) {
        hit.clickedClose = true;
        isOpen = false;
    }
    
    // ESC key to close
    const Uint8* keystate = SDL_GetKeyboardState(nullptr);
    if (keystate[SDL_SCANCODE_ESCAPE]) {
        isOpen = false;
        hit.clickedClose = true;
    }
    
    return hit;
}

UISystem::EquipmentHit UISystem::renderEquipmentUI(Player* player, bool& isOpen, bool anvilOpen, Game* game, bool inventoryOpen) {
    EquipmentHit hit;
    if (!player) return hit;
    
    // Get screen dimensions
    int screenW, screenH;
    SDL_GetRendererOutputSize(renderer, &screenW, &screenH);
    
    // Equipment panel - use custom position if set, otherwise use default positioning
    int panelW = 600;
    int panelH = 500;
    int panelX, panelY;
    
    // Check for custom position from game
    if (game && game->getEquipmentPosX() >= 0 && game->getEquipmentPosY() >= 0) {
        panelX = game->getEquipmentPosX();
        panelY = game->getEquipmentPosY();
    } else {
        // Default positioning - adjust based on what else is open
        if (anvilOpen) {
            // Position on right side when anvil is open (offset further right than inventory)
            panelX = screenW - panelW - 50;  // Right edge with some margin
            panelY = (screenH - panelH) / 2;
        } else if (inventoryOpen) {
            // Position on right side when inventory is open
            panelX = screenW - panelW - 50;
            panelY = (screenH - panelH) / 2;
        } else {
            // Center when nothing else is open
            panelX = (screenW - panelW) / 2;
            panelY = (screenH - panelH) / 2;
        }
    }
    
    // Background panel
    SDL_Rect panel = {panelX, panelY, panelW, panelH};
    SDL_SetRenderDrawColor(renderer, 60, 40, 40, 240);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &panel);
    
    // Title
    renderTextCentered("Equipment & Stats", panelX + panelW/2, panelY + 20);
    
    // Close button
    SDL_Rect closeBtn = {panelX + panelW - 40, panelY + 10, 30, 30};
    SDL_SetRenderDrawColor(renderer, 160, 40, 40, 255);
    SDL_RenderFillRect(renderer, &closeBtn);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &closeBtn);
    renderTextCentered("X", closeBtn.x + closeBtn.w/2, closeBtn.y + closeBtn.h/2);
    
    // Equipment slots layout (3x3 grid plus stats)
    int equipX = panelX + 50;
    int equipY = panelY + 60;
    int slotSize = 60;
    int slotSpacing = 10;
    
    // Note: We'll get equipment directly from player, not from ItemSystem slots
    
    // Equipment slot positions - new layout as requested:
    // Row 1: helmet (center top)
    // Row 2: necklace (left), chest (middle), ring (right) 
    // Row 3: sword (left), belt (middle), bow (right)
    // Row 4: gloves (left), boots (right)
    int slotPositions[9][2] = {
        {equipX + (slotSize + slotSpacing) * 2, equipY + slotSize + slotSpacing},    // Ring (row 2, right) - slot 0
        {equipX + slotSize + slotSpacing, equipY},                                  // Helmet (row 1, center) - slot 1
        {equipX, equipY + slotSize + slotSpacing},                                  // Necklace (row 2, left) - slot 2
        {equipX, equipY + (slotSize + slotSpacing) * 2},                            // Sword (row 3, left) - slot 3
        {equipX + slotSize + slotSpacing, equipY + slotSize + slotSpacing},         // Chest (row 2, middle) - slot 4
        {equipX + (slotSize + slotSpacing) * 2, equipY + (slotSize + slotSpacing) * 2}, // Bow/Shield (row 3, right) - slot 5
        {equipX, equipY + (slotSize + slotSpacing) * 3},                            // Gloves (row 4, left) - slot 6
        {equipX + slotSize + slotSpacing, equipY + (slotSize + slotSpacing) * 2},   // Belt/Waist (row 3, middle) - slot 7
        {equipX + (slotSize + slotSpacing) * 2, equipY + (slotSize + slotSpacing) * 3}  // Boots (row 4, right) - slot 8
    };
    
    // Get ItemSystem once for all slots
    ItemSystem* itemSystem = player->getItemSystem();
    
    for (int i = 0; i < 9; i++) {
        int slotX = slotPositions[i][0];
        int slotY = slotPositions[i][1];
        SDL_Rect slotRect = {slotX, slotY, slotSize, slotSize};
        
        // Slot background
        SDL_SetRenderDrawColor(renderer, 70, 50, 70, 255);
        SDL_RenderFillRect(renderer, &slotRect);
        SDL_SetRenderDrawColor(renderer, 120, 100, 120, 255);
        SDL_RenderDrawRect(renderer, &slotRect);
        
        // Get equipped item from ItemSystem first, fallback to Player equipment
        Item* equippedItem = nullptr;
        if (itemSystem && i < itemSystem->getEquipmentSlots().size()) {
            const auto& equipSlots = itemSystem->getEquipmentSlots();
            if (!equipSlots[i].isEmpty()) {
                equippedItem = equipSlots[i].item;
            }
        }
        
        // Fallback to Player's equipment array if ItemSystem doesn't have the item
        const Player::EquipmentItem* playerEquipItem = nullptr;
        if (!equippedItem) {
            const Player::EquipmentItem& equipItem = player->getEquipment(static_cast<Player::EquipmentSlot>(i));
            if (!equipItem.name.empty() || equipItem.plusLevel > 0 || equipItem.basePower > 0) {
                playerEquipItem = &equipItem;
            }
        }
        
        // Variables for displaying item info
        std::string tooltipText;
        
        // Draw equipped item if one exists
        if (equippedItem || playerEquipItem) {
            // Map equipment slot to icon path
            std::string iconPath;
            switch (static_cast<Player::EquipmentSlot>(i)) {
                case Player::EquipmentSlot::RING:
                    iconPath = "assets/Textures/Items/ring_01.png";
                    break;
                case Player::EquipmentSlot::HELM:
                    iconPath = "assets/Textures/Items/helmet_01.png";
                    break;
                case Player::EquipmentSlot::NECKLACE:
                    iconPath = "assets/Textures/Items/necklace_01.png";
                    break;
                case Player::EquipmentSlot::SWORD:
                    iconPath = "assets/Textures/Items/sword_01.png";
                    break;
                case Player::EquipmentSlot::CHEST:
                    iconPath = "assets/Textures/Items/chestpeice_01.png";
                    break;
                case Player::EquipmentSlot::SHIELD:
                    iconPath = "assets/Textures/Items/bow_01.png";
                    break;
                case Player::EquipmentSlot::GLOVE:
                    iconPath = "assets/Textures/Items/gloves_01.png";
                    break;
                case Player::EquipmentSlot::WAIST:
                    iconPath = "assets/Textures/Items/waist_01.png";
                    break;
                case Player::EquipmentSlot::FEET:
                    iconPath = "assets/Textures/Items/boots_01.png";
                    break;
            }
            
            // Get icon and display data based on item source
            Texture* icon = nullptr;
            int plusLevel = 0;
            SDL_Color rarityColor = {255, 255, 255, 255}; // White default
            
            if (equippedItem) {
                // ItemSystem item
                icon = itemSystem->getItemIcon(equippedItem->id);
                plusLevel = equippedItem->plusLevel;
                rarityColor = equippedItem->getRarityColor();
                tooltipText = equippedItem->getTooltipText();
            } else if (playerEquipItem) {
                // Player equipment item
                plusLevel = playerEquipItem->plusLevel;
                tooltipText = playerEquipItem->name + " (+" + std::to_string(plusLevel) + ")";
            }
            
            // Fallback to slot-specific icon if no ItemSystem icon
            if (!icon && !iconPath.empty()) {
                icon = assetManager->getTexture(iconPath);
            }
            
            // Draw equipment icon
            if (icon) {
                int pad = 4;
                SDL_Rect iconRect = {slotX + pad, slotY + pad, slotSize - pad*2, slotSize - pad*2};
                SDL_RenderCopy(renderer, icon->getTexture(), nullptr, &iconRect);
            }
            
            // Draw +level for all equipment (including +0)
            if (equippedItem || playerEquipItem) {
                renderText("+" + std::to_string(plusLevel), 
                          slotX + slotSize - 20, slotY + slotSize - 18, {200, 255, 200, 255});
            }
            
            // Draw rarity border (only for ItemSystem items)
            if (equippedItem) {
                SDL_SetRenderDrawColor(renderer, rarityColor.r, rarityColor.g, rarityColor.b, 255);
                SDL_Rect borderRect = {slotX - 2, slotY - 2, slotSize + 4, slotSize + 4};
                SDL_RenderDrawRect(renderer, &borderRect);
            }
            
            // Draw elemental indicators
            int indicatorY = slotY + 2;
            if (equippedItem) {
                // ItemSystem item elemental indicators
                if (equippedItem->stats.fireAttack > 0) {
                    SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);
                    SDL_Rect fireRect = {slotX + 2, indicatorY, 8, 4};
                    SDL_RenderFillRect(renderer, &fireRect);
                    indicatorY += 6;
                }
                if (equippedItem->stats.waterAttack > 0) {
                    SDL_SetRenderDrawColor(renderer, 100, 200, 255, 255);
                    SDL_Rect iceRect = {slotX + 2, indicatorY, 8, 4};
                    SDL_RenderFillRect(renderer, &iceRect);
                    indicatorY += 6;
                }
                if (equippedItem->stats.poisonAttack > 0) {
                    SDL_SetRenderDrawColor(renderer, 100, 255, 100, 255);
                    SDL_Rect poisonRect = {slotX + 2, indicatorY, 8, 4};
                    SDL_RenderFillRect(renderer, &poisonRect);
                }
            } else if (playerEquipItem) {
                // Player equipment elemental indicators
                if (playerEquipItem->fire > 0) {
                    SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);
                    SDL_Rect fireRect = {slotX + 2, indicatorY, 8, 4};
                    SDL_RenderFillRect(renderer, &fireRect);
                    indicatorY += 6;
                }
                if (playerEquipItem->ice > 0) {
                    SDL_SetRenderDrawColor(renderer, 100, 200, 255, 255);
                    SDL_Rect iceRect = {slotX + 2, indicatorY, 8, 4};
                    SDL_RenderFillRect(renderer, &iceRect);
                    indicatorY += 6;
                }
                if (playerEquipItem->poison > 0) {
                    SDL_SetRenderDrawColor(renderer, 100, 255, 100, 255);
                    SDL_Rect poisonRect = {slotX + 2, indicatorY, 8, 4};
                    SDL_RenderFillRect(renderer, &poisonRect);
                }
            }
        }
        
        // Check for mouse clicks on equipment slot
        int mx, my;
        Uint32 mouseState = SDL_GetMouseState(&mx, &my);
        bool leftClick = (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
        bool rightClick = (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
        
        if (mx >= slotX && mx <= slotX + slotSize && my >= slotY && my <= slotY + slotSize) {
            if (leftClick) {
                hit.clickedEquipSlot = i;
            }
            if (rightClick) {
                hit.rightClicked = true;
                hit.clickedEquipSlot = i;
            }
            
            // Show tooltip on hover for equipped items
            if (!tooltipText.empty()) {
                renderTooltip(tooltipText, mx, my);
            }
        }
    }
    
    // Stats display - use ItemSystem's calculation
    ItemStats totalStats;
    if (itemSystem) {
        totalStats = itemSystem->calculateTotalStats();
    }
    int statsX = panelX + 350;
    int statsY = panelY + 80;
    int lineHeight = 25;
    
    renderText("Total Stats:", statsX, statsY, {255, 255, 100, 255});
    statsY += lineHeight + 10;
    
    if (totalStats.attack > 0) {
        renderText("Attack: +" + std::to_string(totalStats.attack), statsX, statsY);
        statsY += lineHeight;
    }
    if (totalStats.defense > 0) {
        renderText("Defense: +" + std::to_string(totalStats.defense), statsX, statsY);
        statsY += lineHeight;
    }
    if (totalStats.health > 0) {
        renderText("Health: +" + std::to_string(totalStats.health), statsX, statsY);
        statsY += lineHeight;
    }
    if (totalStats.mana > 0) {
        renderText("Mana: +" + std::to_string(totalStats.mana), statsX, statsY);
        statsY += lineHeight;
    }
    if (totalStats.strength > 0) {
        renderText("Strength: +" + std::to_string(totalStats.strength), statsX, statsY);
        statsY += lineHeight;
    }
    if (totalStats.intelligence > 0) {
        renderText("Intelligence: +" + std::to_string(totalStats.intelligence), statsX, statsY);
        statsY += lineHeight;
    }
    
    // Elemental stats
    if (totalStats.fireAttack > 0) {
        renderText("Fire Attack: +" + std::to_string(totalStats.fireAttack), statsX, statsY, {255, 100, 100, 255});
        statsY += lineHeight;
    }
    if (totalStats.waterAttack > 0) {
        renderText("Water Attack: +" + std::to_string(totalStats.waterAttack), statsX, statsY, {100, 100, 255, 255});
        statsY += lineHeight;
    }
    if (totalStats.poisonAttack > 0) {
        renderText("Poison Attack: +" + std::to_string(totalStats.poisonAttack), statsX, statsY, {100, 255, 100, 255});
        statsY += lineHeight;
    }
    
    // Handle input
    int mx, my;
    Uint32 mouseState = SDL_GetMouseState(&mx, &my);
    bool leftClick = (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    
    // Check title bar for dragging (top 40 pixels of panel, excluding close button)
    SDL_Rect titleBar = {panelX, panelY, panelW - 60, 40};  // Leave space for close button
    if (leftClick && mx >= titleBar.x && mx <= titleBar.x + titleBar.w && 
        my >= titleBar.y && my <= titleBar.y + titleBar.h) {
        hit.titleBarClicked = true;
        hit.dragOffsetX = mx - panelX;
        hit.dragOffsetY = my - panelY;
    }
    
    // Check close button
    if (leftClick && mx >= closeBtn.x && mx <= closeBtn.x + closeBtn.w && 
        my >= closeBtn.y && my <= closeBtn.y + closeBtn.h) {
        hit.clickedClose = true;
        isOpen = false;
    }
    
    // ESC key to close
    const Uint8* keystate = SDL_GetKeyboardState(nullptr);
    if (keystate[SDL_SCANCODE_ESCAPE]) {
        isOpen = false;
        hit.clickedClose = true;
    }
    
    return hit;
}

void UISystem::renderTooltip(const std::string& tooltipText, int mouseX, int mouseY) {
    if (tooltipText.empty()) return;
    
    // Calculate tooltip size
    int maxWidth = 300;
    int lineHeight = 20;
    int padding = 10;
    
    // Split tooltip into lines (basic line breaking)
    std::vector<std::string> lines;
    std::istringstream stream(tooltipText);
    std::string line;
    while (std::getline(stream, line, '\n')) {
        lines.push_back(line);
    }
    
    int tooltipW = maxWidth;
    int tooltipH = lines.size() * lineHeight + padding * 2;
    
    // Position tooltip always ABOVE the mouse/item, keep on screen
    int screenW, screenH;
    SDL_GetRendererOutputSize(renderer, &screenW, &screenH);
    
    // Always position tooltip above the mouse cursor
    int tooltipX = mouseX - tooltipW / 2;  // Center horizontally on mouse
    int tooltipY = mouseY - tooltipH - 50; // Always above with more margin to clear icons
    
    // Keep tooltip on screen horizontally
    if (tooltipX < 10) tooltipX = 10;
    if (tooltipX + tooltipW > screenW - 10) tooltipX = screenW - tooltipW - 10;
    
    // If tooltip would go off top of screen, position it just below mouse instead
    if (tooltipY < 10) tooltipY = mouseY + 30;
    
    // Draw tooltip background
    SDL_Rect tooltipRect = {tooltipX, tooltipY, tooltipW, tooltipH};
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 240);
    SDL_RenderFillRect(renderer, &tooltipRect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &tooltipRect);
    
    // Draw tooltip text
    for (size_t i = 0; i < lines.size(); i++) {
        renderText(lines[i], tooltipX + padding, tooltipY + padding + i * lineHeight, {255, 255, 255, 255});
    }
}

void UISystem::renderSkillBar(const Player* player) {
    if (!player || !player->getSpellSystem() || !assetManager) return;
    
    // Get actual screen dimensions for both fullscreen and windowed mode
    int screenW, screenH;
    SDL_GetRendererOutputSize(renderer, &screenW, &screenH);
    
    int barWidth = SKILL_BAR_COLS * (SKILL_SLOT_SIZE + SKILL_SLOT_PADDING);
    int barHeight = SKILL_BAR_ROWS * (SKILL_SLOT_SIZE + SKILL_SLOT_PADDING);
    
    // Center horizontally, anchor to bottom edge
    skillBarX = (screenW - barWidth) / 2;
    skillBarY = screenH - barHeight - 20;
    
    // Draw background for skill bar
    SDL_Rect barBg = {
        skillBarX - 10,
        skillBarY - 10,
        barWidth + 20,
        barHeight + 20
    };
    SDL_SetRenderDrawColor(renderer, 20, 20, 30, 200);
    SDL_RenderFillRect(renderer, &barBg);
    
    // Draw border
    SDL_SetRenderDrawColor(renderer, 100, 100, 120, 255);
    SDL_RenderDrawRect(renderer, &barBg);
    
    // Get available spells from spell system
    auto spellSystem = player->getSpellSystem();
    auto availableSpells = spellSystem->getAvailableSpells();
    
    // Map spell types to icon paths
    std::unordered_map<int, std::string> spellIcons = {
        {static_cast<int>(SpellType::FIRE_BOLT), "assets/Textures/UI/firebolt_ui.png"},
        {static_cast<int>(SpellType::FLAME_WAVE), "assets/Textures/UI/flame_wave_ui.png"},
        {static_cast<int>(SpellType::METEOR_STRIKE), "assets/Textures/UI/meteor_strike_ui.png"},
        {static_cast<int>(SpellType::DRAGONS_BREATH), "assets/Textures/UI/dragons_breath_ui.png"}
    };
    
    // Add special abilities (always available)
    const std::string dashIcon = "assets/Textures/UI/dash_ui.png";
    const std::string fireShieldIcon = "assets/Textures/UI/fire_sheild_ui.png";
    
    // Render skill slots
    for (int row = 0; row < SKILL_BAR_ROWS; row++) {
        for (int col = 0; col < SKILL_BAR_COLS; col++) {
            int slotIndex = row * SKILL_BAR_COLS + col;
            int x = skillBarX + col * (SKILL_SLOT_SIZE + SKILL_SLOT_PADDING);
            int y = skillBarY + row * (SKILL_SLOT_SIZE + SKILL_SLOT_PADDING);
            
            // Draw slot background
            SDL_Rect slotRect = {x, y, SKILL_SLOT_SIZE, SKILL_SLOT_SIZE};
            SDL_SetRenderDrawColor(renderer, 40, 40, 50, 255);
            SDL_RenderFillRect(renderer, &slotRect);
            
            // Draw slot border
            SDL_SetRenderDrawColor(renderer, 80, 80, 100, 255);
            SDL_RenderDrawRect(renderer, &slotRect);
            
            // Special slots for dash and fire shield (swapped)
            if (slotIndex == 0) {
                // Dash ability (Spacebar key)
                auto* dashTex = assetManager->getTexture(dashIcon);
                if (dashTex) {
                    SDL_RenderCopy(renderer, dashTex->getTexture(), nullptr, &slotRect);
                }
                
                // Render cooldown overlay if on cooldown
                float dashCooldown = player->getDashCooldownRemaining();
                if (dashCooldown > 0) {
                    float cooldownPercent = dashCooldown / player->getDashCooldownMax();
                    int overlayHeight = static_cast<int>(SKILL_SLOT_SIZE * cooldownPercent);
                    SDL_Rect cooldownRect = {x, y + SKILL_SLOT_SIZE - overlayHeight, SKILL_SLOT_SIZE, overlayHeight};
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
                    SDL_RenderFillRect(renderer, &cooldownRect);
                    
                    // Render cooldown text
                    char cooldownText[8];
                    snprintf(cooldownText, sizeof(cooldownText), "%.1f", dashCooldown);
                    renderTextCentered(cooldownText, x + SKILL_SLOT_SIZE/2, y + SKILL_SLOT_SIZE/2, {255, 255, 255, 255});
                }
                
                // Render hotkey
                renderText("SPC", x + 2, y + SKILL_SLOT_SIZE - 14, {255, 255, 100, 255});
            }
            else if (slotIndex == 1 && player->hasFireShield()) {
                // Fire shield ability (F key)
                auto* shieldTex = assetManager->getTexture(fireShieldIcon);
                if (shieldTex) {
                    SDL_RenderCopy(renderer, shieldTex->getTexture(), nullptr, &slotRect);
                }
                renderText("F", x + 2, y + SKILL_SLOT_SIZE - 14, {255, 255, 100, 255});
            }
            else if (slotIndex >= 2 && slotIndex - 2 < static_cast<int>(availableSpells.size())) {
                // Regular spells
                int spellIdx = slotIndex - 2;
                auto spellType = availableSpells[spellIdx];
                
                // Get icon for this spell
                auto iconIt = spellIcons.find(static_cast<int>(spellType));
                if (iconIt != spellIcons.end()) {
                    auto* spellTex = assetManager->getTexture(iconIt->second);
                    if (spellTex) {
                        SDL_RenderCopy(renderer, spellTex->getTexture(), nullptr, &slotRect);
                    }
                }
                
                // Check cooldown
                float cooldown = spellSystem->getCooldownRemaining(spellType);
                if (cooldown > 0) {
                    float cooldownPercent = spellSystem->getCooldownPercent(spellType);
                    int overlayHeight = static_cast<int>(SKILL_SLOT_SIZE * cooldownPercent);
                    SDL_Rect cooldownRect = {x, y + SKILL_SLOT_SIZE - overlayHeight, SKILL_SLOT_SIZE, overlayHeight};
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
                    SDL_RenderFillRect(renderer, &cooldownRect);
                    
                    // Render cooldown text
                    char cooldownText[8];
                    snprintf(cooldownText, sizeof(cooldownText), "%.1f", cooldown);
                    renderTextCentered(cooldownText, x + SKILL_SLOT_SIZE/2, y + SKILL_SLOT_SIZE/2, {255, 255, 255, 255});
                }
                
                // Check mana cost
                int manaCost = spellSystem->getSpellManaCost(spellType);
                if (player->getMana() < manaCost) {
                    // Tint red if not enough mana
                    SDL_SetRenderDrawColor(renderer, 100, 0, 0, 100);
                    SDL_RenderFillRect(renderer, &slotRect);
                }
                
                // Render hotkey (3-6 or Q,E,R,F+)
                const char* hotkeys[] = {"3", "4", "5", "6", "Q", "E", "R", "T"};
                if (spellIdx < 8) {
                    renderText(hotkeys[spellIdx], x + 2, y + SKILL_SLOT_SIZE - 14, {255, 255, 100, 255});
                }
            }
        }
    }
}

void UISystem::renderSpellBook(const Player* player, int mouseX, int mouseY, bool mouseClicked) {
    if (!spellBookOpen || !player || !player->getSpellSystem() || !assetManager) return;
    
    // Get actual screen dimensions
    int screenW, screenH;
    SDL_GetRendererOutputSize(renderer, &screenW, &screenH);
    
    // Spell book window dimensions - larger for better layout
    int bookWidth = 800;
    int bookHeight = 600;
    int bookX = (screenW - bookWidth) / 2;
    int bookY = (screenH - bookHeight) / 2;
    
    // Draw main background
    SDL_Rect bookBg = {bookX, bookY, bookWidth, bookHeight};
    SDL_SetRenderDrawColor(renderer, 20, 15, 30, 250);
    SDL_RenderFillRect(renderer, &bookBg);
    
    // Draw decorative border
    SDL_SetRenderDrawColor(renderer, 180, 140, 70, 255);
    for (int i = 0; i < 4; i++) {
        SDL_Rect border = {bookX - i, bookY - i, bookWidth + i*2, bookHeight + i*2};
        SDL_RenderDrawRect(renderer, &border);
    }
    
    // Title with enhanced styling
    renderTextCentered("ARCANE SPELL COMPENDIUM", bookX + bookWidth/2, bookY + 15, {255, 215, 0, 255});
    
    // Tab system for spell schools
    int tabHeight = 40;
    int tabWidth = bookWidth / 4;
    int tabY = bookY + 45;
    
    // Static tab selection (for now, will be interactive later)
    static int selectedTab = 0; // 0=Fire, 1=Water, 2=Poison, 3=Lightning
    
    // Get spell system info
    auto spellSystem = player->getSpellSystem();
    auto activeElement = spellSystem->getActiveElement();
    const auto& weapon = player->getEquipment(Player::EquipmentSlot::SWORD);
    
    // Tab data
    struct TabInfo {
        const char* name;
        SDL_Color color;
        SpellElement element;
        int enchantLevel;
    };
    
    TabInfo tabs[4] = {
        {"Fire", {255, 100, 50, 255}, SpellElement::FIRE, weapon.fire},
        {"Water", {100, 180, 255, 255}, SpellElement::WATER, weapon.ice},
        {"Poison", {120, 255, 80, 255}, SpellElement::POISON, weapon.poison},
        {"Lightning", {255, 255, 100, 255}, SpellElement::NONE, 0} // Future implementation
    };
    
    // Auto-select tab based on active element
    for (int i = 0; i < 4; i++) {
        if (tabs[i].element == activeElement) {
            selectedTab = i;
            break;
        }
    }
    
    // Render tabs
    for (int i = 0; i < 4; i++) {
        int tabX = bookX + i * tabWidth;
        bool isSelected = (i == selectedTab);
        bool isActive = (tabs[i].element == activeElement && tabs[i].enchantLevel > 0);
        
        // Tab background
        SDL_Color bgColor = isSelected ? SDL_Color{60, 50, 80, 255} : SDL_Color{40, 35, 55, 255};
        if (isActive) {
            bgColor.r = std::min(255, bgColor.r + 20);
            bgColor.g = std::min(255, bgColor.g + 20);
            bgColor.b = std::min(255, bgColor.b + 20);
        }
        
        SDL_Rect tabRect = {tabX, tabY, tabWidth, tabHeight};
        SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        SDL_RenderFillRect(renderer, &tabRect);
        
        // Tab border
        SDL_Color borderColor = isSelected ? tabs[i].color : SDL_Color{100, 100, 100, 255};
        SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
        SDL_RenderDrawRect(renderer, &tabRect);
        
        // Tab text
        SDL_Color textColor = isActive ? tabs[i].color : SDL_Color{150, 150, 150, 255};
        char tabText[32];
        if (tabs[i].enchantLevel > 0) {
            snprintf(tabText, sizeof(tabText), "%s +%d", tabs[i].name, tabs[i].enchantLevel);
        } else {
            snprintf(tabText, sizeof(tabText), "%s", tabs[i].name);
        }
        renderTextCentered(tabText, tabX + tabWidth/2, tabY + tabHeight/2 - 8, textColor);
    }
    
    // Content area
    int contentY = tabY + tabHeight + 20;
    int contentHeight = bookHeight - (contentY - bookY) - 40;
    
    // Current tab spell grid
    TabInfo& currentTab = tabs[selectedTab];
    
    // Define spells for current tab
    std::vector<std::tuple<SpellType, int, const char*>> spells;
    
    if (selectedTab == 0) { // Fire
        spells = {
            // Active Spells
            {SpellType::FIRE_BOLT, 0, "firebolt_ui.png"},
            {SpellType::FLAME_WAVE, 10, "flame_wave_ui.png"},
            {SpellType::METEOR_STRIKE, 20, "meteor_strike_ui.png"},
            {SpellType::DRAGONS_BREATH, 30, "dragons_breath_ui.png"},
            // Passive Abilities
            {SpellType::FIRE_MASTERY, 5, "fire_mastery.png"},
            {SpellType::BURNING_AURA, 15, "burning_aura.png"},
            {SpellType::INFERNO_LORD, 25, "inferno_lord.png"}
        };
    } else if (selectedTab == 1) { // Water
        spells = {
            {SpellType::ICE_SHARD, 0, "firebolt_ui.png"}, // Placeholder
            {SpellType::FROZEN_GROUND, 10, "flame_wave_ui.png"}, // Placeholder
            {SpellType::BLIZZARD, 20, "meteor_strike_ui.png"}, // Placeholder
            {SpellType::ABSOLUTE_ZERO, 30, "dragons_breath_ui.png"} // Placeholder
        };
    } else if (selectedTab == 2) { // Poison
        spells = {
            {SpellType::TOXIC_DART, 0, "firebolt_ui.png"}, // Placeholder
            {SpellType::POISON_CLOUD, 10, "flame_wave_ui.png"}, // Placeholder
            {SpellType::PLAGUE_BOMB, 20, "meteor_strike_ui.png"}, // Placeholder
            {SpellType::DEATHS_EMBRACE, 30, "dragons_breath_ui.png"} // Placeholder
        };
    }
    
    // Spell grid layout - Active spells in top row (4), passives in bottom row (3)
    int spellSize = 64;
    int spellSpacing = 80;
    int gridStartX = bookX + 60;
    int gridStartY = contentY + 40;
    
    // Ensure selected spell index is valid for current tab
    if (selectedSpellIndex >= static_cast<int>(spells.size())) {
        selectedSpellIndex = 0;
    }
    
    // Add section headers
    renderText("ACTIVE SPELLS", gridStartX, gridStartY - 25, {255, 200, 100, 255});
    renderText("PASSIVE ABILITIES", gridStartX, gridStartY + spellSize + 60, {255, 200, 100, 255});
    
    for (size_t i = 0; i < spells.size(); i++) {
        auto [spellType, reqLevel, iconFile] = spells[i];
        
        int col, row;
        if (i < 4) {
            // Active spells: top row (4 columns)
            col = i % 4;
            row = 0;
        } else {
            // Passive abilities: bottom row (3 columns, centered)
            col = (i - 4) % 3;
            row = 1;
        }
        
        int spellX = gridStartX + col * spellSpacing;
        int spellY = gridStartY + row * (spellSize + 80); // Extra spacing between rows
        
        // Center the passive abilities row
        if (i >= 4) {
            spellX += spellSpacing / 2; // Offset to center 3 items
        }
        
        bool unlocked = currentTab.enchantLevel >= reqLevel;
        bool isActive = (currentTab.element == activeElement);
        
        // Spell icon background
        SDL_Rect iconBg = {spellX - 5, spellY - 5, spellSize + 10, spellSize + 10};
        SDL_Color bgColor = unlocked ? SDL_Color{60, 60, 60, 200} : SDL_Color{30, 30, 30, 200};
        SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        SDL_RenderFillRect(renderer, &iconBg);
        
        // Check for mouse click on this spell icon
        bool isSelected = (static_cast<int>(i) == selectedSpellIndex);
        if (mouseClicked) {
            SDL_Rect clickArea = {spellX - 5, spellY - 5, spellSize + 10, spellSize + 10};
            if (mouseX >= clickArea.x && mouseX <= clickArea.x + clickArea.w &&
                mouseY >= clickArea.y && mouseY <= clickArea.y + clickArea.h) {
                selectedSpellIndex = static_cast<int>(i);
                isSelected = true;
            }
        }
        
        // Spell icon border with selection highlighting
        bool isPassiveAbility = (spellType == SpellType::FIRE_MASTERY || 
                                 spellType == SpellType::BURNING_AURA || 
                                 spellType == SpellType::INFERNO_LORD);
        
        SDL_Color borderColor = unlocked ? currentTab.color : SDL_Color{80, 80, 80, 255};
        if (unlocked && isActive) {
            borderColor = {255, 255, 255, 255}; // White border for available spells
        }
        if (isPassiveAbility && unlocked) {
            borderColor = {180, 100, 255, 255}; // Purple border for passive abilities
        }
        if (isSelected) {
            borderColor = {255, 255, 0, 255}; // Yellow border for selected spell
            // Add extra thick border for selected spell
            for (int thickness = 0; thickness < 3; thickness++) {
                SDL_Rect thickBorder = {
                    spellX - 5 - thickness, 
                    spellY - 5 - thickness, 
                    spellSize + 10 + thickness*2, 
                    spellSize + 10 + thickness*2
                };
                SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
                SDL_RenderDrawRect(renderer, &thickBorder);
            }
        } else {
            SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
            SDL_RenderDrawRect(renderer, &iconBg);
        }
        
        // Spell icon
        std::string iconPath = "assets/Textures/UI/" + std::string(iconFile);
        Texture* iconTexture = assetManager->getTexture(iconPath);
        if (iconTexture) {
            SDL_Rect iconRect = {spellX, spellY, spellSize, spellSize};
            
            // Apply grayscale effect for locked spells
            if (!unlocked) {
                SDL_SetTextureColorMod(iconTexture->getTexture(), 100, 100, 100);
            } else {
                SDL_SetTextureColorMod(iconTexture->getTexture(), 255, 255, 255);
            }
            
            SDL_RenderCopy(renderer, iconTexture->getTexture(), nullptr, &iconRect);
        }
        
        // Spell name below icon
        std::string spellName = spellSystem->getSpellName(spellType);
        SDL_Color nameColor = unlocked ? SDL_Color{255, 255, 255, 255} : SDL_Color{120, 120, 120, 255};
        renderTextCentered(spellName, spellX + spellSize/2, spellY + spellSize + 10, nameColor);
        
        // Requirement text
        char reqText[32];
        snprintf(reqText, sizeof(reqText), "Req: +%d", reqLevel);
        SDL_Color reqColor = unlocked ? SDL_Color{100, 255, 100, 255} : SDL_Color{255, 100, 100, 255};
        renderTextCentered(reqText, spellX + spellSize/2, spellY + spellSize + 25, reqColor);
    }
    
    // Detailed info panel on the right
    int infoPanelX = bookX + bookWidth - 280;
    int infoPanelY = contentY;
    int infoPanelW = 250;
    int infoPanelH = contentHeight - 20;
    
    // Info panel background
    SDL_Rect infoPanel = {infoPanelX, infoPanelY, infoPanelW, infoPanelH};
    SDL_SetRenderDrawColor(renderer, 25, 20, 35, 220);
    SDL_RenderFillRect(renderer, &infoPanel);
    SDL_SetRenderDrawColor(renderer, 100, 100, 120, 255);
    SDL_RenderDrawRect(renderer, &infoPanel);
    
    // Info panel title
    renderTextCentered("SPELL DETAILS", infoPanelX + infoPanelW/2, infoPanelY + 15, {255, 215, 0, 255});
    
    // Show details for selected spell
    if (!spells.empty() && selectedSpellIndex >= 0 && selectedSpellIndex < static_cast<int>(spells.size())) {
        auto [detailSpell, detailReq, detailIcon] = spells[selectedSpellIndex];
        bool detailUnlocked = currentTab.enchantLevel >= detailReq;
        
        int detailY = infoPanelY + 45;
        
        // Spell name
        std::string detailName = spellSystem->getSpellName(detailSpell);
        renderText(detailName, infoPanelX + 10, detailY, currentTab.color);
        detailY += 25;
        
        // Description
        std::string desc = spellSystem->getSpellDescription(detailSpell);
        // Word wrap description
        std::vector<std::string> descLines;
        std::string currentLine;
        std::istringstream iss(desc);
        std::string word;
        
        while (iss >> word) {
            if (currentLine.length() + word.length() + 1 > 25) { // ~25 chars per line
                if (!currentLine.empty()) {
                    descLines.push_back(currentLine);
                    currentLine = word;
                } else {
                    descLines.push_back(word);
                }
            } else {
                if (!currentLine.empty()) currentLine += " ";
                currentLine += word;
            }
        }
        if (!currentLine.empty()) descLines.push_back(currentLine);
        
        for (const auto& line : descLines) {
            renderText(line, infoPanelX + 10, detailY, {200, 200, 200, 255});
            detailY += 18;
        }
        
        detailY += 10;
        
        if (detailUnlocked) {
            // Check if this is a passive ability
            bool isPassive = (detailSpell == SpellType::FIRE_MASTERY || 
                             detailSpell == SpellType::BURNING_AURA || 
                             detailSpell == SpellType::INFERNO_LORD);
            
            if (isPassive) {
                // Passive ability stats
                renderText("PASSIVE BONUSES:", infoPanelX + 10, detailY, {255, 200, 100, 255});
                detailY += 20;
                
                // Specific bonuses based on passive type
                if (detailSpell == SpellType::FIRE_MASTERY) {
                    renderText("• +10% Fire Damage", infoPanelX + 10, detailY, {255, 150, 150, 255});
                    detailY += 15;
                    renderText("• +5 Mana Regen/sec", infoPanelX + 10, detailY, {150, 200, 255, 255});
                    detailY += 15;
                } else if (detailSpell == SpellType::BURNING_AURA) {
                    renderText("• Aura Damage: 5/sec", infoPanelX + 10, detailY, {255, 150, 150, 255});
                    detailY += 15;
                    renderText("• Aura Radius: 100px", infoPanelX + 10, detailY, {150, 255, 150, 255});
                    detailY += 15;
                } else if (detailSpell == SpellType::INFERNO_LORD) {
                    renderText("• +25% Fire Damage", infoPanelX + 10, detailY, {255, 150, 150, 255});
                    detailY += 15;
                    renderText("• +10% Critical Hit", infoPanelX + 10, detailY, {255, 200, 100, 255});
                    detailY += 15;
                    renderText("• Fire Immunity", infoPanelX + 10, detailY, {255, 100, 100, 255});
                    detailY += 15;
                }
                
                renderText("Type: Always Active", infoPanelX + 10, detailY, {180, 180, 180, 255});
                detailY += 15;
            } else {
                // Active spell stats
                renderText("COMBAT STATS:", infoPanelX + 10, detailY, {255, 200, 100, 255});
                detailY += 20;
                
                // Calculate damage based on enchantment level
                int baseDamage = 10; // Base damage for spells
                int enchantBonus = currentTab.enchantLevel * 2; // +2 damage per enchant level
                int totalDamage = baseDamage + enchantBonus;
                
                char damageText[64];
                snprintf(damageText, sizeof(damageText), "Damage: %d (%d + %d)", totalDamage, baseDamage, enchantBonus);
                renderText(damageText, infoPanelX + 10, detailY, {255, 150, 150, 255});
                detailY += 18;
                
                // Mana cost
                int manaCost = spellSystem->getSpellManaCost(detailSpell);
                char manaText[32];
                snprintf(manaText, sizeof(manaText), "Mana Cost: %d", manaCost);
                renderText(manaText, infoPanelX + 10, detailY, {150, 200, 255, 255});
                detailY += 18;
                
                // Cooldown
                char cooldownText[32];
                snprintf(cooldownText, sizeof(cooldownText), "Cooldown: %.1fs", spellSystem->getCooldownRemaining(detailSpell));
                renderText(cooldownText, infoPanelX + 10, detailY, {200, 200, 150, 255});
                detailY += 18;
                
                // Range/Area
                renderText("Range: Medium", infoPanelX + 10, detailY, {150, 255, 150, 255});
                detailY += 18;
                
                renderText("Type: Castable", infoPanelX + 10, detailY, {180, 180, 180, 255});
                detailY += 15;
            }
        } else {
            renderText("LOCKED", infoPanelX + 10, detailY, {255, 100, 100, 255});
            detailY += 20;
            
            char unlockText[64];
            snprintf(unlockText, sizeof(unlockText), "Requires %s +%d enchantment", currentTab.name, detailReq);
            
            // Word wrap unlock text
            std::vector<std::string> unlockLines;
            std::string currentUnlockLine;
            std::istringstream unlockIss(unlockText);
            std::string unlockWord;
            
            while (unlockIss >> unlockWord) {
                if (currentUnlockLine.length() + unlockWord.length() + 1 > 25) {
                    if (!currentUnlockLine.empty()) {
                        unlockLines.push_back(currentUnlockLine);
                        currentUnlockLine = unlockWord;
                    } else {
                        unlockLines.push_back(unlockWord);
                    }
                } else {
                    if (!currentUnlockLine.empty()) currentUnlockLine += " ";
                    currentUnlockLine += unlockWord;
                }
            }
            if (!currentUnlockLine.empty()) unlockLines.push_back(currentUnlockLine);
            
            for (const auto& line : unlockLines) {
                renderText(line, infoPanelX + 10, detailY, {200, 150, 150, 255});
                detailY += 15;
            }
        }
    }
    
    // Controls hint at bottom
    renderTextCentered("Press B to close", bookX + bookWidth/2, bookY + bookHeight - 20, {180, 180, 180, 255});
}
