#include "Renderer.h"
#include <iostream>
#include <cmath>

Renderer::Renderer(SDL_Renderer* sdlRenderer) : renderer(sdlRenderer), cameraX(0), cameraY(0) {
    if (!renderer) {
        throw std::runtime_error("Renderer requires a valid SDL_Renderer");
    }
}

void Renderer::renderTexture(SDL_Texture* texture, const SDL_Rect* srcRect, const SDL_Rect* dstRect) {
    if (!texture) {
        return;
    }
    
    SDL_Rect adjustedDstRect;
    if (dstRect) {
        adjustedDstRect = *dstRect;
        // Apply camera offset
        adjustedDstRect.x -= cameraX;
        adjustedDstRect.y -= cameraY;
        // Apply zoom scaling around top-left for simplicity
        if (std::fabs(zoom - 1.0f) > 0.001f) {
            adjustedDstRect.x = static_cast<int>(adjustedDstRect.x * zoom);
            adjustedDstRect.y = static_cast<int>(adjustedDstRect.y * zoom);
            adjustedDstRect.w = static_cast<int>(adjustedDstRect.w * zoom);
            adjustedDstRect.h = static_cast<int>(adjustedDstRect.h * zoom);
        }
    }
    
    SDL_RenderCopy(renderer, texture, srcRect, dstRect ? &adjustedDstRect : nullptr);
}

void Renderer::renderTexture(SDL_Texture* texture, int x, int y, int width, int height) {
    if (!texture) {
        return;
    }
    
    SDL_Rect dstRect = {x - cameraX, y - cameraY, width, height};
    if (std::fabs(zoom - 1.0f) > 0.001f) {
        dstRect.x = static_cast<int>(dstRect.x * zoom);
        dstRect.y = static_cast<int>(dstRect.y * zoom);
        dstRect.w = static_cast<int>(dstRect.w * zoom);
        dstRect.h = static_cast<int>(dstRect.h * zoom);
    }
    SDL_RenderCopy(renderer, texture, nullptr, &dstRect);
}

void Renderer::renderTextureEx(SDL_Texture* texture, const SDL_Rect* srcRect, const SDL_Rect* dstRect, 
                               double angle, const SDL_Point* center, SDL_RendererFlip flip) {
    if (!texture) {
        return;
    }
    
    SDL_Rect adjustedDstRect;
    if (dstRect) {
        adjustedDstRect = *dstRect;
        // Apply camera offset
        adjustedDstRect.x -= cameraX;
        adjustedDstRect.y -= cameraY;
        // Apply zoom scaling
        if (std::fabs(zoom - 1.0f) > 0.001f) {
            adjustedDstRect.x = static_cast<int>(adjustedDstRect.x * zoom);
            adjustedDstRect.y = static_cast<int>(adjustedDstRect.y * zoom);
            adjustedDstRect.w = static_cast<int>(adjustedDstRect.w * zoom);
            adjustedDstRect.h = static_cast<int>(adjustedDstRect.h * zoom);
        }
    }
    
    SDL_RenderCopyEx(renderer, texture, srcRect, dstRect ? &adjustedDstRect : nullptr, 
                     angle, center, flip);
}

void Renderer::renderTextureFlipped(SDL_Texture* texture, int x, int y, int width, int height, 
                                    bool flipHorizontal, bool flipVertical) {
    if (!texture) {
        return;
    }
    
    SDL_Rect dstRect = {x - cameraX, y - cameraY, width, height};
    if (std::fabs(zoom - 1.0f) > 0.001f) {
        dstRect.x = static_cast<int>(dstRect.x * zoom);
        dstRect.y = static_cast<int>(dstRect.y * zoom);
        dstRect.w = static_cast<int>(dstRect.w * zoom);
        dstRect.h = static_cast<int>(dstRect.h * zoom);
    }
    
    SDL_RendererFlip flip = SDL_FLIP_NONE;
    if (flipHorizontal && flipVertical) {
        flip = static_cast<SDL_RendererFlip>(SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL);
    } else if (flipHorizontal) {
        flip = SDL_FLIP_HORIZONTAL;
    } else if (flipVertical) {
        flip = SDL_FLIP_VERTICAL;
    }
    
    SDL_RenderCopyEx(renderer, texture, nullptr, &dstRect, 0.0, nullptr, flip);
}

void Renderer::renderText(const std::string& text, TTF_Font* font, int x, int y, SDL_Color color) {
    if (!font || text.empty()) {
        return;
    }
    
    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
    if (!surface) {
        std::cerr << "Failed to render text: " << TTF_GetError() << std::endl;
        return;
    }
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    
    if (!texture) {
        std::cerr << "Failed to create texture from text surface: " << SDL_GetError() << std::endl;
        return;
    }
    
    int width, height;
    SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);
    
    SDL_Rect dstRect = {x - cameraX, y - cameraY, width, height};
    SDL_RenderCopy(renderer, texture, nullptr, &dstRect);
    
    SDL_DestroyTexture(texture);
}

