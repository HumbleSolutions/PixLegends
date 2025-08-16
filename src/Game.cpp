#include "Game.h"
#include "Renderer.h"
#include "InputManager.h"
#include "AssetManager.h"
#include "Player.h"
#include "Projectile.h"
#include "World.h"
#include "Enemy.h"
#include "Boss.h"
#include "UISystem.h"
#include "AudioManager.h"
#include "Object.h"
#include "DatabaseSQLite.h"
#include "LootGenerator.h"
#include "ItemSystem.h"
#include "SpellSystem.h"
#include <iostream>
#include <random>

Game::Game() : window(nullptr), sdlRenderer(nullptr), isRunning(false), isPaused(false), 
               lastFrameTime(0), accumulator(0.0f), frameTime(0), currentFPS(0.0f), averageFPS(0.0f) {
    initializeSystems();
}

void Game::enterUnderworld() {
    if (!world || !assetManager || !player) return;
    std::cout << "=== ENTERING UNDERWORLD ===" << std::endl;
    inUnderworld = true;
    // Load the provided TMX underworld map
    world->loadTilemap("assets/Underworld Tilemap/TiledMap Editor/sample map.tmx");
    // Safe spawn: search for nearest safe tile around a preferred entrance
    int ts = world->getTileSize();
    int preferTX = 8, preferTY = 8; // near a likely solid start on platform tiles
    int safeTX = preferTX, safeTY = preferTY;
    const int maxSearch = 200;
    bool found = false;
    for (int r = 0; r <= maxSearch && !found; ++r) {
        for (int dy = -r; dy <= r && !found; ++dy) {
            for (int dx = -r; dx <= r; ++dx) {
                int tx = preferTX + dx;
                int ty = preferTY + dy;
                if (world->isSafeTile(tx, ty)) { safeTX = tx; safeTY = ty; found = true; break; }
            }
        }
    }
    float spawnX = static_cast<float>(safeTX * ts);
    float spawnY = static_cast<float>(safeTY * ts);
    player->respawn(spawnX, spawnY);
    // Force player to reinitialize sprites after world change
    player->setState(player->getState());
    // Update camera and visibility for the new map
    world->updateVisibility(player->getX(), player->getY());
    world->updateVisibleChunks(player->getX(), player->getY());
    
    // Reinitialize objects for the underworld
    initializeObjects();
}

void Game::exitUnderworld() {
    if (!world || !assetManager || !player) return;
    std::cout << "=== EXITING UNDERWORLD ===" << std::endl;
    inUnderworld = false;
    // Reset to procedural world generation (the default state)
    // Create a new world instance to reset to procedural generation
    world = std::make_unique<World>(assetManager.get());
    
    // Set player spawn to the default overworld location
    int ts = world->getTileSize();
    float spawnX = static_cast<float>(20 * ts); // Default spawn near the original objects
    float spawnY = static_cast<float>(11 * ts);
    player->respawn(spawnX, spawnY);
    
    // Force player to reinitialize sprites after world change
    player->setState(player->getState());
    // Update camera and visibility for the new map
    world->updateVisibility(player->getX(), player->getY());
    world->updateVisibleChunks(player->getX(), player->getY());
    
    // Reinitialize objects for the overworld
    initializeObjects();
}

float Game::getCameraX() const {
    if (renderer) {
        int camX, camY;
        renderer->getCamera(camX, camY);
        return static_cast<float>(camX);
    }
    return 0.0f;
}

float Game::getCameraY() const {
    if (renderer) {
        int camX, camY;
        renderer->getCamera(camX, camY);
        return static_cast<float>(camY);
    }
    return 0.0f;
}

float Game::getWorldWidth() const {
    if (world) {
        return static_cast<float>(world->getWidth() * world->getTileSize());
    }
    return 0.0f;
}

float Game::getWorldHeight() const {
    if (world) {
        return static_cast<float>(world->getHeight() * world->getTileSize());
    }
    return 0.0f;
}

Game::~Game() {
    saveCurrentUserState();
    cleanup();
}

void Game::initializeSystems() {
    // Create window (start in borderless fullscreen for best compatibility)
    Uint32 windowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN_DESKTOP;
#ifdef SDL_WINDOW_ALLOW_HIGHDPI
    windowFlags |= SDL_WINDOW_ALLOW_HIGHDPI;
#endif
    window = SDL_CreateWindow(
        "PixLegends - 2D Action Adventure",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        windowFlags
    );
    
    if (!window) {
        throw std::runtime_error("Failed to create window: " + std::string(SDL_GetError()));
    }

    // Create renderer
    sdlRenderer = SDL_CreateRenderer(window, -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    // Ensure fullscreen is applied (some platforms ignore flag at creation)
    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    
    if (!sdlRenderer) {
        throw std::runtime_error("Failed to create renderer: " + std::string(SDL_GetError()));
    }

    // Initialize systems
    database = std::make_unique<DatabaseSQLite>();
    // Use executable directory as base so saves are consistent regardless of working directory
    std::string dataRootPath = "data";
    if (const char* basePath = SDL_GetBasePath()) {
        dataRootPath = std::string(basePath) + "data";
        SDL_free((void*)basePath);
    }
    database->initialize(dataRootPath + "/pixlegends.db");
    // Preload remembered login if any
    {
        auto remember = database->loadRememberState();
        std::cout << "Checking remember state... remember=" << (remember.remember ? "true" : "false") << ", username=" << remember.username << std::endl;
        if (remember.remember) {
            loginRemember = true;
            loginUsername = remember.username;
            loginPassword = remember.password;
            
            // Automatically attempt login with remembered credentials
            std::string error;
            auto user = database->authenticate(loginUsername, loginPassword, &error);
            if (user) {
                loggedInUserId = user->userId;
                loginIsAdmin = (user->role == UserRole::ADMIN);
                loginScreenActive = false;
                std::cout << "Auto-logged in user: " << loginUsername << " (ID: " << user->userId << ")" << std::endl;
            } else {
                std::cout << "Auto-login failed for remembered user: " << loginUsername << " - " << error << std::endl;
                // Clear invalid remember state
                database->clearRememberState();
                loginRemember = false;
                loginUsername.clear();
                loginPassword.clear();
            }
        }
    }
    renderer = std::make_unique<Renderer>(sdlRenderer);
    // Enable zoom; all renderers now respect zoom consistently
    renderer->setZoom(1.8f);
    inputManager = std::make_unique<InputManager>();
    assetManager = std::make_unique<AssetManager>(sdlRenderer);
    
    // Preload assets BEFORE creating World
    assetManager->preloadAssets();
    
    // Create World after assets are loaded
    world = std::make_unique<World>(assetManager.get());
    uiSystem = std::make_unique<UISystem>(sdlRenderer);
    uiSystem->setAssetManager(assetManager.get());
    audioManager = std::make_unique<AudioManager>();
    if (audioManager) {
        // Favor SFX over music by default
        audioManager->setMasterVolume(80);
        audioManager->setSoundVolume(100);
        audioManager->setMusicVolume(40);
        audioManager->loadSound("footstep_dirt", "assets/Sound/Footsteps/dirt_step.wav");
        // Player melee variants (alternate)
        audioManager->loadSound("player_melee_1", "assets/Sound/Melee/player_melee.wav");
        audioManager->loadSound("player_melee_1", "assets/Sound/Melee/player_melee_1.wav");
        audioManager->loadSound("player_melee_2", "assets/Sound/Melee/player_melee_2.wav");
        audioManager->loadSound("boss_melee", "assets/Sound/Melee/demon_melee.wav");
        audioManager->loadSound("player_projectile", "assets/Sound/Spells/fire_projectile.wav");
        audioManager->loadSound("potion_drink", "assets/Sound/Spells/potion_drink.wav");
        audioManager->loadSound("upgrade_sound", "assets/Sound/Spells/upgrade_sound.wav");
        // EXP gather SFX
        audioManager->loadSound("exp_gather", "assets/Sound/Effects/exp_gather_sound.wav");
        // Fire shield looped SFX
        audioManager->loadSound("fire_shield_loop", "assets/Sound/Spells/fire_sheild_sound.wav");
        // Load themes once; use distinct keys so persistence matches names
        audioManager->loadMusic("main_theme", "assets/Sound/Music/main_theme.wav");
        audioManager->loadMusic("fast_tempo", "assets/Sound/Music/fast_tempo_theme.wav");
        audioManager->loadMusic("boss_music", "assets/Sound/Music/boss_music.wav");
        // Goblin SFX
        audioManager->loadSound("goblin_melee", "assets/Sound/Melee/goblin_melee.wav");
        audioManager->loadSound("goblin_death", "assets/Sound/Death/goblin_death_sound.wav");
    }
    
    // Create player after other systems are initialized
    player = std::make_unique<Player>(this);
    
    // Load player save data if auto-logged in from remembered credentials
    if (!loginScreenActive && loggedInUserId > 0) {
        auto save = database->loadPlayerState(loggedInUserId);
        if (save) {
            player->applySaveState(*save);
            std::cout << "Loaded save data for auto-logged user (level " << save->level << ")" << std::endl;
        }
        // Load audio settings
        if (audioManager) {
            int m=100, mu=100, s=100, mon=100, pl=100;
            if (database->loadAudioSettings(loggedInUserId, m, mu, s, mon, pl)) {
                audioManager->setMasterVolume(m);
                audioManager->setMusicVolume(mu);
                audioManager->setSoundVolume(s);
                audioManager->setMonsterVolume(mon);
                audioManager->setPlayerVolume(pl);
            }
        }
    } else {
        // Start at login screen; defer save load until authenticated
        loadOrCreateDefaultUserAndSave();
    }
    // Safe spawn: find nearest non-hazard tile around preferred spot
    if (world && player) {
        int ts = world->getTileSize();
        int preferTX = 15;
        int preferTY = 10;
        int safeTX = preferTX;
        int safeTY = preferTY;
        const int maxSearch = 200; // tiles radius search cap
        bool found = false;
        for (int r = 0; r <= maxSearch && !found; ++r) {
            for (int dy = -r; dy <= r && !found; ++dy) {
                for (int dx = -r; dx <= r; ++dx) {
                    int tx = preferTX + dx;
                    int ty = preferTY + dy;
                    if (world->isSafeTile(tx, ty)) { safeTX = tx; safeTY = ty; found = true; break; }
                }
            }
        }
        float spawnX = static_cast<float>(safeTX * ts);
        float spawnY = static_cast<float>(safeTY * ts);
        player->setSpawnPoint(spawnX, spawnY);
        player->respawn(spawnX, spawnY);
    }
    
    // Initialize objects in the world
    initializeObjects();

    // Autotiling demo removed

    // Spawn enemies: Demon and Wizard (elite), and a few Goblin minions (trash)
    if (world && assetManager) {
        // Demon near start
        float demonX = 26.0f * world->getTileSize();
        float demonY = 11.0f * world->getTileSize();
        {
            auto e = std::make_unique<Enemy>(demonX, demonY, assetManager.get(), EnemyKind::Demon);
            e->setPackRarity(PackRarity::Elite);
            world->addEnemy(std::move(e));
        }
        // Wizard nearby
        float wizX = 30.0f * world->getTileSize();
        float wizY = 14.0f * world->getTileSize();
        {
            auto e = std::make_unique<Enemy>(wizX, wizY, assetManager.get(), EnemyKind::Wizard);
            e->setPackRarity(PackRarity::Elite);
            world->addEnemy(std::move(e));
        }

        // Goblin minion pack (trash/grey): small cluster
        const int ts = world->getTileSize();
        for (int i = 0; i < 4; ++i) {
            float gx = (20.0f + (i % 2) * 1.5f) * ts;
            float gy = (16.0f + (i / 2) * 1.5f) * ts;
            auto g = std::make_unique<Enemy>(gx, gy, assetManager.get(), EnemyKind::Goblin);
            g->setPackRarity(PackRarity::Trash);
            g->setRenderScale(0.75f); // smaller on-screen
            world->addEnemy(std::move(g));
        }
        
        // Add some skeleton warriors to test sprite flipping
        for (int i = 0; i < 3; i++) {
            float sx = (25.0f + i * 2.0f) * ts;
            float sy = 18.0f * ts;
            auto skeleton = std::make_unique<Enemy>(sx, sy, assetManager.get(), EnemyKind::Skeleton);
            skeleton->setPackRarity(PackRarity::Common);
            world->addEnemy(std::move(skeleton));
        }
        
        // Add Trash tier enemies for testing
        // Imps
        for (int i = 0; i < 2; i++) {
            float ix = (30.0f + i * 1.5f) * ts;
            float iy = 15.0f * ts;
            auto imp = std::make_unique<Enemy>(ix, iy, assetManager.get(), EnemyKind::Imp);
            imp->setPackRarity(PackRarity::Trash);
            world->addEnemy(std::move(imp));
        }
        
        // Flying Eyes
        for (int i = 0; i < 2; i++) {
            float fx = (33.0f + i * 1.5f) * ts;
            float fy = 16.0f * ts;
            auto eye = std::make_unique<Enemy>(fx, fy, assetManager.get(), EnemyKind::FlyingEye);
            eye->setPackRarity(PackRarity::Trash);
            world->addEnemy(std::move(eye));
        }
        
        // Poison Skulls
        for (int i = 0; i < 2; i++) {
            float px = (36.0f + i * 1.5f) * ts;
            float py = 17.0f * ts;
            auto skull = std::make_unique<Enemy>(px, py, assetManager.get(), EnemyKind::PoisonSkull);
            skull->setPackRarity(PackRarity::Trash);
            world->addEnemy(std::move(skull));
        }
        
        // Add Common tier enemies for testing
        // Lizardman
        for (int i = 0; i < 2; i++) {
            float lx = (40.0f + i * 1.5f) * ts;
            float ly = 19.0f * ts;
            auto lizard = std::make_unique<Enemy>(lx, ly, assetManager.get(), EnemyKind::Lizardman);
            lizard->setPackRarity(PackRarity::Common);
            world->addEnemy(std::move(lizard));
        }
        
        // Dwarf Warrior
        for (int i = 0; i < 2; i++) {
            float dx = (43.0f + i * 1.5f) * ts;
            float dy = 20.0f * ts;
            auto dwarf = std::make_unique<Enemy>(dx, dy, assetManager.get(), EnemyKind::DwarfWarrior);
            dwarf->setPackRarity(PackRarity::Common);
            world->addEnemy(std::move(dwarf));
        }
        
        // Harpy
        for (int i = 0; i < 2; i++) {
            float hx = (46.0f + i * 1.5f) * ts;
            float hy = 21.0f * ts;
            auto harpy = std::make_unique<Enemy>(hx, hy, assetManager.get(), EnemyKind::Harpy);
            harpy->setPackRarity(PackRarity::Common);
            world->addEnemy(std::move(harpy));
        }
        
        // Add Magic tier enemies for testing
        // Skeleton Mage
        for (int i = 0; i < 2; i++) {
            float smx = (50.0f + i * 2.0f) * ts;
            float smy = 22.0f * ts;
            auto skeleMage = std::make_unique<Enemy>(smx, smy, assetManager.get(), EnemyKind::SkeletonMage);
            skeleMage->setPackRarity(PackRarity::Magic);
            world->addEnemy(std::move(skeleMage));
        }
        
        // Pyromancer
        for (int i = 0; i < 2; i++) {
            float pmx = (53.0f + i * 2.0f) * ts;
            float pmy = 23.0f * ts;
            auto pyro = std::make_unique<Enemy>(pmx, pmy, assetManager.get(), EnemyKind::Pyromancer);
            pyro->setPackRarity(PackRarity::Magic);
            world->addEnemy(std::move(pyro));
        }
        
        // Witch
        for (int i = 0; i < 2; i++) {
            float wx = (56.0f + i * 2.0f) * ts;
            float wy = 24.0f * ts;
            auto witch = std::make_unique<Enemy>(wx, wy, assetManager.get(), EnemyKind::Witch);
            witch->setPackRarity(PackRarity::Magic);
            world->addEnemy(std::move(witch));
        }
        
        // Elite tier enemies - spread across the world for testing
        const float eliteSpacing = 8.0f;  // More space between powerful enemies
        
        // First row of Elite enemies (Dragons, Minotaur, etc.)
        float eliteX = 80.0f * ts;
        float eliteY = 10.0f * ts;
        
        auto dragon = std::make_unique<Enemy>(eliteX, eliteY, assetManager.get(), EnemyKind::Dragon);
        dragon->setPackRarity(PackRarity::Elite);
        world->addEnemy(std::move(dragon));
        
        auto minotaur = std::make_unique<Enemy>(eliteX + eliteSpacing * ts, eliteY, assetManager.get(), EnemyKind::Minotaur);
        minotaur->setPackRarity(PackRarity::Elite);
        world->addEnemy(std::move(minotaur));
        
        auto stoneGolem = std::make_unique<Enemy>(eliteX + 2 * eliteSpacing * ts, eliteY, assetManager.get(), EnemyKind::StoneGolem);
        stoneGolem->setPackRarity(PackRarity::Elite);
        world->addEnemy(std::move(stoneGolem));
        
        auto hugeKnight = std::make_unique<Enemy>(eliteX + 3 * eliteSpacing * ts, eliteY, assetManager.get(), EnemyKind::HugeKnight);
        hugeKnight->setPackRarity(PackRarity::Elite);
        world->addEnemy(std::move(hugeKnight));
        
        auto cyclops = std::make_unique<Enemy>(eliteX + 4 * eliteSpacing * ts, eliteY, assetManager.get(), EnemyKind::Cyclops);
        cyclops->setPackRarity(PackRarity::Elite);
        world->addEnemy(std::move(cyclops));
        
        // Second row of Elite enemies
        eliteY = 15.0f * ts;
        
        auto centaur = std::make_unique<Enemy>(eliteX, eliteY, assetManager.get(), EnemyKind::Centaur);
        centaur->setPackRarity(PackRarity::Elite);
        world->addEnemy(std::move(centaur));
        
        auto headlessHorseman = std::make_unique<Enemy>(eliteX + eliteSpacing * ts, eliteY, assetManager.get(), EnemyKind::HeadlessHorseman);
        headlessHorseman->setPackRarity(PackRarity::Elite);
        world->addEnemy(std::move(headlessHorseman));
        
        auto medusa = std::make_unique<Enemy>(eliteX + 2 * eliteSpacing * ts, eliteY, assetManager.get(), EnemyKind::Medusa);
        medusa->setPackRarity(PackRarity::Elite);
        world->addEnemy(std::move(medusa));
        
        auto cerberus = std::make_unique<Enemy>(eliteX + 3 * eliteSpacing * ts, eliteY, assetManager.get(), EnemyKind::Cerberus);
        cerberus->setPackRarity(PackRarity::Elite);
        world->addEnemy(std::move(cerberus));
        
        auto gryphon = std::make_unique<Enemy>(eliteX + 4 * eliteSpacing * ts, eliteY, assetManager.get(), EnemyKind::Gryphon);
        gryphon->setPackRarity(PackRarity::Elite);
        world->addEnemy(std::move(gryphon));
        
        // Third row of Elite enemies
        eliteY = 20.0f * ts;
        
        auto gargoyle = std::make_unique<Enemy>(eliteX, eliteY, assetManager.get(), EnemyKind::Gargoyle);
        gargoyle->setPackRarity(PackRarity::Elite);
        world->addEnemy(std::move(gargoyle));
        
        auto werewolf = std::make_unique<Enemy>(eliteX + eliteSpacing * ts, eliteY, assetManager.get(), EnemyKind::Werewolf);
        werewolf->setPackRarity(PackRarity::Elite);
        world->addEnemy(std::move(werewolf));
        
        auto mimic = std::make_unique<Enemy>(eliteX + 2 * eliteSpacing * ts, eliteY, assetManager.get(), EnemyKind::Mimic);
        mimic->setPackRarity(PackRarity::Elite);
        world->addEnemy(std::move(mimic));
        
        auto maskedOrc = std::make_unique<Enemy>(eliteX + 3 * eliteSpacing * ts, eliteY, assetManager.get(), EnemyKind::MaskedOrc);
        maskedOrc->setPackRarity(PackRarity::Elite);
        world->addEnemy(std::move(maskedOrc));
        
        auto koboldWarrior = std::make_unique<Enemy>(eliteX + 4 * eliteSpacing * ts, eliteY, assetManager.get(), EnemyKind::KoboldWarrior);
        koboldWarrior->setPackRarity(PackRarity::Elite);
        world->addEnemy(std::move(koboldWarrior));
        
        // Fourth row of Elite enemies (final ones)
        eliteY = 25.0f * ts;
        
        auto satyrArcher = std::make_unique<Enemy>(eliteX, eliteY, assetManager.get(), EnemyKind::SatyrArcher);
        satyrArcher->setPackRarity(PackRarity::Elite);
        world->addEnemy(std::move(satyrArcher));
        
        auto babyDragon = std::make_unique<Enemy>(eliteX + eliteSpacing * ts, eliteY, assetManager.get(), EnemyKind::BabyDragon);
        babyDragon->setPackRarity(PackRarity::Elite);
        world->addEnemy(std::move(babyDragon));
    }
    
    // Set initial visibility and generate initial visible chunks around player starting position
    if (world && player) {
        world->updateVisibility(player->getX(), player->getY());
        world->updateVisibleChunks(player->getX(), player->getY());
    }
    // Start background theme at game start (will be overridden by saved theme in loadOrCreateDefaultUserAndSave)
    backgroundMusicName = "main_theme";
    if (audioManager && audioManager->hasMusic(backgroundMusicName)) {
        audioManager->playMusic(backgroundMusicName);
        currentMusicTrack = backgroundMusicName;
    } else {
        // Attempt to fall back to fast_tempo if main_theme asset missing
        backgroundMusicName = "fast_tempo";
        if (audioManager && audioManager->hasMusic(backgroundMusicName)) {
            audioManager->playMusic(backgroundMusicName);
            currentMusicTrack = backgroundMusicName;
        } else {
            currentMusicTrack.clear();
        }
    }
    
    isRunning = true;
    lastFrameTime = SDL_GetTicks();
}

void Game::run() {
    while (isRunning) {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastFrameTime) / 1000.0f;
        lastFrameTime = currentTime;
        
        // Cap delta time to prevent spiral of death
        if (deltaTime > 0.1f) deltaTime = 0.1f;
        
        accumulator += deltaTime;
        
        // Fixed timestep for physics/updates
        while (accumulator >= TARGET_FRAME_TIME) {
            if (!isPaused) {
                update(TARGET_FRAME_TIME);
            }
            accumulator -= TARGET_FRAME_TIME;
        }
        
        // Handle events
        handleEvents();
        
        // Render (variable timestep for smooth rendering)
        render();
        
        // Update performance metrics
        updatePerformanceMetrics();
        
        // Cap frame rate
        frameTime = SDL_GetTicks() - currentTime;
        if (frameTime < (1000 / TARGET_FPS)) {
            SDL_Delay((1000 / TARGET_FPS) - frameTime);
        }
    }
}

