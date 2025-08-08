# PixLegends

[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/std/the-standard)
[![CMake](https://img.shields.io/badge/CMake-3.16+-green.svg)](https://cmake.org/)
[![SDL2](https://img.shields.io/badge/SDL2-2.28.5+-orange.svg)](https://www.libsdl.org/)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)](https://github.com/yourusername/PixLegends)

A 2D pixel art action-adventure game built with C++ and SDL2, featuring a wizard character with melee and ranged combat abilities.

![PixLegends Screenshot](assets/Wizard%202D%20Pixel%20Art%20v2.0/Sprites/without_outline/IDLE.png)

## Features

- **Wizard Character**: Animated player character with idle, walk, attack, and hurt animations
- **Combat System**: Melee attacks (staff) and ranged magic projectiles
- **Leveling System**: Experience points, level progression, and stat scaling
- **Tile-based World**: Grid-based environment with collision detection
- **Modern C++**: Built with C++17 and modern programming practices
- **Modular Architecture**: Clean separation of concerns with dedicated systems

## Controls

| Action | Key |
|--------|-----|
| **Movement** | WASD or Arrow Keys |
| **Melee Attack** | Spacebar or Left Mouse Button |
| **Ranged Attack** | Shift or Right Mouse Button |
| **Interact** | E or Middle Mouse Button |
| **Inventory** | I |
| **Pause** | P |
| **Quit** | Escape |

## Project Structure

```
PixLegends/
├── src/                    # Source files
│   ├── main.cpp              # Entry point
│   ├── Game.cpp              # Main game loop and system management
│   ├── Player.cpp            # Player character logic
│   ├── AssetManager.cpp      # Asset loading and caching
│   ├── InputManager.cpp      # Input handling
│   ├── Renderer.cpp          # SDL2 rendering utilities
│   ├── World.cpp             # World/tilemap management
│   ├── UISystem.cpp          # User interface
│   └── AudioManager.cpp      # Audio system
├── include/               # Header files
│   ├── Game.h
│   ├── Player.h
│   ├── AssetManager.h
│   ├── InputManager.h
│   ├── Renderer.h
│   ├── World.h
│   ├── UISystem.h
│   └── AudioManager.h
├── assets/                # Game assets (sprites, tilesets, etc.)
├── external/              # External dependencies (SDL2, etc.)
├── CMakeLists.txt            # Build configuration
├── build.bat                 # Windows build script
├── build.sh                  # Linux/macOS build script
└── README.md                 # This file
```

## Quick Start

### Prerequisites

- **CMake** (version 3.16 or higher)
- **C++17 compatible compiler** (GCC 7+, Clang 5+, MSVC 2017+)
- **SDL2** development libraries
- **SDL2_image** development libraries
- **SDL2_ttf** development libraries

### Windows

#### Option 1: Using Visual Studio

1. **Install SDL2 libraries**:
   ```cmd
   # Run the setup script (if available)
   setup_sdl2.bat
   ```

2. **Build the project**:
   ```cmd
   mkdir build
   cd build
   cmake .. -G "Visual Studio 16 2019" -A x64
   cmake --build . --config Release
   ```

#### Option 2: Using MinGW

1. **Install SDL2 via MSYS2**:
   ```bash
   pacman -S mingw-w64-x86_64-sdl2 mingw-w64-x86_64-sdl2_image mingw-w64-x86_64-sdl2_ttf
   ```

2. **Build the project**:
   ```bash
   mkdir build
   cd build
   cmake .. -G "MinGW Makefiles"
   cmake --build .
   ```

### Linux

1. **Install dependencies**:
   ```bash
   # Ubuntu/Debian
   sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev
   
   # Fedora
   sudo dnf install SDL2-devel SDL2_image-devel SDL2_ttf-devel
   
   # Arch Linux
   sudo pacman -S sdl2 sdl2_image sdl2_ttf
   ```

2. **Build the project**:
   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```

### macOS

1. **Install dependencies via Homebrew**:
   ```bash
   brew install sdl2 sdl2_image sdl2_ttf
   ```

2. **Build the project**:
   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```

## Running the Game

After building, run the executable from the build directory:

```bash
# Linux/macOS
./PixLegends

# Windows
PixLegends.exe
```

## Asset Requirements

The game expects the following asset structure:

```
assets/
├── Wizard 2D Pixel Art v2.0/
│   └── Sprites/
│       └── without_outline/
│           ├── IDLE.png
│           ├── WALK.png
│           ├── MELEE ATTACK.png
│           ├── RANGED ATTACK.png
│           ├── HURT.png
│           └── DEATH.png
├── FULL_Fantasy Forest/
│   ├── Tiles/
│   │   ├── Tileset Inside.png
│   │   └── Tileset Outside.png
│   └── Backgrounds/
│       ├── Sky.png
│       └── Grass Mountains.png
└── fonts/
    └── arial.ttf  # Font file for UI text
```

## Development

### Adding New Features

1. **Enemy AI**: Implement in `World.cpp` with state machines
2. **Projectile System**: Add to `Player.cpp` for ranged attacks
3. **Inventory System**: Create new `Inventory.h/cpp` files
4. **Audio**: Complete `AudioManager.cpp` with SDL2_mixer integration

### Code Style

- Use modern C++17 features
- Follow RAII principles
- Use smart pointers for memory management
- Keep functions small and focused
- Add comments for complex logic

## Troubleshooting

### Common Issues

| Issue | Solution |
|-------|----------|
| **SDL2 not found** | Ensure SDL2 development libraries are installed |
| **Font not loading** | Place a font file in `assets/fonts/` |
| **Assets not found** | Verify asset paths match the expected structure |
| **Build errors** | Check compiler compatibility with C++17 |

### Debug Mode

Build in debug mode for additional logging:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
```

## Contributing

We welcome contributions! Please follow these steps:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Development Guidelines

- Follow the existing code style
- Add tests for new features
- Update documentation as needed
- Ensure all tests pass before submitting

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Roadmap

- [ ] Enemy AI and combat
- [ ] Projectile system
- [ ] Inventory and equipment
- [ ] Audio system integration
- [ ] Level loading from files
- [ ] Save/load system
- [ ] Particle effects
- [ ] More enemy types
- [ ] Boss battles
- [ ] Multiplayer support

## Acknowledgments

- **SDL2** - Cross-platform development library
- **Pixel Art Assets** - Various pixel art creators
- **CMake** - Build system generator

## Support

If you encounter any issues or have questions:

1. Check the [Issues](../../issues) page
2. Search existing discussions
3. Create a new issue with detailed information

---

**Made with C++**
