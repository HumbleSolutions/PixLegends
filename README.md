## PixLegends

A 2D action-adventure game built with SDL2 featuring advanced equipment systems, elemental enchantments, and dynamic combat mechanics. Experience a rich fantasy world with dual equipment systems, magic anvil upgrading, and fire-based combat abilities.

### 🌟 Key Features

**Advanced Equipment & Enchantment System**
- **Dual Equipment Systems**: Legacy + modern ItemSystem with full synchronization
- **Item Rarity**: 5-tier system (Common→Uncommon→Rare→Epic→Legendary) with color coding
- **Equipment Upgrading**: +0 to +30 enhancement via Magic Anvil with success/failure mechanics
- **Elemental Enchantments**: Fire, Water, and Poison damage/resistance via enchantment scrolls
- **Unique Item Instances**: Each item has unique ID preventing upgrade contamination

**Dynamic Combat & Abilities**
- **Fire Damage System**: Weapons add fire damage, armor provides resistance
- **Fire Shield Ability**: AoE damage shield scaling with waist enchantments
- **Dash Ability**: Directional dash with invincibility frames and cooldown
- **Enhanced Projectiles**: Fire projectiles and arrows with enchanted damage scaling

**Rich UI & Interface**
- **Dual Inventory System**: Equipment/Materials (6×8) + Scrolls/Enchantments (4×5)
- **Magic Anvil Interface**: Visual upgrading with success/failure feedback
- **Advanced Tooltips**: Item stats, hover detection, and rarity display
- **Equipment Visualization**: Visual slots showing +levels and rarity colors

### 🎮 Current Implementation Status

**✅ Fully Implemented**
- Complete player character with movement, combat, and abilities
- Goblin enemies with AI states (patrol, attack, hurt, death)
- Dynamic loot generation with rarity-based drops
- Persistent save system with user accounts and settings
- Audio system with themes and volume controls
- Procedural world generation with biomes and fog of war

**🚧 Partially Implemented**  
- Enemy variety (25+ creature sprites available, only Goblins active)
- Terrain diversity (extensive tile sets available, basic implementation)
- Elemental system (Fire/Water/Poison active, Lightning planned)

### 🎯 Controls

**Movement & Combat**
- Movement: W/A/S/D
- Melee attack: Space or Left Mouse (direction-based)
- Ranged attack: Shift or Right Mouse (mouse targeting)
- Dash: Movement direction during dash
- Fire Shield: Space (when belt has fire enchantment)

**Interaction & UI**
- Interact: E (objects, magic anvil)
- Inventory: I (dual-pane equipment and scrolls)
- Equipment: U (visual equipment slots)
- Potions: 1 (HP), 2 (MP)
- Pause: P | Options: Esc
- Debug: F1 (autotile demo), F3 (hitboxes)

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

### 🗺️ Roadmap

**Priority 1: Content Expansion**
- **Enemy Diversity**: Activate 25+ existing creature sprites (Dragons, Demons, etc.)
- **Boss Battles**: Multi-phase fights with arena mechanics
- **Lightning Enchantments**: Complete the electrical damage system

**Priority 2: World Building**
- **Biome Expansion**: Utilize extensive terrain assets (snow, desert, forest)
- **Underworld Content**: Hell-themed dungeons with giant boss encounters
- **NPC System**: Quests and merchants using existing NPC sprites

**Priority 3: Advanced Features**
- **Magic Schools**: Full spell system with elemental combinations
- **Social Systems**: Guilds, trading, and leaderboards
- **Performance**: Mobile optimization and cross-platform saves

**Technical Improvements**
- Entity Component System (ECS) migration
- Data-driven configuration (JSON)
- Mod support and community tools

### License
Code is under MIT (see `LICENSE`). Third‑party assets in `assets/` retain their original licenses; review before redistribution.
