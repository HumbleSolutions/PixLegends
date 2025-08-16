# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

**Windows:**
```
build.bat
```
Generates Visual Studio 2022 solution in `build/` and builds `Release\PixLegends.exe`. DLLs and assets are automatically copied.

**Linux/macOS:**
```
./build.sh
./build/PixLegends
```

**Manual CMake:**
```
mkdir -p build && cd build
cmake ..
cmake --build . --config Release
```

## Project Architecture

PixLegends is a 2D action-adventure game built with SDL2 using C++17. The architecture follows a hybrid component-based design with dual equipment systems and advanced enchantment mechanics:

### Core Systems (src/ and include/)
- **Game**: Main game loop, system coordination, timing, and state management with modal UI control
- **Renderer**: SDL2-based rendering system with texture caching, sprite management, and viewport culling
- **AssetManager**: Resource loading and management for sprites, audio, fonts, and sprite sheets
- **InputManager**: Event handling and input state management with mouse/keyboard support
- **AudioManager**: Audio playbook with SDL_mixer support (OGG music, WAV SFX) and volume controls
- **UISystem**: Advanced HUD rendering, options overlay, dual inventory system, and magic anvil interface
- **Database**: File-backed CSV database for user accounts, save data, audio settings, and theme persistence
- **ItemSystem**: New item management system with rarity, enchantments, and unique instance tracking

### Game Entities
- **Player**: Character with movement, combat (melee/ranged), dual equipment systems, leveling, and dash ability
- **Enemy**: AI-driven entities with state machines (idle/patrol/attack/hurt/death) - currently Goblins implemented
- **World**: Chunk-based procedural generation with biomes, objects, fog of war, and Underworld areas
- **Object**: Interactive items (chests, pots, signs, bonfire, magic anvil) with advanced loot tables
- **Projectile**: Arrow and fire spell projectiles with collision detection and enchanted damage
- **LootGenerator**: Dynamic loot generation with rarity-based drops and equipment scaling

### Advanced Equipment & Enchantment System
- **Dual Equipment Systems**: Legacy Player equipment array + new ItemSystem with full compatibility
- **Item Rarity System**: Common (white) → Uncommon (green) → Rare (blue) → Epic (purple) → Legendary (orange)
- **Equipment Upgrading**: +0 to +30 enhancement levels with success/failure chances via Magic Anvil
- **Elemental Enchantments**: Fire, Water, and Poison damage/resistance via enchantment scrolls
- **Dynamic Item Names**: Sword names change based on enhancement level (+26 = "Dragonbone Sword")
- **Unique Item Instances**: Each item has unique ID to prevent upgrade contamination
- **Fire Damage System**: Weapons add fire damage to attacks, armor provides fire resistance

### Combat & Abilities
- **Melee Combat**: Sword attacks with crit chance, attack speed, and durability mechanics
- **Ranged Combat**: Bow with arrows, fire projectiles from enchanted weapons
- **Fire Shield Ability**: Space-activated shield with AoE damage scaling with waist enchantments
- **Dash Ability**: Directional dash with I-frames and cooldown system
- **Damage Calculation**: Separates physical and elemental damage with resistance application

### Key Features
- Fixed 60 FPS timestep with frame accumulation
- Chunk streaming and viewport culling for performance
- Modal UI system (options, inventory, magic anvil) with proper state management
- Persistent user accounts and save system via CSV files in `data/`
- Audio settings and theme persistence per user account
- Advanced tooltip system with item stats and hover detection
- Debug tools (F1: autotile demo, F3: hitbox display)
- Remember login functionality for development convenience

### UI Systems
- **HUD**: Health/Mana bars, experience bar, potion counters, minimap
- **Inventory**: Dual-pane system - Equipment/Materials (6×8) + Scrolls/Enchantments (4×5)
- **Equipment UI**: Visual equipment slots with +level display and rarity colors
- **Magic Anvil**: Item upgrading and enchanting interface with success/failure feedback
- **Options Menu**: Audio controls, theme selection, account management

### Asset Organization
- `assets/`: Organized by category with selective inclusion (unused assets gitignored)
- **Used Assets**: Main Character, Goblin, Items, UI, Sounds, Fonts, Core Textures
- **Excluded Assets**: 25+ unused creature types, extra tile variations, mockups
- Automatic asset copying to build output directory
- Support for PNG sprites, WAV/OGG audio, and TTF fonts

### Development Tools
- `tools/`: PowerShell scripts for TMX map generation and GIF frame extraction
- `external/`: SDL2 library dependencies and setup scripts
- Comprehensive CMake configuration with Windows DLL handling
- **Updated .gitignore**: Excludes unused assets, user data, and development files

### Dependencies
- SDL2, SDL2_image, SDL2_ttf (required)
- SDL2_mixer (optional, enables OGG music support)
- CMake 3.16+ and C++17 compiler

### Default Controls
- **Movement**: WASD
- **Combat**: LMB (melee), Shift/RMB (ranged)
- **Abilities**: Space (dash), F (fire shield when belt has fire enchantment)
- **Spells**: 3-6 or Q,E,R,T (cast elemental spells based on weapon enchantment)
- **Interact**: E (objects, NPCs)
- **Potions**: 1 (HP), 2 (MP)
- **UI**: I (inventory), U (equipment), B (spell book), P (pause), Esc (options), F1/F3 (debug)
- **Mouse**: Left-click (melee direction), Right-click (ranged targeting), Hover (tooltips)

## Current Implementation Status

