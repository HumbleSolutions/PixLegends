## PixLegends

A 2D action-adventure prototype built with SDL2. It features chunk-based procedural world generation, a combat-ready player character, a boss enemy, interactable objects with loot, a simple HUD, options overlay, audio with mixer fallback, and an autotile visualization demo.

### Current status
- Procedural world with biomes, large coherent regions, rivers/lakes, and fog of war (toggle always-on; see `World`)
- Chunk streaming and screen-space culling for tiles, objects, and enemies
- Player: WASD movement, melee and ranged attacks, potions (HP/MP), leveling, stats, gold
- Enemy: Demon boss with idle/fly/attack/hurt/death states and contact damage, on-screen boss health bar when engaged
- Interactables: chests/pots/crates/signs/bonfire with simple loot and prompts; respawn on death with UI popup
- UI/HUD: health/mana/xp bars, gold, debug info, FPS counter (color-coded), death popup, options overlay
- Assets: pixel-art characters/tiles/objects and SFX/music included under `assets/`

### Controls
- Movement: W/A/S/D
- Melee attack: Space or Left Mouse
- Ranged attack: Left Shift or Right Mouse
- Interact: E
- Use potions: 1 (HP), 2 (MP)
- Pause: P
- Options overlay: Esc
- Toggle autotile demo: F1 (replaces world render with demo grid)
- Toggle debug hitboxes: F3

### Build and run

Prerequisites
- CMake 3.16+
- A C++17 compiler
- SDL2, SDL2_image, SDL2_ttf (SDL2_mixer optional but recommended for OGG music)

Windows (Visual Studio 2022)
```bat
build.bat
```
This generates `build/` with a Visual Studio solution and builds `Release\PixLegends.exe`. Required DLLs (SDL2, image, ttf, and mixer if present) are copied next to the executable. Assets are copied into `Release\assets` automatically.

Linux/macOS
```bash
./build.sh
./build/PixLegends
```

Manual CMake
```bash
mkdir -p build
cd build
cmake ..
cmake --build . --config Release
```

Dependency setup notes
- The project will first try `find_package(SDL2/SDL2_image/SDL2_ttf/SDL2_mixer)`. If not found, it falls back to manual detection, checking common paths and the bundled `external/` folder.
- On Windows you can run `setup_sdl2.bat` to download and populate `external/` with headers, libs, and DLLs.

### Persistent data (accounts and saves)
- A lightweight file-backed database is included to manage users and player saves under the `data/` directory.
- On first run it creates CSV files and two default users:
  - `admin` / `admin` (role: ADMIN)
  - `player` / `player` (role: PLAYER)
- The game auto-loads the `player` account and periodically autosaves the player state.

Managing accounts
- Use the `Database` class to register/authenticate users in a future UI, or edit the CSVs in `data/` directly during development.

### Audio
- With SDL_mixer: OGG music and WAV SFX are supported with proper volume/ducking and fade transitions.
- Without SDL_mixer: falls back to raw SDL device playback for WAV. OGG requires SDL_mixer.

### Performance features
- Fixed-timestep update with frame capping to target 60 FPS
- Chunked world generation with render-distance streaming
- Viewport/object/enemy culling
- Texture caching, sprite-sheet loading, RGBA8888 conversion, blend modes
- In-game FPS counter with average and frame time

### Project layout highlights
- Source: `src/` and headers `include/`
- Assets: `assets/` (sprites, tiles, objects, audio, fonts)
- External libraries: `external/` (pre-populated or filled by `setup_sdl2.bat`)
- Build helpers: `build.bat`, `build.sh`

### Roadmap
- More enemy types and encounters
- Object pooling for projectiles and effects
- Texture atlasing; additional biome art; lighting pass
- Save/load and broader gameplay loop
  - Expand the file-backed DB or switch to SQLite for richer queries and concurrency

### License
Code is under MIT (see `LICENSE`). Third‑party assets in `assets/` retain their original licenses; review before redistribution.