void Game::update(float deltaTime) {
    // While on login screen, do not update gameplay systems to avoid sounds/movement
    if (loginScreenActive) {
        if (uiSystem) uiSystem->update(deltaTime);
        return;
    }
    // Update player (keep world running while inventory/anvil are open)
    if (player) {
        player->update(deltaTime);
    }
    
    // Update world
    if (world) {
        bool playedPlayerMeleeSfx = false; // prevent boss melee SFX from overriding on the same frame
        world->update(deltaTime);

        // Periodic goblin minion respawns near the player in waves; cap scales with player level
        {
            static float goblinSpawnTimer = 0.0f;
            static int waveId = 0;
            // Wave cadence
            goblinSpawnTimer += deltaTime;
            if (goblinSpawnTimer >= 8.0f) { // spawn cadence per wave tick
                goblinSpawnTimer = 0.0f;
                waveId++;

                // Count alive goblins
                int aliveGoblins = 0;
                for (auto& e : world->getEnemies()) {
                    if (e && std::string(e->getDisplayName()) == std::string("Goblin") && !e->isDead()) aliveGoblins++;
                }

                // Dynamic cap by player level
                int level = player ? player->getLevel() : 1;
                const int caps[10] = {5,8,11,14,17,20,23,26,28,30};
                int maxGoblins = (level <= 0 ? 5 : (level >= 10 ? 30 : caps[level-1]));

                // Prevent spawning while player is near the Magic Anvil
                bool playerOnAnvil = false;
                if (player) {
                    int ts = world->getTileSize();
                    int ptx = static_cast<int>(player->getX() / ts);
                    int pty = static_cast<int>(player->getY() / ts);
                    for (const auto& obj : world->getObjects()) {
                        if (obj && obj->getType() == ObjectType::MAGIC_ANVIL) {
                            int ax = obj->getX(); int ay = obj->getY();
                            int dx = ptx - ax; int dy = pty - ay;
                            if (dx*dx + dy*dy <= 3*3) { playerOnAnvil = true; break; }
                        }
                    }
                }

                if (!stopMonsterSpawns && !playerOnAnvil && aliveGoblins < maxGoblins) {
                    // Wave size scales mildly with level and alternates per waveId
                    int base = 2 + (level / 3); // grows slowly
                    int variance = (waveId % 3); // 0..2
                    int packSize = std::min(8, base + variance + static_cast<int>(SDL_GetTicks() % 3));
                    int ts = world->getTileSize();
                    // Spawn NEAR the player within a ring to make it noticeable
                    int playerTileX = static_cast<int>(player->getX() / ts);
                    int playerTileY = static_cast<int>(player->getY() / ts);
                    int radiusTilesMin = 6;
                    int radiusTilesMax = 14;
                    int W = world->getWidth();
                    int H = world->getHeight();
                    for (int i = 0; i < packSize; ++i) {
                        int tries = 0; bool placed = false;
                        while (tries++ < 80 && !placed) {
                            // Pick a random offset in an annulus around the player
                            Uint32 rseed = SDL_GetTicks() + i * 1337 + tries * 31;
                            int dx = static_cast<int>(static_cast<int>(rseed % (radiusTilesMax*2+1)) - radiusTilesMax);
                            int dy = static_cast<int>(static_cast<int>((rseed/3) % (radiusTilesMax*2+1)) - radiusTilesMax);
                            int dist2 = dx*dx + dy*dy;
                            if (dist2 < radiusTilesMin*radiusTilesMin || dist2 > radiusTilesMax*radiusTilesMax) continue;
                            int tx = std::max(0, std::min(W-1, playerTileX + dx));
                            int ty = std::max(0, std::min(H-1, playerTileY + dy));
                            if (!world->isSafeTile(tx, ty)) continue;
                            // Avoid spawning within the anvil safe zone
                            bool nearAnvil = false;
                            for (const auto& obj : world->getObjects()) {
                                if (obj && obj->getType() == ObjectType::MAGIC_ANVIL) {
                                    int ax = obj->getX(); int ay = obj->getY();
                                    int adx = tx - ax; int ady = ty - ay;
                                    if (adx*adx + ady*ady <= 3*3) { nearAnvil = true; break; }
                                }
                            }
                            if (nearAnvil) continue;
                            float gx = tx * ts + ts * 0.5f;
                            float gy = ty * ts + ts * 0.5f;
                            auto g = std::make_unique<Enemy>(gx, gy, assetManager.get(), EnemyKind::Goblin);
                            // Goblins are always trash mobs
                            g->setPackRarity(PackRarity::Trash);
                            g->setRenderScale(0.75f);
                            world->addEnemy(std::move(g));
                            placed = true;
                        }
                    }
                }
            }
        }

        // EXP orb magnet and auto-pickup when walking over them
        if (player) {
            auto& objects = const_cast<std::vector<std::unique_ptr<Object>>&>(world->getObjects());
            SDL_Rect playerRect = player->getCollisionRect();
            const float playerCenterX = player->getX() + player->getWidth() * 0.5f;
            const float playerCenterY = player->getY() + player->getHeight() * 0.5f;
            const float activationRadius = 180.0f; // pixels
            std::vector<std::pair<int,int>> pendingRemovals;

            for (auto& objPtr : objects) {
                if (!objPtr) continue;
                Object* o = objPtr.get();
                ObjectType t = o->getType();
                if (t != ObjectType::EXP_ORB1 && t != ObjectType::EXP_ORB2 && t != ObjectType::EXP_ORB3) continue;

                int ts = world->getTileSize();
                o->setTileSizeHint(ts);
                float ox = o->getPixelX();
                float oy = o->getPixelY();
                // Default to tile center if not initialized
                if (ox <= 0.0f && oy <= 0.0f) {
                    ox = o->getX() * ts + ts * 0.5f;
                    oy = o->getY() * ts + ts * 0.5f;
                    o->setPositionPixels(ox, oy);
                }

                float dx = playerCenterX - ox;
                float dy = playerCenterY - oy;
                float distSq = dx*dx + dy*dy;
                // Respect 2s delay before beginning magnet
                bool magnetActive = true;
                Uint32 now = SDL_GetTicks();
                float sinceSpawnSec = (now - o->getSpawnTicks()) / 1000.0f;
                if (sinceSpawnSec < o->getMagnetDelaySeconds()) magnetActive = false;

                if (magnetActive && distSq <= activationRadius * activationRadius) {
                    float dist = std::max(1.0f, std::sqrt(distSq));
                    float nx = dx / dist;
                    float ny = dy / dist;
                    // Speed increases as you get closer
                    float base = 60.0f; // px/sec
                    float bonus = 240.0f * (1.0f - std::min(1.0f, dist / activationRadius));
                    float speed = base + bonus;
                    ox += nx * speed * deltaTime;
                    oy += ny * speed * deltaTime;
                    o->setPositionPixels(ox, oy);
                }

                // Auto-pickup if touches player collision rect
                SDL_Rect orbRect{ static_cast<int>(ox) - 6, static_cast<int>(oy) - 6, 12, 12 };
                SDL_Rect inter;
                if (SDL_IntersectRect(&playerRect, &orbRect, &inter)) {
                    // Transfer XP loot
                    if (o->hasLoot()) {
                        auto loot = o->getLoot();
                        for (const auto& item : loot) {
                            if (item.type == LootType::EXPERIENCE) {
                                player->gainExperience(item.amount);
                                if (audioManager) audioManager->playSound("exp_gather");
                            }
                        }
                    }
                    o->setVisible(false);
                    pendingRemovals.emplace_back(o->getX(), o->getY());
                }
            }

            for (const auto& rc : pendingRemovals) {
                world->removeObject(rc.first, rc.second);
            }
        }
        
        // Update chunks and visibility based on player position
        if (player) {
            world->updateVisibleChunks(player->getX(), player->getY());
            world->updateVisibility(player->getX(), player->getY());
            // Update enemies with player tracking
            // Use player center for enemy AI distance checks
            float playerCenterX = player->getX() + player->getWidth() * 0.5f;
            float playerCenterY = player->getY() + player->getHeight() * 0.5f;
            world->updateEnemies(deltaTime, playerCenterX, playerCenterY);
            // Handle combat interactions after updates
            // 1) Player projectiles vs enemies
            auto& enemies = const_cast<std::vector<std::unique_ptr<Enemy>>&>(world->getEnemies());
            auto& playerProjectiles = player->getProjectiles();
            for (auto& projPtr : playerProjectiles) {
                if (!projPtr || !projPtr->isActive()) continue;
                SDL_Rect pRect = projPtr->getCollisionRect();
                for (auto& enemyPtr : enemies) {
                    if (!enemyPtr || enemyPtr->isDead()) continue;
                    SDL_Rect eRect = enemyPtr->getCollisionRect();
                    SDL_Rect inter;
                    if (SDL_IntersectRect(&pRect, &eRect, &inter)) {
                        enemyPtr->takeDamage(projPtr->getDamage());
                        projPtr->deactivate();
                        break;
                    }
                }
            }

            // 1b) Player melee vs enemies (once per swing during active frames)
            if (player->isMeleeAttacking()) {
                SDL_Rect hitbox = player->getMeleeHitbox();
                if (player->isMeleeHitActive()) {
                    bool hitSomething = false;
                    
                    // Check boss first (higher priority)
                    if (world->hasBoss() && world->getCurrentBoss() && !world->getCurrentBoss()->isDead()) {
                        Boss* boss = world->getCurrentBoss();
                        SDL_Rect bossRect = boss->getCollisionRect();
                        SDL_Rect inter;
                        if (SDL_IntersectRect(&hitbox, &bossRect, &inter)) {
                            if (player->consumeMeleeHitIfActive()) {
                                boss->takeDamage(player->rollMeleeDamageForHit());
                                hitSomething = true;
                            }
                        }
                    }
                    
                    // Check regular enemies if we didn't hit a boss
                    if (!hitSomething) {
                        for (auto& enemyPtr : enemies) {
                            if (!enemyPtr || enemyPtr->isDead()) continue;
                            SDL_Rect eRect = enemyPtr->getCollisionRect();
                            SDL_Rect inter;
                            if (SDL_IntersectRect(&hitbox, &eRect, &inter)) {
                                if (player->consumeMeleeHitIfActive()) {
                                    enemyPtr->takeDamage(player->rollMeleeDamageForHit());
                                }
                                break;
                            }
                        }
                    }
                }
            }

            // 2) Enemy contact damage to player (simple melee) and wizard projectiles
            for (auto& enemyPtr : enemies) {
                if (!enemyPtr || enemyPtr->isDead()) continue;
                SDL_Rect eRect = enemyPtr->getCollisionRect();
                SDL_Rect playerRect = player->getCollisionRect();
                SDL_Rect inter;
                if (SDL_IntersectRect(&playerRect, &eRect, &inter)) {
                    // Only allow contact damage if enemy is fully overlapping the player center or has reached attack state
                    bool deepOverlap = false;
                    {
                        int pcx = static_cast<int>(player->getX() + player->getWidth() * 0.5f);
                        int pcy = static_cast<int>(player->getY() + player->getHeight() * 0.65f);
                        if (pcx >= eRect.x && pcx <= eRect.x + eRect.w &&
                            pcy >= eRect.y && pcy <= eRect.y + eRect.h) {
                            deepOverlap = true;
                        }
                    }
                    if ((enemyPtr->isAttackReady()) && deepOverlap) {
                        player->takeDamage(enemyPtr->getContactDamage());
                        if (audioManager && !playedPlayerMeleeSfx) { 
                            // Use goblin melee when minion; otherwise keep existing
                            if (enemyPtr->getKind() == EnemyKind::Goblin) {
                                audioManager->playSound("goblin_melee");
                            } else {
                                audioManager->playSound("boss_melee");
                            }
                            audioManager->startMusicDuck(0.35f, 0.5f);
                        }
                        enemyPtr->consumeAttackCooldown();
                        if (player->isDead()) {
                            // Reset enemies to idle spawn when player dies
                            for (auto& e2 : enemies) {
                                if (e2) e2->resetToSpawn();
                            }
                        }
                    }
                }
                // Wizard projectiles
                for (const auto& proj : enemyPtr->getProjectiles()) {
                    if (!proj || !proj->isActive()) continue;
                    SDL_Rect pRect = proj->getCollisionRect();
                    SDL_Rect inter2;
                    if (SDL_IntersectRect(&pRect, &playerRect, &inter2)) {
                        player->takeDamage(12);
                        const_cast<Projectile*>(proj.get())->deactivate();
                    }
                }
            }
            
            // 2a) Boss contact damage to player
            if (world->hasBoss() && world->getCurrentBoss() && !world->getCurrentBoss()->isDead()) {
                Boss* boss = world->getCurrentBoss();
                SDL_Rect bossRect = boss->getCollisionRect();
                SDL_Rect playerRect = player->getCollisionRect();
                SDL_Rect inter;
                if (SDL_IntersectRect(&playerRect, &bossRect, &inter)) {
                    // Only allow contact damage if boss is fully overlapping the player center
                    bool deepOverlap = false;
                    {
                        int pcx = static_cast<int>(player->getX() + player->getWidth() * 0.5f);
                        int pcy = static_cast<int>(player->getY() + player->getHeight() * 0.65f);
                        if (pcx >= bossRect.x && pcx <= bossRect.x + bossRect.w &&
                            pcy >= bossRect.y && pcy <= bossRect.y + bossRect.h) {
                            deepOverlap = true;
                        }
                    }
                    if (deepOverlap && boss->isAttackReady()) {
                        player->takeDamage(boss->getContactDamage());
                        if (audioManager) {
                            audioManager->playSound("boss_melee");
                            audioManager->startMusicDuck(0.35f, 0.5f);
                        }
                        boss->consumeAttackCooldown();
                        if (player->isDead()) {
                            // Reset boss to spawn when player dies
                            boss->resetToSpawn();
                        }
                    }
                }
                
                // Boss projectiles vs player
                for (const auto& proj : boss->getProjectiles()) {
                    if (!proj || !proj->isActive()) continue;
                    SDL_Rect pRect = proj->getCollisionRect();
                    SDL_Rect inter2;
                    if (SDL_IntersectRect(&pRect, &playerRect, &inter2)) {
                        player->takeDamage(15); // Boss projectiles do more damage
                        const_cast<Projectile*>(proj.get())->deactivate();
                    }
                }
            }

            // 2b) Enhanced loot drops from dead enemies (one-time) + corpse despawn
            for (auto& enemyPtr : enemies) {
                if (!enemyPtr) continue;
                if (enemyPtr->isDead() && !enemyPtr->isLootDropped()) {
                    // Generate enhanced loot using the new system
                    int enemyLevel = 1; // Base level for now, could be based on area/zone later
                    float rarityBonus = 0.0f;
                    
                    // Apply rarity bonus based on enemy pack rarity
                    switch (enemyPtr->getPackRarity()) {
                        case PackRarity::Trash:   rarityBonus = -0.1f; break;
                        case PackRarity::Common:  rarityBonus = 0.0f; break;
                        case PackRarity::Magic:   rarityBonus = 0.2f; break;
                        case PackRarity::Elite:   rarityBonus = 0.4f; break;
                    }
                    
                    // Generate loot using the new system
                    auto lootItems = LootGenerator::getInstance().generateEnemyLoot(enemyLevel, rarityBonus);
                    
                    if (world) {
                        int ts = world->getTileSize();
                        int tx = static_cast<int>(enemyPtr->getX()) / ts;
                        int ty = static_cast<int>(enemyPtr->getY()) / ts;
                        
                        // Create loot objects for each dropped item
                        for (const auto& loot : lootItems) {
                            if (loot.type == LootType::GOLD) {
                                // Add gold directly to player
                                player->addGold(loot.amount);
                                continue;
                            }
                            
                            // Create visual loot object
                            std::unique_ptr<Object> lootObj;
                            
                            switch (loot.type) {
                                case LootType::EXPERIENCE: {
                                    // Choose orb type based on XP amount
                                    ObjectType orbType = ObjectType::EXP_ORB1;
                                    const char* orbTex = "assets/Textures/Objects/exp_orb1.png";
                                    
                                    if (loot.amount >= 100) {
                                        orbType = ObjectType::EXP_ORB3;
                                        orbTex = "assets/Textures/Objects/exp_orb3.png";
                                    } else if (loot.amount >= 25) {
                                        orbType = ObjectType::EXP_ORB2;
                                        orbTex = "assets/Textures/Objects/exp_orb2.png";
                                    }
                                    
                                    lootObj = std::make_unique<Object>(orbType, tx, ty, orbTex);
                                    break;
                                }
                                case LootType::HEALTH_POTION:
                                    lootObj = std::make_unique<Object>(ObjectType::CHEST_UNOPENED, tx, ty, 
                                        "assets/Textures/All Potions/HP potions/full_hp_potion.png");
                                    break;
                                case LootType::MANA_POTION:
                                    lootObj = std::make_unique<Object>(ObjectType::CHEST_UNOPENED, tx, ty,
                                        "assets/Textures/All Potions/Mana potion/full_mana_potion.png");
                                    break;
                                default:
                                    // For other loot types (equipment, scrolls, materials), use a generic chest for now
                                    lootObj = std::make_unique<Object>(ObjectType::CHEST_UNOPENED, tx, ty,
                                        "assets/Textures/Objects/chest_unopened.png");
                                    break;
                            }
                            
                            if (lootObj) {
                                lootObj->setAssetManager(assetManager.get());
                                lootObj->setTileSizeHint(ts);
                                
                                // Randomize position slightly to avoid overlap
                                float offsetX = (rand() % 21 - 10) * 0.5f; // -5 to +5 pixels
                                float offsetY = (rand() % 21 - 10) * 0.5f;
                                lootObj->setPositionPixels(tx * ts + ts * 0.5f - 8.0f + offsetX, 
                                                          ty * ts + ts * 0.5f - 8.0f + offsetY);
                                lootObj->setCollectible(true);
                                lootObj->setSpawnTicks(SDL_GetTicks());
                                lootObj->setMagnetDelaySeconds(1.5f);
                                
                                // Add the loot data to the object
                                lootObj->addLoot(loot);
                                
                                // Set texture
                                if (Texture* t = assetManager->getTexture(lootObj->getTexture() ? "" : "assets/Textures/Objects/chest_unopened.png")) {
                                    lootObj->setTexture(t);
                                }
                                
                                world->addObject(std::move(lootObj));
                            }
                        }
                    }

                    // Rate-limit goblin death SFX so multiple goblins dying together don't spam
                    if (audioManager && std::string(enemyPtr->getDisplayName()) == std::string("Goblin")) {
                        static Uint32 lastGoblinDeathSoundTicks = 0;
                        Uint32 nowTicks = SDL_GetTicks();
                        const Uint32 cooldownMs = 200; // play at most once per 200ms
                        if (nowTicks - lastGoblinDeathSoundTicks >= cooldownMs) {
                            int prevSV = audioManager->getSoundVolume();
                            audioManager->setSoundVolume(std::max(0, prevSV * 10 / 100));
                            audioManager->playSound("goblin_death");
                            audioManager->setSoundVolume(prevSV);
                            lastGoblinDeathSoundTicks = nowTicks;
                        }
                    }
                    enemyPtr->markLootDropped();
                }
                // Despawn dead enemies after 60 seconds
                if (enemyPtr->isDead()) {
                    if (enemyPtr->isDespawnReady(SDL_GetTicks(), 60000)) {
                        // Replace with nullptr; cleanup after loop
                        enemyPtr.reset();
                    }
                }
            }
            // Remove nulls
            enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](const std::unique_ptr<Enemy>& e){ return !e; }), enemies.end());
        }
    }

    // Allow respawn via UI popup if player is dead (handled in render)
    
    // Update UI
    if (uiSystem) {
        uiSystem->update(deltaTime);
    }
    
    // Update audio
    if (audioManager) {
        audioManager->update(deltaTime);
    }
    
    // Update input states (do this last so pressed states are preserved for the current frame)
    inputManager->update();

    // Periodically autosave basic player state (e.g., every ~5 seconds via accumulator of frame time)
    static float autosaveTimer = 0.0f;
    autosaveTimer += deltaTime;
    if (autosaveTimer >= 5.0f && database && player) {
        autosaveTimer = 0.0f;
        // Save to the default user (id 1) if exists
        auto user = database->getUserById(1);
        if (user) {
            PlayerSave s = player->makeSaveState();
            database->savePlayerState(user->userId, s);
        }
    }
}