### ✅ Implemented Features
- Complete player character system with 4-directional movement and combat
- Dual equipment system (legacy + ItemSystem) with full synchronization
- Advanced enchantment system with fire/water/poison elements
- Magic anvil for upgrades (+0 to +30) and enchantments
- Item rarity system with 5 tiers and visual color coding
- Inventory management with dual-pane UI (equipment + scrolls)
- Fire damage system for weapons and fire resistance for armor
- Fire shield ability scaling with belt enchantments
- Persistent save system with user accounts and audio/theme settings
- Goblin enemy AI with patrol, attack, and death states
- Dynamic loot generation with rarity-based drops
- Advanced UI with tooltips, hover detection, and proper layout
- Audio system with music themes and sound effects

### 🚧 Partially Implemented
- **Enemy System**: Only Goblins implemented, 25+ creature sprites available but unused
- **Terrain System**: Basic grass/water/lava, many tile types available but unused
- **Enchantment System**: Fire/Water/Poison implemented, Lightning planned but not active
- **Underworld Areas**: Basic platform tiles, advanced assets available but unused

### ❌ Not Yet Implemented
- Boss battles and special encounters
- Magic/spell system beyond fire projectiles
- Multiplayer or co-op functionality
- Advanced AI behaviors (formations, tactics)
- Environmental effects and weather
- Achievement and progression systems

## Future Feature Recommendations

### Priority 1: Core Gameplay Expansion
1. **Enemy Diversity Implementation**
   - Activate existing creature sprites (Dragons, Demons, Centaurs, etc.)
   - Implement unique AI behaviors per creature type
   - Add creature-specific loot tables and XP rewards
   - Create encounter escalation system

2. **Boss Battle System**
   - Implement multi-phase boss fights using existing Demon Boss sprites
   - Create arena-style boss rooms with environmental hazards
   - Add unique boss loot (legendary items, rare enchantments)
   - Boss health bars and combat state management

3. **Enhanced Combat Mechanics**
   - **Lightning Enchantments**: Complete the electrical damage system
   - **Combo System**: Chain attacks for increased damage
   - **Status Effects**: Burning (fire), Frozen (water), Poisoned (poison), Stunned (lightning)
   - **Critical Hit Visuals**: Screen shake and visual effects for crits

### Priority 2: Content & World Building
4. **Advanced Terrain System**
   - Activate unused tile types (snow, desert, forest biomes)
   - Environmental hazards and interactive terrain
   - Weather effects and day/night cycles
   - Biome-specific enemy spawns and loot

5. **Underworld Expansion**
   - Implement advanced Underworld assets (giant boss, props, animations)
   - Create hell-themed dungeons with unique mechanics
   - Lava mechanics and heat-based environmental challenges
   - Underworld-exclusive enemies and rewards

6. **NPC & Quest System**
   - Implement NPC Pack sprites (Alchemist, Blacksmith, Enchanter)
   - Quest journal and objective tracking
   - Merchant systems for buying/selling equipment
   - Story progression and narrative elements

### Priority 3: Advanced Systems
7. **Magic & Spell System**
   - **Active Magic Schools**: Fire, Water, Earth, Lightning spells
   - **Mana Management**: Spell costs and cooldowns
   - **Spell Combinations**: Elemental reactions and combos
   - **Magic Progression**: Spell levels and mastery systems

8. **Advanced Enchantment Features**
   - **Set Bonuses**: Equipment sets with special effects
   - **Legendary Effects**: Unique properties for legendary items
   - **Enchantment Fusion**: Combine multiple enchantments
   - **Cursed Items**: High-power items with drawbacks

9. **Social Features**
   - **Guilds**: Player organizations with shared benefits
   - **Trading System**: Player-to-player item exchange
   - **Leaderboards**: Equipment levels, boss clear times
   - **Collaborative Dungeons**: Multi-player content

### Priority 4: Polish & Accessibility
10. **Enhanced UI/UX**
    - **Keybind Customization**: Remappable controls
    - **Accessibility Options**: Colorblind support, font scaling
    - **Advanced Settings**: Graphics quality, performance options
    - **Tutorial System**: New player onboarding

11. **Performance & Optimization**
    - **Asset Streaming**: Load assets on-demand
    - **Memory Management**: Optimize sprite caching
    - **Mobile Port**: Touch controls and mobile UI adaptation
    - **Cross-Platform Saves**: Cloud save synchronization

## Technical Design Considerations

### Architecture Recommendations
- **Entity Component System (ECS)**: Consider migration for better performance and modularity
- **State Machine Framework**: Formalize AI and game state management
- **Event System**: Decouple systems with event-driven architecture
- **Mod Support**: Plugin architecture for community content

### Code Quality Improvements
- **Unit Testing**: Implement testing framework for core systems
- **Performance Profiling**: Add performance monitoring and optimization
- **Error Handling**: Comprehensive error recovery and logging
- **Documentation**: API documentation and code comments

### Data-Driven Design
- **JSON Configuration**: Move hard-coded values to config files
- **Scriptable AI**: Lua or similar for AI behavior scripting
- **Dynamic Loading**: Runtime loading of creatures, items, and content
- **Editor Tools**: In-game level editor and content creation tools

## Asset Utilization Strategy
With 25+ unused creature types and extensive terrain assets, prioritize:
1. **Creature Implementation Pipeline**: Systematic activation of existing sprites
2. **Biome Diversification**: Utilize terrain variety for world depth
3. **Asset Validation**: Ensure all activated assets meet quality standards
4. **Performance Testing**: Monitor impact of asset activation on performance

This roadmap leverages existing assets while building upon the solid foundation of equipment, enchantment, and combat systems already implemented.