void Renderer::renderTextCentered(const std::string& text, TTF_Font* font, int x, int y, SDL_Color color) {
    if (!font || text.empty()) {
        return;
    }
    
    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
    if (!surface) {
        std::cerr << "Failed to render text: " << TTF_GetError() << std::endl;
        return;
    }
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    
    if (!texture) {
        std::cerr << "Failed to create texture from text surface: " << SDL_GetError() << std::endl;
        return;
    }
    
    int width, height;
    SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);
    
    SDL_Rect dstRect = {x - width/2 - cameraX, y - height/2 - cameraY, width, height};
    SDL_RenderCopy(renderer, texture, nullptr, &dstRect);
    
    SDL_DestroyTexture(texture);
}

void Renderer::renderRect(const SDL_Rect& rect, SDL_Color color, bool filled) {
    renderRect(rect.x, rect.y, rect.w, rect.h, color, filled);
}

void Renderer::renderRect(int x, int y, int width, int height, SDL_Color color, bool filled) {
    setDrawColor(color);
    
    SDL_Rect rect = {x - cameraX, y - cameraY, width, height};
    
    if (filled) {
        SDL_RenderFillRect(renderer, &rect);
    } else {
        SDL_RenderDrawRect(renderer, &rect);
    }
}

void Renderer::renderLine(int x1, int y1, int x2, int y2, SDL_Color color) {
    setDrawColor(color);
    SDL_RenderDrawLine(renderer, x1 - cameraX, y1 - cameraY, x2 - cameraX, y2 - cameraY);
}

void Renderer::renderCircle(int centerX, int centerY, int radius, SDL_Color color, bool filled) {
    setDrawColor(color);
    
    int x = radius;
    int y = 0;
    int err = 0;
    
    while (x >= y) {
        drawCirclePoints(centerX - cameraX, centerY - cameraY, x, y, color, filled);
        
        if (err <= 0) {
            y += 1;
            err += 2*y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2*x + 1;
        }
    }
}

void Renderer::drawCirclePoints(int centerX, int centerY, int x, int y, SDL_Color color, bool filled) {
    if (filled) {
        SDL_RenderDrawLine(renderer, centerX - x, centerY + y, centerX + x, centerY + y);
        SDL_RenderDrawLine(renderer, centerX - x, centerY - y, centerX + x, centerY - y);
        SDL_RenderDrawLine(renderer, centerX - y, centerY + x, centerX + y, centerY + x);
        SDL_RenderDrawLine(renderer, centerX - y, centerY - x, centerX + y, centerY - x);
    } else {
        SDL_RenderDrawPoint(renderer, centerX + x, centerY + y);
        SDL_RenderDrawPoint(renderer, centerX - x, centerY + y);
        SDL_RenderDrawPoint(renderer, centerX + x, centerY - y);
        SDL_RenderDrawPoint(renderer, centerX - x, centerY - y);
        SDL_RenderDrawPoint(renderer, centerX + y, centerY + x);
        SDL_RenderDrawPoint(renderer, centerX - y, centerY + x);
        SDL_RenderDrawPoint(renderer, centerX + y, centerY - x);
        SDL_RenderDrawPoint(renderer, centerX - y, centerY - x);
    }
}

void Renderer::renderProgressBar(int x, int y, int width, int height, float progress, 
                                SDL_Color backgroundColor, SDL_Color foregroundColor) {
    // Clamp progress to 0-1
    progress = std::max(0.0f, std::min(1.0f, progress));
    
    // Draw background
    renderRect(x, y, width, height, backgroundColor, true);
    
    // Draw foreground (progress)
    int progressWidth = static_cast<int>(width * progress);
    if (progressWidth > 0) {
        renderRect(x, y, progressWidth, height, foregroundColor, true);
    }
    
    // Draw border
    renderRect(x, y, width, height, {255, 255, 255, 255}, false);
}

void Renderer::setDrawColor(SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

void Renderer::setDrawColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
}

void Renderer::clear() {
    SDL_RenderClear(renderer);
}

void Renderer::present() {
    SDL_RenderPresent(renderer);
}

void Renderer::setCamera(int x, int y) {
    cameraX = x;
    cameraY = y;
}

void Renderer::getCamera(int& x, int& y) const {
    x = cameraX;
    y = cameraY;
}

void Renderer::worldToScreen(int worldX, int worldY, int& screenX, int& screenY) const {
    screenX = worldX - cameraX;
    screenY = worldY - cameraY;
}

void Renderer::screenToWorld(int screenX, int screenY, int& worldX, int& worldY) const {
    // Inverse of worldToScreen, account for zoom and camera offset
    float invZoom = (std::fabs(zoom) < 0.0001f) ? 1.0f : (1.0f / zoom);
    worldX = static_cast<int>(std::floor(static_cast<float>(screenX) * invZoom)) + cameraX;
    worldY = static_cast<int>(std::floor(static_cast<float>(screenY) * invZoom)) + cameraY;
}