void Game::render() {
    // Clear screen
    SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
    SDL_RenderClear(sdlRenderer);
    
    // Set camera (center on player using actual renderer output size and zoom)
    if (player) {
        int playerX = static_cast<int>(player->getX());
        int playerY = static_cast<int>(player->getY());
        int outW = 0, outH = 0; SDL_GetRendererOutputSize(sdlRenderer, &outW, &outH);
        if (outW <= 0) { outW = WINDOW_WIDTH; outH = WINDOW_HEIGHT; }
        float zoom = renderer->getZoom();
        int cameraX = static_cast<int>(playerX - (outW / (2.0f * zoom)));
        int cameraY = static_cast<int>(playerY - (outH / (2.0f * zoom)));
        renderer->setCamera(cameraX, cameraY);
        if (world) {
            world->updateVisibleChunks(player->getX(), player->getY());
            world->updateVisibility(player->getX(), player->getY());
        }
    }
    
    // Login screen overlay when active
    if (loginScreenActive) {
        // Dim background
        int outW=0,outH=0; SDL_GetRendererOutputSize(sdlRenderer, &outW, &outH);
        SDL_SetRenderDrawBlendMode(sdlRenderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 200);
        SDL_Rect dim{0,0,(outW>0?outW:WINDOW_WIDTH),(outH>0?outH:WINDOW_HEIGHT)};
        SDL_RenderFillRect(sdlRenderer, &dim);
        // Panel
        int panelW = 520, panelH = 320;
        SDL_Rect panel{ dim.w/2 - panelW/2, dim.h/2 - panelH/2, panelW, panelH };
        SDL_SetRenderDrawColor(sdlRenderer, 35,35,48,245);
        SDL_RenderFillRect(sdlRenderer, &panel);
        SDL_SetRenderDrawColor(sdlRenderer, 210,210,230,255);
        SDL_RenderDrawRect(sdlRenderer, &panel);
        if (uiSystem) {
            uiSystem->renderTextCentered("PixLegends - Login", dim.w/2, panel.y + 36);
            // Fake input fields: show current username/password (masked)
            std::string userDisplay = "User: " + (loginUsername.empty()? std::string("<click and type>") : loginUsername);
            std::string passMask(loginPassword.size(), '*');
            std::string passDisplay = "Pass: " + (loginPassword.empty()? std::string("<click and type>") : passMask);
            uiSystem->renderText(userDisplay, panel.x + 40, panel.y + 90);
            uiSystem->renderText(passDisplay, panel.x + 40, panel.y + 140);
            // Visual caret indicator
            if (loginActiveField == LoginField::Username) {
                uiSystem->renderText("<", panel.x + 30, panel.y + 90, SDL_Color{180,220,255,255});
            } else if (loginActiveField == LoginField::Password) {
                uiSystem->renderText("<", panel.x + 30, panel.y + 140, SDL_Color{180,220,255,255});
            }
            // Remember checkbox
            SDL_Rect chk{ panel.x + 40, panel.y + 180, 20, 20 };
            SDL_SetRenderDrawColor(sdlRenderer, 255,255,255,255);
            SDL_RenderDrawRect(sdlRenderer, &chk);
            if (loginRemember) {
                SDL_SetRenderDrawColor(sdlRenderer, 90,160,90,255);
                SDL_RenderFillRect(sdlRenderer, &chk);
            }
            uiSystem->renderText("Remember me", chk.x + 28, chk.y + 2);
            if (!loginError.empty()) uiSystem->renderText(loginError, panel.x + 40, panel.y + 180, SDL_Color{255,120,120,255});
            // Buttons: Login and Register
            SDL_Rect btnLogin{ panel.x + 40, panel.y + panelH - 70, 180, 44 };
            SDL_Rect btnRegister{ panel.x + 240, panel.y + panelH - 70, 180, 44 };
            SDL_SetRenderDrawColor(sdlRenderer, 90,160,90,255); SDL_RenderFillRect(sdlRenderer, &btnLogin);
            SDL_SetRenderDrawColor(sdlRenderer, 255,255,255,255); SDL_RenderDrawRect(sdlRenderer, &btnLogin);
            uiSystem->renderText("Login", btnLogin.x + 12, btnLogin.y + 12);
            SDL_SetRenderDrawColor(sdlRenderer, 80,120,170,255); SDL_RenderFillRect(sdlRenderer, &btnRegister);
            SDL_SetRenderDrawColor(sdlRenderer, 255,255,255,255); SDL_RenderDrawRect(sdlRenderer, &btnRegister);
            uiSystem->renderText("Register", btnRegister.x + 12, btnRegister.y + 12);
        }
        // Present and early return (do not render world)
        SDL_RenderPresent(sdlRenderer);
        return;
    }

    // Render world
    if (world) {
        // Keep rendering the world even while the anvil UI is open so the background remains visible
        world->render(renderer.get());
    }
    
    // Render player
    if (player) {
        player->render(renderer.get());
        player->renderProjectiles(renderer.get());
        
        // Render spell effects
        if (player->getSpellSystem()) {
            player->getSpellSystem()->render(renderer.get());
            
            // Render channeling indicators (range and target)
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            player->getSpellSystem()->renderIndicators(renderer.get(), mouseX, mouseY);
        }
    }
    
    // Render UI
    if (uiSystem) {
        // Minimap first, so HUD frame overlays on top of it
        if (world && player && assetManager) {
            Texture* uiFrame = assetManager->getTexture("assets/Textures/UI/hp_mana_bar_ui.png");
            if (uiFrame) {
                // HUD frame position and size (matches UISystem::renderPlayerStats)
                const int frameX = 10;
                const int frameY = 10;
                const int texW = uiFrame->getWidth();
                const int texH = uiFrame->getHeight();

                // Define the inner UI square in the HUD art using base pixel measurements
                // Base texture reference dimensions (approximate): 399x108
                const float BASE_W = 399.0f;
                const float BASE_H = 108.0f;
                const float scaleX = texW / BASE_W;
                const float scaleY = texH / BASE_H;

                // Inner square top-left and size in base pixels (estimated to match the frame art)
                const float SQ_LEFT = 8.0f;   // px from left
                const float SQ_TOP  = 8.0f;   // px from top
                const float SQ_SIZE = 94.0f;  // square side length

                const int squareX = frameX + static_cast<int>(std::round(SQ_LEFT * scaleX));
                const int squareY = frameY + static_cast<int>(std::round(SQ_TOP * scaleY));
                const int squareW = static_cast<int>(std::round(SQ_SIZE * scaleX));
                const int squareH = static_cast<int>(std::round(SQ_SIZE * scaleY));

                // Fit minimap centered inside the square with a small padding
                const int pad = static_cast<int>(std::round(2.0f * std::min(scaleX, scaleY)));
                const int mmSize = std::max(1, std::min(squareW, squareH) - pad * 2);
                const int mmX = squareX + (squareW - mmSize) / 2;
                const int mmY = squareY + (squareH - mmSize) / 2;

                // Stretch request: right +16, down +0, left +12, up +12 (relative to current rect)
                const int addRight = 16;
                const int addDown = 10; // stretch down 10px
                const int addLeft = 12;
                const int addUp = 12;

                const int mmW = mmSize + addLeft + addRight;
                const int mmH = mmSize + addUp + addDown;
                const int mmXAdj = mmX - addLeft;
                const int mmYAdj = mmY - addUp;

                world->renderMinimap(renderer.get(), mmXAdj, mmYAdj, mmW, mmH, player->getX(), player->getY());
            }
        }
        // Draw HUD frame and stats above the minimap
        uiSystem->renderPlayerStats(player.get());
        
        // Render skill bar
        uiSystem->renderSkillBar(player.get());
        
        // Render channeling bar above skill bar
        if (player->getSpellSystem()) {
            player->getSpellSystem()->renderChannelBar(renderer.get());
        }
        
        // Render spell book if open
        if (uiSystem->isSpellBookOpen()) {
            int mouseX = inputManager->getMouseX();
            int mouseY = inputManager->getMouseY();
            bool mouseClicked = inputManager->isMouseButtonPressed(SDL_BUTTON_LEFT);
            uiSystem->renderSpellBook(player.get(), mouseX, mouseY, mouseClicked);
        }
        uiSystem->renderDebugInfo(player.get());
        // Render FPS counter
        uiSystem->renderFPSCounter(currentFPS, averageFPS, frameTime);
        
        // Render death popup with respawn button if dead (still allow on top of world)
        if (player && player->isDead()) {
            // Wait for death animation to finish, then fade in popup
            static float deathPopupFade = 0.0f;
            if (player->isDeathAnimationFinished()) {
                deathPopupFade = std::min(1.0f, deathPopupFade + 0.05f);
            } else {
                deathPopupFade = 0.0f;
            }
            bool clickedRespawn = false;
            uiSystem->renderDeathPopup(clickedRespawn, deathPopupFade);
            if (clickedRespawn) {
                // Reset world enemies and player
                if (world) {
                    auto& enemies = world->getEnemies();
                    for (auto& e : enemies) {
                        if (e) e->resetToSpawn();
                    }
                }
                player->respawn(player->getSpawnX(), player->getSpawnY());
                // After respawn, enforce minion cap by player level to avoid sudden overpopulation
                if (world && player) {
                    int level = player->getLevel();
                    const int caps[10] = {5,8,11,14,17,20,23,26,28,30};
                    int maxGoblins = (level <= 0 ? 5 : (level >= 10 ? 30 : caps[level-1]));
                    // Gather live goblin indices
                    auto& enemies = world->getEnemies();
                    int alive = 0;
                    for (const auto& e : enemies) {
                        if (e && std::string(e->getDisplayName()) == std::string("Goblin") && !e->isDead()) alive++;
                    }
                    if (alive > maxGoblins) {
                        // Despawn extras starting from farthest from player
                        struct GRef { size_t idx; float dist2; };
                        std::vector<GRef> gobRefs; gobRefs.reserve(enemies.size());
                        float pcx = player->getX(); float pcy = player->getY();
                        for (size_t i = 0; i < enemies.size(); ++i) {
                            auto& e = enemies[i];
                            if (!e || std::string(e->getDisplayName()) != std::string("Goblin") || e->isDead()) continue;
                            float dx = e->getX() - pcx; float dy = e->getY() - pcy; gobRefs.push_back({i, dx*dx + dy*dy});
                        }
                        std::sort(gobRefs.begin(), gobRefs.end(), [](const GRef& a, const GRef& b){ return a.dist2 > b.dist2; });
                        int toRemove = alive - maxGoblins;
                        for (int k = 0; k < toRemove && k < static_cast<int>(gobRefs.size()); ++k) {
                            enemies[gobRefs[k].idx].reset();
                        }
                        enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](const std::unique_ptr<Enemy>& e){ return !e; }), enemies.end());
                    }
                }
                deathPopupFade = 0.0f;
            }
        }

        // Boss/Pack health bar while engaged (hide during modal anvil). Show for Magic, Elite, and Bosses.
        if (world && player && !anvilOpen) {
            Enemy* engagedEnemy = nullptr;
            bool isBossFight = false;
            
            // Check for active boss first (bosses take priority over regular enemies)
            if (world->hasBoss() && world->getCurrentBoss() && !world->getCurrentBoss()->isDead()) {
                float pcx = player->getX() + player->getWidth() * 0.5f;
                float pcy = player->getY() + player->getHeight() * 0.5f;
                Boss* boss = world->getCurrentBoss();
                bool engaged = boss->getIsAggroed() || boss->isWithinAttackRange(pcx, pcy);
                if (engaged) {
                    engagedEnemy = boss;
                    isBossFight = true;
                }
            }
            
            // If no boss is engaged, check regular enemies
            if (!engagedEnemy) {
                auto& enemies = world->getEnemies();
                for (auto& e : enemies) {
                    if (!e || e->isDead()) continue;
                    float pcx = player->getX() + player->getWidth() * 0.5f;
                    float pcy = player->getY() + player->getHeight() * 0.5f;
                    bool engaged = e->getIsAggroed() || e->isWithinAttackRange(pcx, pcy);
                    // Only Magic/Elite show top HP bar and gate boss music
                    if (engaged && !(e->getPackRarity() == PackRarity::Elite || e->getPackRarity() == PackRarity::Magic)) engaged = false;
                    if (!engaged) continue;
                    engagedEnemy = e.get();
                    if (std::string(engagedEnemy->getDisplayName()) == "Demon") break;
                }
            }
            
            if (engagedEnemy) {
                int outW = 0, outH = 0;
                SDL_GetRendererOutputSize(sdlRenderer, &outW, &outH);
                uiSystem->renderBossHealthBar(engagedEnemy->getDisplayName(), engagedEnemy->getHealth(), engagedEnemy->getMaxHealth(), (outW > 0 ? outW : WINDOW_WIDTH));
                // Start boss music if available (bosses and elites)
                if ((isBossFight || engagedEnemy->getPackRarity() == PackRarity::Elite) && audioManager && audioManager->hasMusic("boss_music") && currentMusicTrack != "boss_music") { 
                    audioManager->fadeToMusic("boss_music", 400, 300); 
                    currentMusicTrack = "boss_music"; 
                }
                bossWasDead = false;
                bossMusicHoldTimerSec = 0.0f;
                bossFadeOutPending = false;
            } else {
                // Boss died or disengaged: switch back to main theme if present; otherwise stop music
                bool bossJustDied = bossWasDead; // simplified hold logic without specific enemy
                if (bossJustDied) {
                    bossWasDead = true;
                    bossMusicHoldTimerSec = 3.0f; // hold boss music for 3 seconds
                    bossFadeOutPending = true;
                }
                if (bossWasDead) {
                    if (bossMusicHoldTimerSec > 0.0f) {
                        // Use fixed timestep duration since we're inside update(TARGET_FRAME_TIME)
                        bossMusicHoldTimerSec -= TARGET_FRAME_TIME;
                    } else if (bossFadeOutPending) {
                        // Start a 3s fade-out of boss music, then fade main theme in
                        if (audioManager) {
                            audioManager->fadeToMusic("main_theme", 3000, 400);
                            currentMusicTrack = "main_theme";
                        }
                        bossFadeOutPending = false;
                    }
                } else {
                    // Not dead: handle disengage (no special hold)
                if (audioManager) {
                        if (audioManager->hasMusic(backgroundMusicName)) {
                            if (currentMusicTrack != backgroundMusicName) {
                                audioManager->fadeToMusic(backgroundMusicName, 500, 400);
                                currentMusicTrack = backgroundMusicName;
                            }
                        } else {
                            if (!currentMusicTrack.empty()) {
                                audioManager->stopMusic();
                                currentMusicTrack.clear();
            }
        }
                }
            }
            }
        }

        // Debug draw melee, enemy, and player hitboxes (F3 toggle)
        if (getDebugHitboxes() && player && !anvilOpen) {
            SDL_SetRenderDrawBlendMode(sdlRenderer, SDL_BLENDMODE_BLEND);
            int camX = 0, camY = 0; renderer->getCamera(camX, camY);
            float z = renderer->getZoom();

            // Player melee hitbox (world -> screen). World/enemy rendering is not zoom-scaled,
            // so keep debug overlays in the same coordinate space (subtract camera only).
            SDL_Rect meleeW = player->getMeleeHitbox();
            auto scaleRect = [camX, camY, z](const SDL_Rect& r) -> SDL_Rect {
                float x1 = (r.x - camX) * z;
                float y1 = (r.y - camY) * z;
                float x2 = (r.x + r.w - camX) * z;
                float y2 = (r.y + r.h - camY) * z;
                int ix1 = static_cast<int>(std::floor(x1));
                int iy1 = static_cast<int>(std::floor(y1));
                int ix2 = static_cast<int>(std::floor(x2));
                int iy2 = static_cast<int>(std::floor(y2));
                SDL_Rect out{ ix1, iy1, std::max(1, ix2 - ix1), std::max(1, iy2 - iy1) };
                return out;
            };
            SDL_Rect melee = scaleRect(meleeW);
            SDL_SetRenderDrawColor(sdlRenderer, 0, 255, 0, 160);
            if (melee.w > 0 && melee.h > 0) SDL_RenderDrawRect(sdlRenderer, &melee);

            // Enemy hitboxes
            if (world) {
                auto& enemies = world->getEnemies();
                for (auto& e : enemies) {
                    if (!e) continue;
                    SDL_Rect erW = e->getCollisionRect();
                    SDL_Rect er = scaleRect(erW);
                    SDL_SetRenderDrawColor(sdlRenderer, 255, 0, 0, 160);
                    SDL_RenderDrawRect(sdlRenderer, &er);
                }
            }

            // Player collision rect (world -> screen)
            SDL_Rect prW = player->getCollisionRect();
            SDL_Rect pr = scaleRect(prW);
            SDL_SetRenderDrawColor(sdlRenderer, 0, 200, 255, 160);
            SDL_RenderDrawRect(sdlRenderer, &pr);
        }
        
        // Render interaction prompt
        if (player && !anvilOpen) {
            std::string interactionPrompt = player->getCurrentInteractionPrompt();
            if (!interactionPrompt.empty()) {
                // Render the prompt at the center of the screen, slightly above the bottom
                int promptX = WINDOW_WIDTH / 2;
                int promptY = WINDOW_HEIGHT - 100;
                uiSystem->renderInteractionPrompt(interactionPrompt, promptX, promptY);
            }
            
            // Render loot notification if any
            std::string lootNotification = player->getLastLootNotification();
            if (!lootNotification.empty()) {
                // Render the notification at the center of the screen, above the interaction prompt
                int notificationX = WINDOW_WIDTH / 2;
                int notificationY = WINDOW_HEIGHT - 150;
                uiSystem->renderLootNotification(lootNotification, notificationX, notificationY);
                
                // Clear the notification after a short time (for now, just clear it immediately)
                // In a full implementation, you'd want to add a timer
                static int notificationTimer = 0;
                notificationTimer++;
                if (notificationTimer > 180) { // 3 seconds at 60 FPS
                    player->clearLootNotification();
                    notificationTimer = 0;
                }
            }
        }
    }
    // Enhanced inventory overlay – does not pause the world. Process input first
    if (inventoryOpen && uiSystem && player) {
        UISystem::InventoryHit ih = uiSystem->renderEnhancedInventory(player.get(), inventoryOpen, anvilOpen, this, equipmentOpen);
        // Handle UI interactions
        if (ih.clickedClose) {
            inventoryOpen = false;
        }
        
        // Handle inventory panel dragging
        static bool draggingInventory = false;
        if (ih.titleBarClicked && !draggingInventory) {
            draggingInventory = true;
        }
        if (draggingInventory) {
            int mx, my;
            SDL_GetMouseState(&mx, &my);
            if (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
                // Still dragging - update position
                setInventoryPos(mx - ih.dragOffsetX, my - ih.dragOffsetY);
            } else {
                // Released mouse - stop dragging
                draggingInventory = false;
            }
        }
        
        // Handle item slot clicks and dragging
        ItemSystem* itemSystem = player->getItemSystem();
        if (itemSystem) {
            if (ih.clickedItemSlot >= 0 && !processingEquipmentEvent) {
                const auto& itemInventory = itemSystem->getItemInventory();
                if (ih.clickedItemSlot < itemInventory.size() && !itemInventory[ih.clickedItemSlot].isEmpty()) {
                    Item* item = itemInventory[ih.clickedItemSlot].item;
                    
                    // Set flag to prevent double processing
                    processingEquipmentEvent = true;
                    
                    if (!ih.rightClicked) {
                        // Left click - check if anvil is open for dragging, otherwise equip
                        if (anvilOpen && item->type == ItemType::EQUIPMENT) {
                            // Start dragging to anvil
                            draggingFromInventory = true;
                            draggingPayload = item->id;
                            // Store the specific item instance being dragged
                            anvilTargetItem = item;
                            anvilItemSource = AnvilItemSource::INVENTORY_ITEM;
                        } else {
                            // Regular equip behavior
                            Item* item = itemInventory[ih.clickedItemSlot].item;
                            int equipSlot = static_cast<int>(item->equipmentType);
                            
                            if (itemSystem->equipItem(ih.clickedItemSlot)) {
                                // Sync the Player equipment array with the newly equipped item
                                const auto& equipSlots = itemSystem->getEquipmentSlots();
                                if (equipSlot >= 0 && equipSlot < equipSlots.size() && !equipSlots[equipSlot].isEmpty()) {
                                    player->syncEquipmentFromItem(static_cast<Player::EquipmentSlot>(equipSlot), 
                                                                 equipSlots[equipSlot].item);
                                }
                            }
                        }
                    } else if (anvilOpen && item->type == ItemType::EQUIPMENT) {
                        // Right click - automatically select anvil slot for this equipment type AND set target item
                        if (item->id.find("sword") != std::string::npos) anvilSelectedSlot = 3;
                        else if (item->id.find("helmet") != std::string::npos) anvilSelectedSlot = 1;
                        else if (item->id.find("ring") != std::string::npos) anvilSelectedSlot = 0;
                        else if (item->id.find("necklace") != std::string::npos) anvilSelectedSlot = 2;
                        else if (item->id.find("chestpeice") != std::string::npos) anvilSelectedSlot = 4;
                        else if (item->id.find("bow") != std::string::npos) anvilSelectedSlot = 5;
                        else if (item->id.find("gloves") != std::string::npos) anvilSelectedSlot = 6;
                        else if (item->id.find("waist") != std::string::npos) anvilSelectedSlot = 7;
                        else if (item->id.find("boots") != std::string::npos) anvilSelectedSlot = 8;
                        
                        // Set the target item for anvil operations
                        anvilTargetItem = item;
                        anvilItemSource = AnvilItemSource::INVENTORY_ITEM;
                    } else if (!anvilOpen && item->type == ItemType::EQUIPMENT) {
                        // Right click - ordered swap: unequip first, then equip
                        int equipSlot = static_cast<int>(item->equipmentType);
                        if (equipSlot >= 0 && equipSlot < 9) {
                            const auto& equipSlots = itemSystem->getEquipmentSlots();
                            
                            // Step 1: If there's an item equipped in that slot, unequip it first
                            if (equipSlot < equipSlots.size() && !equipSlots[equipSlot].isEmpty()) {
                                if (itemSystem->unequipItem(equipSlot)) {
                                    // Also clear the Player equipment array to stay synchronized
                                    player->clearEquipmentSlot(static_cast<Player::EquipmentSlot>(equipSlot));
                                }
                            }
                            
                            // Step 2: Now equip the new item
                            if (itemSystem->equipItem(ih.clickedItemSlot)) {
                                // Sync the Player equipment array with the newly equipped item
                                const auto& equipSlots = itemSystem->getEquipmentSlots();
                                if (equipSlot < equipSlots.size() && !equipSlots[equipSlot].isEmpty()) {
                                    player->syncEquipmentFromItem(static_cast<Player::EquipmentSlot>(equipSlot), 
                                                                 equipSlots[equipSlot].item);
                                }
                            }
                        }
                    }
                    // Right click also shows tooltip (handled by UI)
                    
                    // Clear the processing flag
                    processingEquipmentEvent = false;
                }
            }
            
            if (ih.clickedScrollSlot >= 0) {
                const auto& scrollInventory = itemSystem->getScrollInventory();
                if (ih.clickedScrollSlot < scrollInventory.size() && !scrollInventory[ih.clickedScrollSlot].isEmpty()) {
                    Item* scroll = scrollInventory[ih.clickedScrollSlot].item;
                    
                    if (!ih.rightClicked && anvilOpen && scroll->type == ItemType::SCROLL) {
                        // Start dragging scroll to anvil
                        draggingFromInventory = true;
                        draggingPayload = scroll->id;
                    } else if (ih.rightClicked && anvilOpen && scroll->type == ItemType::SCROLL) {
                        // Right click - automatically stage scroll in anvil and consume it
                        anvilStagedScrollKey = scroll->id;
                        // Consume the scroll from player's inventory
                        if (scroll->id == "upgrade_scroll") {
                            player->consumeUpgradeScroll();
                        } else {
                            player->consumeElementScroll(scroll->id);
                        }
                    }
                }
            }
        }
    }
    
    // Equipment/Stats UI overlay
    if (equipmentOpen && uiSystem && player) {
        UISystem::EquipmentHit eh = uiSystem->renderEquipmentUI(player.get(), equipmentOpen, anvilOpen, this, inventoryOpen);
        // Handle UI interactions
        if (eh.clickedClose) {
            equipmentOpen = false;
        }
        
        // Handle equipment panel dragging
        static bool draggingEquipment = false;
        if (eh.titleBarClicked && !draggingEquipment) {
            draggingEquipment = true;
        }
        if (draggingEquipment) {
            int mx, my;
            SDL_GetMouseState(&mx, &my);
            if (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
                // Still dragging - update position
                setEquipmentPos(mx - eh.dragOffsetX, my - eh.dragOffsetY);
            } else {
                // Released mouse - stop dragging
                draggingEquipment = false;
            }
        }
        
        // Handle equipment slot clicks and dragging
        ItemSystem* itemSystem = player->getItemSystem();
        if (itemSystem && eh.clickedEquipSlot >= 0 && eh.clickedEquipSlot < 9) {
            const auto& equipSlots = itemSystem->getEquipmentSlots();
            if (eh.clickedEquipSlot < equipSlots.size() && !equipSlots[eh.clickedEquipSlot].isEmpty()) {
                Item* equippedItem = equipSlots[eh.clickedEquipSlot].item;
                if (!eh.rightClicked) {
                    // Left click - check if anvil is open for dragging, otherwise unequip
                    if (anvilOpen) {
                        // Start dragging to anvil
                        draggingFromInventory = true;
                        draggingPayload = equippedItem->id;
                        // Store the specific item instance being dragged
                        anvilTargetItem = equippedItem;
                        anvilItemSource = AnvilItemSource::EQUIPPED_SLOT;
                        // Also set the anvil selected slot to match this equipment type
                        anvilSelectedSlot = eh.clickedEquipSlot;
                    } else {
                        // Regular unequip behavior - move to inventory using ItemSystem
                        if (itemSystem->unequipItem(eh.clickedEquipSlot)) {
                            // Also clear the Player equipment array to stay synchronized
                            player->clearEquipmentSlot(static_cast<Player::EquipmentSlot>(eh.clickedEquipSlot));
                        }
                    }
                } else if (anvilOpen) {
                    // Right click - automatically select this equipment slot in anvil and set target item
                    anvilSelectedSlot = eh.clickedEquipSlot;
                    anvilTargetItem = equippedItem;
                    anvilItemSource = AnvilItemSource::EQUIPPED_SLOT;
                }
                // Right click also shows tooltip (handled by UI)
            }
        }
    }

    // Magic Anvil overlay after processing inventory so dragging payload is known
    if (anvilOpen && uiSystem && player) {
        int outW=0,outH=0; SDL_GetRendererOutputSize(sdlRenderer,&outW,&outH); if (outW<=0){outW=WINDOW_WIDTH;outH=WINDOW_HEIGHT;}
        int mx=0,my=0; SDL_GetMouseState(&mx,&my);
        Uint32 ms = SDL_GetMouseState(nullptr,nullptr);
        bool mouseDown = (ms & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
        UISystem::AnvilHit hit;
        static std::string currentScrollPreview;
        // Provide external payload while dragging from inventory (enables drop detection on release)
        std::string external = (draggingFromInventory ? draggingPayload : std::string());
        uiSystem->renderMagicAnvil(player.get(), outW, outH, mx, my, mouseDown, hit, external, anvilSelectedSlot, anvilStagedScrollKey, this);
        if (hit.clickedSlot>=0) anvilSelectedSlot = hit.clickedSlot;
        // Apply via button or drag-drop
        static float upgradeFlashTimer = 0.0f; static bool lastUpgradeSuccess = false;
        auto chanceForNext = [](int currentPlus) -> float {
            static const float table[31] = {
                0.0f, 100.0f, 95.0f, 90.0f, 80.0f, 75.0f, 70.0f, 65.0f, 55.0f, 50.0f,
                45.0f, 40.0f, 35.0f, 30.0f, 28.0f, 25.0f, 20.0f, 18.0f, 15.0f, 13.0f,
                12.0f, 10.0f, 8.0f, 7.0f, 6.0f, 5.0f, 4.5f, 4.0f, 3.5f, 3.0f, 2.5f
            }; // index is next level; we pass current+1 later
            int next = currentPlus + 1; if (next < 1) next = 1; if (next > 30) next = 30; return table[next];
        };
        static std::mt19937 rng(static_cast<unsigned int>(SDL_GetTicks()));
        // Clicking upgrade button applies staged scroll (upgrade or element)
        if (hit.clickedSideUpgrade && !anvilStagedScrollKey.empty()) {
            anvilUpgradeAnimT = 0.0001f; // start sweep
            if (anvilStagedScrollKey == "upgrade_scroll") {
                // Determine which item to upgrade
                Item* targetItem = anvilTargetItem;
                int currentPlusLevel = 0;
                
                if (targetItem) {
                    // Upgrade specific item instance
                    currentPlusLevel = targetItem->plusLevel;
                } else {
                    // Fallback to equipment slot method
                    const auto& eq = player->getEquipment(static_cast<Player::EquipmentSlot>(anvilSelectedSlot));
                    currentPlusLevel = eq.plusLevel;
                    targetItem = nullptr;
                }
                
                if (currentPlusLevel < 30) {
                    std::uniform_real_distribution<float> dist(0.0f, 100.0f);
                    float roll = dist(rng);
                    float chance = chanceForNext(currentPlusLevel);
                    if (roll <= chance) {
                        if (targetItem) {
                            player->upgradeSpecificItem(targetItem, 1);
                        } else {
                            player->upgradeEquipment(static_cast<Player::EquipmentSlot>(anvilSelectedSlot), 1);
                        }
                        lastUpgradeSuccess = true;
                    } else {
                        lastUpgradeSuccess = false;
                    }
                    anvilLastSuccess = lastUpgradeSuccess;
                    // Do NOT reveal immediately; start suspense sweep and reveal after it completes
                    anvilStagedScrollKey.clear();
                    Object::setMagicAnvilPulse(3.2f);
                    if (audioManager) audioManager->playSound("upgrade_sound");
                } else { lastUpgradeSuccess = false; upgradeFlashTimer = 0.8f; }
            } else {
                // Enchantment
                if (anvilTargetItem) {
                    player->enchantSpecificItem(anvilTargetItem, anvilStagedScrollKey, 1);
                } else {
                    player->enchantEquipment(static_cast<Player::EquipmentSlot>(anvilSelectedSlot), anvilStagedScrollKey, 1);
                }
                lastUpgradeSuccess = true; anvilLastSuccess = true; anvilStagedScrollKey.clear();
                Object::setMagicAnvilPulse(3.2f);
                if (audioManager) audioManager->playSound("upgrade_sound");
            }
        }
        // Handle dropped items from enhanced inventory system
        if (hit.droppedItem && draggingFromInventory && !draggingPayload.empty()) {
            // An equipment item was dropped into the item slot
            // Map the item ID to an equipment slot if possible
            if (draggingPayload.find("sword") != std::string::npos) anvilSelectedSlot = 3;
            else if (draggingPayload.find("helmet") != std::string::npos) anvilSelectedSlot = 1;
            else if (draggingPayload.find("ring") != std::string::npos) anvilSelectedSlot = 0;
            else if (draggingPayload.find("necklace") != std::string::npos) anvilSelectedSlot = 2;
            else if (draggingPayload.find("chestpeice") != std::string::npos) anvilSelectedSlot = 4;
            else if (draggingPayload.find("bow") != std::string::npos) anvilSelectedSlot = 5;
            else if (draggingPayload.find("gloves") != std::string::npos) anvilSelectedSlot = 6;
            else if (draggingPayload.find("waist") != std::string::npos) anvilSelectedSlot = 7;
            else if (draggingPayload.find("boots") != std::string::npos) anvilSelectedSlot = 8;
            
            draggingFromInventory = false;
            draggingPayload.clear();
            // Keep anvilTargetItem set since the item was successfully dropped
        }
        
        // Dropping an element scroll into scroll slot stages it; clicking upgrade applies if element staged
        if (hit.droppedScroll) {
            anvilStagedScrollKey = hit.droppedScrollKey;
            
            // Clear dragging state if this was from inventory
            if (draggingFromInventory) {
                draggingFromInventory = false;
                draggingPayload.clear();
                // Don't clear anvilTargetItem here since scroll was successfully dropped
            }
        }
        if (!hit.clickedElement.empty()) {
            std::string elem = hit.clickedElement;
            if (!anvilStagedScrollKey.empty() && anvilStagedScrollKey != elem) {
                lastUpgradeSuccess = false; upgradeFlashTimer = 0.8f;
            } else {
                // Use specific item if available, otherwise fall back to slot
                if (anvilTargetItem) {
                    player->enchantSpecificItem(anvilTargetItem, elem, 1);
                } else {
                    player->enchantEquipment(static_cast<Player::EquipmentSlot>(anvilSelectedSlot), elem, 1);
                }
                lastUpgradeSuccess = true; upgradeFlashTimer = 0.8f; anvilStagedScrollKey.clear();
                Object::setMagicAnvilPulse(3.2f);
            }
        }
        if (hit.droppedScroll) { currentScrollPreview = hit.droppedScrollKey; }
        else if (!mouseDown) { currentScrollPreview = anvilStagedScrollKey; }
        
        // Handle anvil panel dragging
        static bool draggingAnvil = false;
        if (hit.titleBarClicked && !draggingAnvil) {
            draggingAnvil = true;
        }
        if (draggingAnvil) {
            if (mouseDown) {
                // Still dragging - update position
                setAnvilPos(mx - hit.dragOffsetX, my - hit.dragOffsetY);
            } else {
                // Released mouse - stop dragging
                draggingAnvil = false;
            }
        }
        
        // Clear dragging state if mouse was released and nothing was dropped
        if (!mouseDown && draggingFromInventory && !hit.droppedItem && !hit.droppedScroll) {
            draggingFromInventory = false;
            draggingPayload.clear();
            anvilTargetItem = nullptr;
            anvilItemSource = AnvilItemSource::NONE;
        }
        // Close via Escape handled in event loop; remove close button logic
    }

    // Upgrade success/fail flash overlay near anvil side panel, if any
    static float upgradeFlashTimer = 0.0f; static bool lastUpgradeSuccess = false; // references above
        if (anvilOpen && assetManager) {
        // Sweep loading bar inside indicator: alternate succeed/failed image while sweeping
        if (anvilUpgradeAnimT > 0.0f) {
            anvilUpgradeAnimT = std::min(1.0f, anvilUpgradeAnimT + 2.0f * TARGET_FRAME_TIME); // ~0.5s sweep
            
            // Use current anvil UI positioning - match exactly with UISystem indicatorRect
            Texture* anvilBG = assetManager->getTexture("assets/Textures/UI/AnvilUI.png");
            int outW=0,outH=0; SDL_GetRendererOutputSize(sdlRenderer,&outW,&outH); if (outW<=0){outW=WINDOW_WIDTH;outH=WINDOW_HEIGHT;}
            int pw = anvilBG ? anvilBG->getWidth() : 200;
            int ph = anvilBG ? anvilBG->getHeight() : 200;
            int anvilX, anvilY;
            
            // Use same positioning logic as anvil UI
            if (anvilPosX >= 0 && anvilPosY >= 0) {
                anvilX = anvilPosX;
                anvilY = anvilPosY;
            } else {
                anvilX = outW / 4 - pw / 2;  // Default anvil positioning
                anvilY = (outH - ph) / 2;
            }
            
            // Match indicator rect from UISystem.cpp exactly
            SDL_Rect indicatorRect = {
                anvilX + pw / 2 - 55,  // Center horizontally
                anvilY + ph - 50,      // Near bottom
                110, 20
            };
            
            // Alternate texture as it fills
            bool alt = (static_cast<int>(anvilUpgradeAnimT * 10.0f) % 2) == 0;
            Texture* tAlt = assetManager->getTexture(alt ? "assets/Textures/UI/upgrade_succeed.png" : "assets/Textures/UI/upgrade_failed.png");
            if (tAlt) {
                SDL_Rect src{0,0, static_cast<int>(tAlt->getWidth()*anvilUpgradeAnimT), tAlt->getHeight()};
                SDL_Rect dst{ indicatorRect.x, indicatorRect.y, static_cast<int>(indicatorRect.w*anvilUpgradeAnimT), indicatorRect.h };
                SDL_RenderCopy(sdlRenderer, tAlt->getTexture(), &src, &dst);
            }
            if (anvilUpgradeAnimT >= 1.0f) { anvilUpgradeAnimT = 0.0f; anvilResultFlashTimer = 1.2f; }
        }
    }
    if (anvilOpen && assetManager) {
        // Final reveal steady display
        if (anvilResultFlashTimer > 0.0f) {
            anvilResultFlashTimer = std::max(0.0f, anvilResultFlashTimer - TARGET_FRAME_TIME);
            Texture* t = assetManager->getTexture(anvilLastSuccess ? "assets/Textures/UI/upgrade_succeed.png" : "assets/Textures/UI/upgrade_failed.png");
            if (t) {
                // Use current anvil UI positioning - match exactly with UISystem indicatorRect
                Texture* anvilBG = assetManager->getTexture("assets/Textures/UI/AnvilUI.png");
                int outW=0,outH=0; SDL_GetRendererOutputSize(sdlRenderer,&outW,&outH); if (outW<=0){outW=WINDOW_WIDTH;outH=WINDOW_HEIGHT;}
                int pw = anvilBG ? anvilBG->getWidth() : 200;
                int ph = anvilBG ? anvilBG->getHeight() : 200;
                int anvilX, anvilY;
                
                // Use same positioning logic as anvil UI
                if (anvilPosX >= 0 && anvilPosY >= 0) {
                    anvilX = anvilPosX;
                    anvilY = anvilPosY;
                } else {
                    anvilX = outW / 4 - pw / 2;  // Default anvil positioning
                    anvilY = (outH - ph) / 2;
                }
                
                // Match indicator rect from UISystem.cpp exactly
                SDL_Rect indicatorRect = {
                    anvilX + pw / 2 - 55,  // Center horizontally
                    anvilY + ph - 50,      // Near bottom
                    110, 20
                };
                SDL_Rect s{0,0,t->getWidth(), t->getHeight()};
                SDL_RenderCopy(sdlRenderer, t->getTexture(), &s, &indicatorRect);
            }
        }
    }
    // Options overlay on top
    if (optionsOpen) {
        renderOptionsMenuOverlay();
    }
    
    // Present
    SDL_RenderPresent(sdlRenderer);
}

