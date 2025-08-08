#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>

class Renderer {
public:
    explicit Renderer(SDL_Renderer* sdlRenderer);
    ~Renderer() = default;

    // Texture rendering
    void renderTexture(SDL_Texture* texture, const SDL_Rect* srcRect = nullptr, const SDL_Rect* dstRect = nullptr);
    void renderTexture(SDL_Texture* texture, int x, int y, int width = 0, int height = 0);
    
    // Text rendering
    void renderText(const std::string& text, TTF_Font* font, int x, int y, SDL_Color color = {255, 255, 255, 255});
    void renderTextCentered(const std::string& text, TTF_Font* font, int x, int y, SDL_Color color = {255, 255, 255, 255});
    
    // Shape rendering
    void renderRect(const SDL_Rect& rect, SDL_Color color = {255, 255, 255, 255}, bool filled = true);
    void renderRect(int x, int y, int width, int height, SDL_Color color = {255, 255, 255, 255}, bool filled = true);
    void renderLine(int x1, int y1, int x2, int y2, SDL_Color color = {255, 255, 255, 255});
    void renderCircle(int centerX, int centerY, int radius, SDL_Color color = {255, 255, 255, 255}, bool filled = true);
    
    // UI rendering
    void renderProgressBar(int x, int y, int width, int height, float progress, 
                          SDL_Color backgroundColor = {50, 50, 50, 255},
                          SDL_Color foregroundColor = {0, 255, 0, 255});
    
    // Utility functions
    void setDrawColor(SDL_Color color);
    void setDrawColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255);
    void clear();
    void present();
    
    // Camera/viewport
    void setCamera(int x, int y);
    void getCamera(int& x, int& y) const;
    void worldToScreen(int worldX, int worldY, int& screenX, int& screenY) const;
    void screenToWorld(int screenX, int screenY, int& worldX, int& worldY) const;
    
    // Access to SDL renderer
    SDL_Renderer* getSDLRenderer() const { return renderer; }

private:
    SDL_Renderer* renderer;
    int cameraX, cameraY;
    
    // Helper functions
    void drawCirclePoints(int centerX, int centerY, int x, int y, SDL_Color color, bool filled);
};
