# PixLegends - 2D Action Adventure Game

A 2D action-adventure game built with SDL2, featuring procedural world generation, dynamic lighting, and interactive gameplay.

## Recent Performance Optimizations (Latest Update)

### 🎯 Key Improvements Implemented

#### 1. **FPS Counter & Performance Monitoring**
- ✅ Added real-time FPS counter displaying current FPS, average FPS, and frame time
- ✅ Color-coded performance indicators (Green: ≥55 FPS, Yellow: ≥30 FPS, Red: <30 FPS)
- ✅ Performance metrics tracking with 1-second history buffer
- ✅ Positioned in top-right corner for easy monitoring

#### 2. **Procedural Generation Optimization**
- ✅ **Reduced object count** from unlimited to maximum 8 objects
- ✅ **Intelligent spawning system** with controlled positioning
- ✅ **Spawn radius limitation** (15 tiles around player starting position)
- ✅ **Performance-focused object placement** to reduce lag
- ✅ **Optimized object types** - only spawn essential objects

#### 3. **Texture Loading Fixes**
- ✅ **Fixed black squares issue** by implementing proper RGBA8888 format conversion
- ✅ **Improved texture caching** with better error handling
- ✅ **Consistent blend mode** application for all textures
- ✅ **Reduced texture loading overhead** with batch processing
- ✅ **Memory optimization** through proper surface cleanup

#### 4. **Rendering Performance Optimizations**
- ✅ **Viewport culling** - only render tiles visible on screen
- ✅ **Camera-based rendering** with margin for smooth scrolling
- ✅ **Reduced debug output** (300 frames instead of 60)
- ✅ **Object culling** - only render objects in visible area
- ✅ **Optimized tile rendering** with proper texture management

#### 5. **Memory and CPU Optimizations**
- ✅ **Reduced object updates** - only update visible objects
- ✅ **Efficient data structures** for performance monitoring
- ✅ **Optimized asset loading** with batch processing
- ✅ **Reduced console spam** for better performance
- ✅ **Smart caching** for frequently accessed assets

### 🚀 Performance Impact

- **FPS Improvement**: Significant boost in frame rates, especially on lower-end systems
- **Memory Usage**: Reduced memory footprint through optimized object spawning
- **Loading Times**: Faster asset loading with batch processing
- **Smooth Gameplay**: Eliminated lag caused by excessive object spawning
- **Visual Quality**: Fixed texture rendering issues for better visual experience

### 🎮 Gameplay Features

- **Procedural World Generation**: Dynamic tile-based world with multiple biomes
- **Fog of War**: Dynamic visibility system with exploration tracking
- **Interactive Objects**: Chests, pots, crates with loot system
- **Player Progression**: Health, mana, experience, and gold systems
- **Combat System**: Melee and ranged combat with projectiles
- **UI System**: Comprehensive HUD with stats, debug info, and FPS counter

### 🔧 Technical Details

#### Performance Monitoring
```cpp
// FPS counter implementation
void UISystem::renderFPSCounter(float currentFPS, float averageFPS, Uint32 frameTime) {
    // Color-coded performance display
    // Green: ≥55 FPS, Yellow: ≥30 FPS, Red: <30 FPS
}
```

#### Optimized Object Spawning
```cpp
// Reduced from unlimited to maximum 8 objects
const int maxObjects = 8;
const int spawnRadius = 15; // Tiles around player starting position
```

#### Texture Loading Fix
```cpp
// Proper RGBA8888 format conversion
SDL_PixelFormat* format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888);
convertedSurface = SDL_ConvertSurface(surface, format, 0);
SDL_SetTextureBlendMode(sdlTexture, SDL_BLENDMODE_BLEND);
```

#### Viewport Culling
```cpp
// Only render visible tiles
int startX = std::max(0, cameraX / tileSize - 2);
int endX = std::min(width - 1, (cameraX + 1920) / tileSize + 2);
```

### 📊 Performance Metrics

- **Target FPS**: 60 FPS
- **Object Limit**: 8 maximum objects
- **Render Optimization**: Viewport culling with 2-tile margin
- **Memory Usage**: Optimized through efficient caching
- **Loading Time**: Reduced through batch processing

### 🐛 Bug Fixes

1. **Texture Rendering**: Fixed black squares and incorrect texture display
2. **Object Spawning**: Eliminated excessive object creation causing lag
3. **Performance Monitoring**: Added comprehensive FPS tracking
4. **Memory Leaks**: Fixed through proper resource management
5. **Rendering Issues**: Optimized for smooth 60 FPS gameplay

### 🎯 Future Optimizations

- [ ] Implement object pooling for dynamic objects
- [ ] Add LOD (Level of Detail) system for distant objects
- [ ] Optimize pathfinding algorithms
- [ ] Implement texture atlasing for better memory usage
- [ ] Add multi-threading for asset loading

## Building and Running

### Prerequisites
- SDL2
- SDL2_image
- SDL2_ttf
- CMake 3.10+

### Build Instructions
```bash
mkdir build
cd build
cmake ..
make
```

### Running the Game
```bash
./PixLegends
```

## Controls

- **WASD**: Move player
- **Mouse**: Aim and shoot
- **E**: Interact with objects
- **ESC**: Exit game
- **P**: Pause/unpause

## License

This project is licensed under the MIT License - see the LICENSE file for details.