void Game::loadOrCreateDefaultUserAndSave() {
    if (!database || !player) return;
    // Ensure there is at least one default user
    auto user = database->getUserById(1);
    if (!user) {
        // Create default admin and player users
        database->registerUser("admin", "admin", UserRole::ADMIN);
        auto playerUser = database->registerUser("player", "player", UserRole::PLAYER);
        if (playerUser) {
            // Initialize default save
            PlayerSave s = player->makeSaveState();
            database->savePlayerState(playerUser->userId, s);
        }
    }
    // Load last save for default player if available
    auto def = database->getUserByName("player");
    if (def) {
        auto save = database->loadPlayerState(def->userId);
        if (save) {
            // Apply minimal fields to player
            player->applySaveState(*save);
        }
        // Load audio prefs and apply
        if (audioManager) {
            int m=100, mu=100, s=100, mon=100, pl=100;
            if (database->loadAudioSettings(def->userId, m, mu, s, mon, pl)) {
                audioManager->setMasterVolume(m);
                audioManager->setMusicVolume(mu);
                audioManager->setSoundVolume(s);
                audioManager->setMonsterVolume(mon);
                audioManager->setPlayerVolume(pl);
            }
            std::string theme;
            if (database->loadTheme(def->userId, theme) && !theme.empty()) {
                // Validate against loaded keys only
                if (theme != "main_theme" && theme != "fast_tempo") theme = "main_theme";
                backgroundMusicName = theme;
                if (audioManager->hasMusic(backgroundMusicName)) {
                    audioManager->fadeToMusic(backgroundMusicName, 200, 200);
                    currentMusicTrack = backgroundMusicName;
                } else if (audioManager->hasMusic("main_theme")) {
                    backgroundMusicName = "main_theme";
                    audioManager->fadeToMusic(backgroundMusicName, 200, 200);
                    currentMusicTrack = backgroundMusicName;
                }
            }
        }
    }
}

void Game::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                // Persist audio and theme on hard quit (Alt+F4 or window close)
                if (database && audioManager) {
                    int uid = loggedInUserId;
                    if (uid <= 0) {
                        auto defU = database->getUserByName("player");
                        if (defU) uid = defU->userId;
                    }
                    if (uid > 0) {
                        database->saveAudioSettings(uid,
                            audioManager->getMasterVolume(),
                            audioManager->getMusicVolume(),
                            audioManager->getSoundVolume(),
                            audioManager->getMonsterVolume(),
                            audioManager->getPlayerVolume());
                        database->saveTheme(uid, backgroundMusicName);
                    }
                }
                isRunning = false;
                break;
                
            case SDL_KEYDOWN:
                if (loginScreenActive) {
                    // Simple text input handling for username/password
                    if (event.key.keysym.sym == SDLK_BACKSPACE) {
                        if (SDL_GetModState() & KMOD_SHIFT) {
                            // Clear both on Shift+Backspace
                            loginUsername.clear(); loginPassword.clear();
                        } else {
                            if (loginActiveField == LoginField::Password) {
                                if (!loginPassword.empty()) loginPassword.pop_back();
                            } else {
                                if (!loginUsername.empty()) loginUsername.pop_back();
                            }
                        }
                    } else if (event.key.keysym.sym == SDLK_TAB) {
                        // Toggle focus between fields
                        loginActiveField = (loginActiveField == LoginField::Username ? LoginField::Password : LoginField::Username);
                        // Prevent the tab from propagating to game input
                        break;
                    } else if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
                        // Pressing Enter triggers Login
                        if (database) {
                            std::string err;
                            auto u = database->authenticate(loginUsername, loginPassword, &err);
                            if (u) {
                                loggedInUserId = u->userId;
                                loginIsAdmin = (u->role == UserRole::ADMIN);
                                loginScreenActive = false;
                                loginError.clear();
                                if (loginRemember) database->saveRememberState(DatabaseSQLite::RememberState{loginUsername, loginPassword, true}); else database->clearRememberState();
                                auto save = database->loadPlayerState(loggedInUserId);
                                if (save) player->applySaveState(*save);
                                // Load persisted audio settings and theme on login
                                if (audioManager) {
                                    int m=100, mu=100, s=100, mon=100, pl=100;
                                    if (database->loadAudioSettings(loggedInUserId, m, mu, s, mon, pl)) {
                                        audioManager->setMasterVolume(m);
                                        audioManager->setMusicVolume(mu);
                                        audioManager->setSoundVolume(s);
                                        audioManager->setMonsterVolume(mon);
                                        audioManager->setPlayerVolume(pl);
                                    }
                                    std::string theme;
                                    if (database->loadTheme(loggedInUserId, theme) && !theme.empty()) {
                                        backgroundMusicName = theme;
                                        if (audioManager->hasMusic(backgroundMusicName)) {
                                            audioManager->fadeToMusic(backgroundMusicName, 200, 200);
                                            currentMusicTrack = backgroundMusicName;
                                        }
                                    }
                                }
                            } else {
                                loginError = err.empty()? std::string("Invalid credentials") : err;
                            }
                        }
                        break;
                    }
                } else if (!optionsOpen && event.key.keysym.sym == SDLK_p) {
                    isPaused = !isPaused;
                } else if (!optionsOpen && event.key.keysym.sym == SDLK_F3) {
                    setDebugHitboxes(!getDebugHitboxes());
                } else if (!optionsOpen && event.key.keysym.sym == SDLK_F5) {
                    setInfinitePotions(!getInfinitePotions());
                    std::cout << "Infinite potions: " << (getInfinitePotions() ? "ON" : "OFF") << std::endl;
                } else if (!optionsOpen && event.key.keysym.sym == SDLK_F6) {
                    // Manual save
                    std::cout << "Manual save triggered (F6) - User ID: " << loggedInUserId << std::endl;
                    saveCurrentUserState();
                } else if (!optionsOpen && event.key.keysym.sym == SDLK_F7) {
                    // Debug: Enter underworld
                    if (!inUnderworld) {
                        enterUnderworld();
                        std::cout << "DEBUG: Entered underworld" << std::endl;
                    }
                } else if (!optionsOpen && event.key.keysym.sym == SDLK_F8) {
                    // Debug: Exit underworld
                    if (inUnderworld) {
                        exitUnderworld();
                        std::cout << "DEBUG: Exited underworld" << std::endl;
                    }
                } else if (!optionsOpen && event.key.keysym.sym == SDLK_i) {
                    // Toggle inventory UI
                    inventoryOpen = !inventoryOpen;
                } else if (!optionsOpen && event.key.keysym.sym == SDLK_b) {
                    // Toggle spell book
                    uiSystem->toggleSpellBook();
                } else if (!optionsOpen && event.key.keysym.sym == SDLK_u) {
                    // Toggle equipment UI
                    equipmentOpen = !equipmentOpen;
                } else if (!optionsOpen && event.key.keysym.sym == SDLK_F9) {
                    // Dev hotkey: grant large stacks of test scrolls
                    if (player) {
                        player->addItemToInventory("upgrade_scroll", 50);
                        player->addItemToInventory("fire_scroll", 50);
                        player->addItemToInventory("water_scroll", 50);
                        player->addItemToInventory("poison_scroll", 50);
                    }
                    break;
                }
                // Only forward to game input when not on login screen
                if (!optionsOpen && !loginScreenActive) inputManager->handleKeyDown(event.key);
                break;
                
            case SDL_KEYUP:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    if (anvilOpen) { anvilOpen = false; break; }
                    bool wasOpen = optionsOpen;
                    optionsOpen = !optionsOpen;
                    // Persist audio + theme whenever options menu closes
                    if (wasOpen && database && audioManager) {
                        int uid = loggedInUserId;
                        if (uid <= 0) { auto defU = database->getUserByName("player"); if (defU) uid = defU->userId; }
                        if (uid > 0) {
                            database->saveAudioSettings(uid,
                                audioManager->getMasterVolume(),
                                audioManager->getMusicVolume(),
                                audioManager->getSoundVolume(),
                                audioManager->getMonsterVolume(),
                                audioManager->getPlayerVolume());
                            database->saveTheme(uid, backgroundMusicName);
                        }
                    }
                    break;
                }
                if (!optionsOpen) inputManager->handleKeyUp(event.key);
                else handleOptionsInput(event);
                break;
                
            case SDL_MOUSEBUTTONDOWN: {
                int mx = event.button.x, my = event.button.y;
                if (loginScreenActive) {
                    // Hit test input fields and buttons
                    int outW=0,outH=0; SDL_GetRendererOutputSize(sdlRenderer,&outW,&outH); if (outW<=0){outW=WINDOW_WIDTH;outH=WINDOW_HEIGHT;}
                    int panelW=520,panelH=320; SDL_Rect panel{ outW/2 - panelW/2, outH/2 - panelH/2, panelW, panelH };
                    SDL_Rect userField{ panel.x + 120, panel.y + 85, 300, 28 };
                    SDL_Rect passField{ panel.x + 120, panel.y + 135, 300, 28 };
                    SDL_Rect chk{ panel.x + 40, panel.y + 180, 20, 20 }; // remember
                    SDL_Rect btnLogin{ panel.x + 40, panel.y + panelH - 70, 180, 44 };
                    SDL_Rect btnRegister{ panel.x + 240, panel.y + panelH - 70, 180, 44 };
                    if (mx>=userField.x && mx<=userField.x+userField.w && my>=userField.y && my<=userField.y+userField.h) { loginActiveField = LoginField::Username; }
                    else if (mx>=passField.x && mx<=passField.x+passField.w && my>=passField.y && my<=passField.y+passField.h) { loginActiveField = LoginField::Password; }
                    else if (mx>=chk.x && mx<=chk.x+chk.w && my>=chk.y && my<=chk.y+chk.h) { loginRemember = !loginRemember; }
                    else if (mx>=btnLogin.x && mx<=btnLogin.x+btnLogin.w && my>=btnLogin.y && my<=btnLogin.y+btnLogin.h) {
                        if (database) {
                            std::string err;
                            auto u = database->authenticate(loginUsername, loginPassword, &err);
                            if (u) {
                                loggedInUserId = u->userId;
                                loginIsAdmin = (u->role == UserRole::ADMIN);
                                loginScreenActive = false;
                                loginError.clear();
                                if (loginRemember) database->saveRememberState(DatabaseSQLite::RememberState{loginUsername, loginPassword, true}); else database->clearRememberState();
                                // Load save
                                auto save = database->loadPlayerState(loggedInUserId);
                                if (save) player->applySaveState(*save);
                            } else {
                                loginError = err.empty()? std::string("Invalid credentials") : err;
                            }
                        }
                    } else if (mx>=btnRegister.x && mx<=btnRegister.x+btnRegister.w && my>=btnRegister.y && my<=btnRegister.y+btnRegister.h) {
                        if (database) {
                            std::string err;
                            auto u = database->registerUser(loginUsername, loginPassword, UserRole::PLAYER, &err);
                            if (u) {
                                loggedInUserId = u->userId;
                                loginIsAdmin = false;
                                loginScreenActive = false;
                                loginError.clear();
                                if (loginRemember) database->saveRememberState(DatabaseSQLite::RememberState{loginUsername, loginPassword, true}); else database->clearRememberState();
                                // Create initial save
                                database->savePlayerState(loggedInUserId, player->makeSaveState());
                                // Initialize audio file defaults on first register
                                if (audioManager) {
                                    database->saveAudioSettings(loggedInUserId,
                                        audioManager->getMasterVolume(),
                                        audioManager->getMusicVolume(),
                                        audioManager->getSoundVolume(),
                                        audioManager->getMonsterVolume(),
                                        audioManager->getPlayerVolume());
                                    database->saveTheme(loggedInUserId, backgroundMusicName);
                                }
                            } else {
                                loginError = err.empty()? std::string("Registration failed") : err;
                            }
                        }
                    } else {
                        loginActiveField = LoginField::None;
                    }
                } else if (anvilOpen) {
                    // While anvil is open, do not close on right-click; allow UI to handle quick-use from inventory
                } else {
                    inputManager->handleMouseDown(event.button);
                }
                break; }
                
            case SDL_MOUSEBUTTONUP:
                inputManager->handleMouseUp(event.button);
                break;
                
            case SDL_MOUSEMOTION:
                if (!loginScreenActive) inputManager->handleMouseMotion(event.motion);
                break;

            case SDL_TEXTINPUT:
                if (loginScreenActive) {
                    if (loginActiveField == LoginField::Password) loginPassword += event.text.text;
                    else /* Username or None default to Username */ loginUsername += event.text.text;
                }
                break;
        }
    }
}

void Game::renderOptionsMenuOverlay() {
    if (!uiSystem) return;
    // Query current settings
    int master = audioManager ? audioManager->getMasterVolume() : 100;
    int music  = audioManager ? audioManager->getMusicVolume()  : 100;
    int sound  = audioManager ? audioManager->getSoundVolume()  : 100;
    static int monsterVolCache = 100;
    static int playerVolCache = 100;
    // On opening options, sync caches with current audio so the save path always reflects UI values
    if (optionsOpen) {
        monsterVolCache = audioManager ? audioManager->getMonsterVolume() : monsterVolCache;
        playerVolCache  = audioManager ? audioManager->getPlayerVolume()  : playerVolCache;
    }
    // Basic flags from window/renderer
    Uint32 winFlags = SDL_GetWindowFlags(window);
    bool fullscreen = (winFlags & SDL_WINDOW_FULLSCREEN) || (winFlags & SDL_WINDOW_FULLSCREEN_DESKTOP);
    bool vsync = true; // We created renderer with PRESENTVSYNC
    int mx, my; Uint32 mouse = SDL_GetMouseState(&mx, &my);
    bool mouseDown = (mouse & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    UISystem::MenuHitResult hit;
    static UISystem::OptionsTab activeTab = UISystem::OptionsTab::Main;
    uiSystem->renderOptionsMenu(optionsSelectedIndex,
                                master, music, sound,
                                monsterVolCache, playerVolCache,
                                fullscreen, vsync,
                                activeTab,
                                mx, my, mouseDown,
                                hit);
    // If a theme selection was just made, consume the edge so other controls remain interactive
    if (hit.newThemeIndex != -1) {
        // Nothing else to do here; selection will be applied below
        // Prevent stale state from blocking further clicks
        optionsSelectedIndex = optionsSelectedIndex; // no-op, edge consumed
    }
    if (hit.newTabIndex >= 0 && hit.newTabIndex <= 2) {
        activeTab = static_cast<UISystem::OptionsTab>(hit.newTabIndex);
    }
    if (audioManager) {
        if (hit.changedMaster)  { audioManager->setMasterVolume(hit.newMaster); }
        if (hit.changedMusic)   { audioManager->setMusicVolume(hit.newMusic); }
        if (hit.changedSound)   { audioManager->setSoundVolume(hit.newSound); }
        if (hit.changedMonster) { monsterVolCache = hit.newMonster; audioManager->setMonsterVolume(monsterVolCache); }
        if (hit.changedPlayer)  { playerVolCache  = hit.newPlayer;  audioManager->setPlayerVolume(playerVolCache); }
        // Persist audio settings per account when changed (fallback to default 'player' when not logged in)
        if (database && (hit.changedMaster || hit.changedMusic || hit.changedSound || hit.changedMonster || hit.changedPlayer)) {
            int uid = loggedInUserId;
            if (uid <= 0) {
                auto defU = database->getUserByName("player");
                if (defU) uid = defU->userId;
            }
            if (uid > 0) database->saveAudioSettings(uid,
                audioManager->getMasterVolume(),
                audioManager->getMusicVolume(),
                audioManager->getSoundVolume(),
                monsterVolCache,
                playerVolCache);
        }
    if (hit.toggledStopSpawns) { setStopMonsterSpawns(hit.stopSpawnsNewValue); }
    if (hit.newThemeIndex == 0) {
            backgroundMusicName = "main_theme";
            if (audioManager->hasMusic(backgroundMusicName)) { audioManager->fadeToMusic(backgroundMusicName, 200, 200); currentMusicTrack = backgroundMusicName; }
            if (database) {
                int uid = loggedInUserId;
                if (uid <= 0) { auto defU = database->getUserByName("player"); if (defU) uid = defU->userId; }
                if (uid > 0) database->saveTheme(uid, backgroundMusicName);
            }
        } else if (hit.newThemeIndex == 1) {
            backgroundMusicName = "fast_tempo";
            if (audioManager->hasMusic(backgroundMusicName)) { audioManager->fadeToMusic(backgroundMusicName, 200, 200); currentMusicTrack = backgroundMusicName; }
            if (database) {
                int uid = loggedInUserId;
                if (uid <= 0) { auto defU = database->getUserByName("player"); if (defU) uid = defU->userId; }
                if (uid > 0) database->saveTheme(uid, backgroundMusicName);
            }
        }
        // Clear theme selection edge so UI remains interactive
        hit.newThemeIndex = -1;
    }
    if (hit.clickedReset) {
        // Reset world and player to initial state
        // Regenerate world visibility and reposition player at spawn
        if (world && player) {
            // Reset enemies
            auto& enemies = world->getEnemies();
            for (auto& e : enemies) {
                if (e) e->resetToSpawn();
            }
            // Teleport player to spawn and clear projectiles
            player->respawn(player->getSpawnX(), player->getSpawnY());
            world->updateVisibility(player->getX(), player->getY());
            world->updateVisibleChunks(player->getX(), player->getY());
        }
        // Also reset music to selected background theme
        if (audioManager) {
            if (audioManager->hasMusic(backgroundMusicName)) {
                audioManager->fadeToMusic(backgroundMusicName, 400, 400);
                currentMusicTrack = backgroundMusicName;
            }
        }
        optionsOpen = false;
    }
    if (hit.clickedFullscreen) {
        Uint32 flags = SDL_GetWindowFlags(window);
        bool fs = (flags & SDL_WINDOW_FULLSCREEN) || (flags & SDL_WINDOW_FULLSCREEN_DESKTOP);
        SDL_SetWindowFullscreen(window, fs ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
        // After toggling full screen, recenter camera on the player
        if (player) {
            int playerX = static_cast<int>(player->getX());
            int playerY = static_cast<int>(player->getY());
            int outW=0,outH=0; SDL_GetRendererOutputSize(sdlRenderer, &outW, &outH);
            if (outW <= 0) { outW = WINDOW_WIDTH; outH = WINDOW_HEIGHT; }
            float zoom = renderer->getZoom();
            int cameraX = static_cast<int>(playerX - (outW / (2.0f * zoom)));
            int cameraY = static_cast<int>(playerY - (outH / (2.0f * zoom)));
            renderer->setCamera(cameraX, cameraY);
            if (world) {
                world->updateVisibleChunks(player->getX(), player->getY());
                world->updateVisibility(player->getX(), player->getY());
            }
        }
    }
    if (hit.clickedResume) {
        optionsOpen = false;
    }
    if (hit.clickedLogout) {
        // Save current state and return to login screen
        if (database && player) {
            int uid = loggedInUserId > 0 ? loggedInUserId : (database->getUserByName("player").has_value() ? database->getUserByName("player").value().userId : -1);
            if (uid > 0) {
                database->savePlayerState(uid, player->makeSaveState());
                if (audioManager) {
                    database->saveAudioSettings(uid,
                        audioManager->getMasterVolume(),
                        audioManager->getMusicVolume(),
                        audioManager->getSoundVolume(),
                        audioManager->getMonsterVolume(),
                        audioManager->getPlayerVolume());
                    database->saveTheme(uid, backgroundMusicName);
                }
            }
        }
        // Persist remember state on logout based on checkbox
        if (database) {
            if (loginRemember) {
                database->saveRememberState(DatabaseSQLite::RememberState{ loginUsername, loginPassword, true });
            } else {
                database->clearRememberState();
                loginUsername.clear();
                loginPassword.clear();
            }
        }
        loginError.clear();
        loggedInUserId = -1;
        loginScreenActive = true;
        optionsOpen = false;
    }
}

void Game::handleOptionsInput(const SDL_Event& event) {
    // Mouse-only; keyboard arrows disabled. Only handle Esc to close handled elsewhere.
    (void)event;
}

void Game::initializeObjects() {
    if (!world || !assetManager) {
        return;
    }
    
    // Clear existing objects before spawning new ones
    world->clearObjects();
    
    // OPTIMIZATION: Reduced object count and implemented intelligent spawning
    // Only spawn objects in a reasonable area around the player starting position
    const int spawnRadius = 15; // Reduced from previous large area
    int maxObjects = 0;   // Will be set to number of planned spawns
    
    std::cout << "Initializing optimized object spawning..." << std::endl;
    
    // Create a more controlled object spawning system
    std::vector<std::pair<int, int>> spawnPositions;
    
    // Only add overworld objects if not in underworld
    if (!inUnderworld) {
        spawnPositions = {
            {20, 10},  // Nearby chest
            {21, 11},  // Nearby pot
            {20, 8},   // Flag
            {18, 20},  // Bonfire
            {25, 10},  // Wood sign
            {24, 9},   // Portal to Underworld
            {15, 12},  // Clay pot
            {8, 15},   // Wood crate
            {12, 18},  // Steel crate
            {12, 26}   // Magic Anvil (overworld position)
        };
    } else {
        // Underworld only gets the magic anvil at a clear position avoiding props
        spawnPositions = {
            {12, 15}   // Magic Anvil (underworld position - moved to avoid clipping)
        };
    }
    maxObjects = static_cast<int>(spawnPositions.size());
    
    // Limit the number of objects to spawn
    int objectsSpawned = 0;
    for (const auto& pos : spawnPositions) {
        if (objectsSpawned >= maxObjects) break;
        
        int x = pos.first;
        int y = pos.second;
        
        // Check if position is within spawn radius (or special case for underworld anvil)
        bool withinRadius = (std::abs(x - 20) <= spawnRadius && std::abs(y - 11) <= spawnRadius);
        bool isUnderworldAnvil = (inUnderworld && x == 12 && y == 15);
        
        
        if (withinRadius || isUnderworldAnvil) {
            std::unique_ptr<Object> object;
            
            // Magic Anvil at different positions based on world
            // Overworld: (12,26), Underworld: (12,15) - moved to avoid clipping
            bool isAnvilPosition = inUnderworld ? (x == 12 && y == 15) : (x == 12 && y == 26);
            if (isAnvilPosition) {
                // Magic Anvil (animated) - use different sprite based on world
                std::string anvilSprite = inUnderworld ? 
                    "assets/Textures/Objects/underword_magic_anvil.png" : 
                    "assets/Textures/Objects/magic_anvil.png";
                std::cout << "Creating anvil at (" << x << "," << y << "), inUnderworld=" << inUnderworld 
                          << ", sprite=" << anvilSprite << std::endl;
                object = std::make_unique<Object>(ObjectType::MAGIC_ANVIL, x, y, anvilSprite);
                object->setSpriteSheet(assetManager->getSpriteSheet(anvilSprite));
            } else if (x == 20 && y == 10) {
                // Create different object types based on position
                // Nearby chest with good loot
                object = std::make_unique<Object>(ObjectType::CHEST_UNOPENED, x, y, "assets/Textures/Objects/chest_unopened.png");
                object->setTexture(assetManager->getTexture("assets/Textures/Objects/chest_unopened.png"));
                object->addLoot(Loot(LootType::GOLD, 100, "Gold Coins"));
                object->addLoot(Loot(LootType::EXPERIENCE, 50, "Experience"));
                object->addLoot(Loot(LootType::HEALTH_POTION, 20, "HP Charges"));
                object->addLoot(Loot(LootType::MANA_POTION, 20, "MP Charges"));
            } else if (x == 21 && y == 11) {
                // Nearby pot
                object = std::make_unique<Object>(ObjectType::CLAY_POT, x, y, "assets/Textures/Objects/clay_pot.png");
                object->setTexture(assetManager->getTexture("assets/Textures/Objects/clay_pot.png"));
                object->addLoot(Loot(LootType::GOLD, 25, "Gold Coins"));
            } else if (x == 20 && y == 8) {
                // Flag (decorative)
                object = std::make_unique<Object>(ObjectType::FLAG, x, y, "assets/Textures/Objects/flag.png");
                object->setTexture(assetManager->getTexture("assets/Textures/Objects/flag.png"));
            } else if (x == 18 && y == 20) {
                // Bonfire (animated)
                object = std::make_unique<Object>(ObjectType::BONFIRE, x, y, "assets/Textures/Objects/Bonfire.png");
                object->setSpriteSheet(assetManager->getSpriteSheet("assets/Textures/Objects/Bonfire.png"));
            } else if (x == 25 && y == 10) {
                // Wood sign (decorative)
                object = std::make_unique<Object>(ObjectType::WOOD_SIGN, x, y, "assets/Textures/Objects/wood_sign.png");
                object->setTexture(assetManager->getTexture("assets/Textures/Objects/wood_sign.png"));
            } else if (x == 24 && y == 9) {
                // Portal to Underworld at a nearby tile
                object = std::make_unique<Object>(ObjectType::PORTAL, x, y, "assets/Underworld Tilemap/Props/static/individual sprites/nasty structure_0.png");
                if (Texture* t = assetManager->getTexture("assets/Underworld Tilemap/Props/static/individual sprites/nasty structure_0.png")) {
                    object->setTexture(t);
                }
                // Interaction prompt uses default; behavior wired in Player::interact via object type
            } else if (x == 15 && y == 12) {
                // Clay pot
                object = std::make_unique<Object>(ObjectType::CLAY_POT, x, y, "assets/Textures/Objects/clay_pot.png");
                object->setTexture(assetManager->getTexture("assets/Textures/Objects/clay_pot.png"));
                object->addLoot(Loot(LootType::GOLD, 10, "Gold Coins"));
                object->addLoot(Loot(LootType::HEALTH_POTION, 10, "HP Charges"));
            } else if (x == 8 && y == 15) {
                // Wood crate
                object = std::make_unique<Object>(ObjectType::WOOD_CRATE, x, y, "assets/Textures/Objects/wood_crate.png");
                object->setTexture(assetManager->getTexture("assets/Textures/Objects/wood_crate.png"));
                object->addLoot(Loot(LootType::GOLD, 25, "Gold Coins"));
                object->addLoot(Loot(LootType::HEALTH_POTION, 20, "HP Charges"));
            } else if (x == 12 && y == 18) {
                // Steel crate
                object = std::make_unique<Object>(ObjectType::STEEL_CRATE, x, y, "assets/Textures/Objects/steel_crate.png");
                object->setTexture(assetManager->getTexture("assets/Textures/Objects/steel_crate.png"));
                object->addLoot(Loot(LootType::GOLD, 75, "Gold Coins"));
                object->addLoot(Loot(LootType::EXPERIENCE, 50, "Experience"));
                object->addLoot(Loot(LootType::MANA_POTION, 20, "MP Charges"));
            }
            
            if (object) {
                object->setAssetManager(assetManager.get());
                world->addObject(std::move(object));
                objectsSpawned++;
                std::cout << "Spawned object at (" << x << ", " << y << ")" << std::endl;
            }
        }
    }
    
    std::cout << "Optimized object spawning complete! Spawned " << objectsSpawned << " objects." << std::endl;
    std::cout << "Performance optimization: Reduced object count from unlimited to " << maxObjects << " maximum objects." << std::endl;
}

void Game::cleanup() {
    // Systems will be cleaned up automatically via unique_ptr destructors
    
    if (sdlRenderer) {
        SDL_DestroyRenderer(sdlRenderer);
        sdlRenderer = nullptr;
    }
    
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
}

void Game::saveCurrentUserState() {
    if (database && loggedInUserId > 0 && player) {
        try {
            std::string error;
            bool success = database->savePlayerState(loggedInUserId, player->makeSaveState(), &error);
            if (!success) {
                std::cout << "Error saving player state: " << error << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "Exception during save: " << e.what() << std::endl;
        } catch (...) {
            std::cout << "Unknown exception during save" << std::endl;
        }
    }
}

void Game::updatePerformanceMetrics() {
    // Calculate current FPS
    if (frameTime > 0) {
        currentFPS = 1000.0f / frameTime;
    } else {
        currentFPS = 0.0f;
    }
    
    // Add to history
    fpsHistory.push_back(currentFPS);
    if (fpsHistory.size() > FPS_HISTORY_SIZE) {
        fpsHistory.pop_front();
    }
    
    // Calculate average FPS
    if (!fpsHistory.empty()) {
        float sum = 0.0f;
        for (float fps : fpsHistory) {
            sum += fps;
        }
        averageFPS = sum / fpsHistory.size();
    } else {
        averageFPS = currentFPS;
    }
}
