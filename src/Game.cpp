#include "Game.h"
#include "Renderer.h"
#include "InputManager.h"
#include "AssetManager.h"
#include "Player.h"
#include "Projectile.h"
#include "World.h"
#include "Enemy.h"
#include "UISystem.h"
#include "AudioManager.h"
#include "Object.h"
#include "AutotileDemo.h"
#include <iostream>

Game::Game() : window(nullptr), sdlRenderer(nullptr), isRunning(false), isPaused(false), 
               lastFrameTime(0), accumulator(0.0f), frameTime(0), currentFPS(0.0f), averageFPS(0.0f) {
    initializeSystems();
}

Game::~Game() {
    cleanup();
}

void Game::initializeSystems() {
    // Create window
    window = SDL_CreateWindow(
        "PixLegends - 2D Action Adventure",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    
    if (!window) {
        throw std::runtime_error("Failed to create window: " + std::string(SDL_GetError()));
    }

    // Create renderer
    sdlRenderer = SDL_CreateRenderer(window, -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    
    if (!sdlRenderer) {
        throw std::runtime_error("Failed to create renderer: " + std::string(SDL_GetError()));
    }

    // Initialize systems
    renderer = std::make_unique<Renderer>(sdlRenderer);
    inputManager = std::make_unique<InputManager>();
    assetManager = std::make_unique<AssetManager>(sdlRenderer);
    
    // Preload assets BEFORE creating World
    assetManager->preloadAssets();
    
    // Create World after assets are loaded
    world = std::make_unique<World>(assetManager.get());
    uiSystem = std::make_unique<UISystem>(sdlRenderer);
    uiSystem->setAssetManager(assetManager.get());
    audioManager = std::make_unique<AudioManager>();
    
    // Create player after other systems are initialized
    player = std::make_unique<Player>(this);
    // Relocate spawn: e.g., near tile (15, 10)
    if (world && player) {
        int ts = world->getTileSize();
        float spawnX = 15.0f * ts;
        float spawnY = 10.0f * ts;
        player->setSpawnPoint(spawnX, spawnY);
        player->respawn(spawnX, spawnY);
    }
    
    // Initialize objects in the world
    initializeObjects();

    // Autotiling demo: press F1 to toggle; default off
    demoMode = false;
    autotileDemo = std::make_unique<AutotileDemo>(assetManager.get(), 20, 12, 32);

    // Spawn first enemy (Demon Boss) near player start
    if (world && assetManager) {
        // Player starts roughly around tile (20,11) => pixels ~ (20*32, 11*32)
        float bossSpawnX = 26.0f * world->getTileSize();
        float bossSpawnY = 11.0f * world->getTileSize();
        world->addEnemy(std::make_unique<Enemy>(bossSpawnX, bossSpawnY, assetManager.get()));
    }
    
    // Set initial visibility and generate initial visible chunks around player starting position
    if (world && player) {
        world->updateVisibility(player->getX(), player->getY());
        world->updateVisibleChunks(player->getX(), player->getY());
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
    // Update player
    if (player) {
        player->update(deltaTime);
    }
    
    // Update world
    if (world) {
        world->update(deltaTime);
        
        // Update chunks and visibility based on player position
        if (player) {
            world->updateVisibleChunks(player->getX(), player->getY());
            world->updateVisibility(player->getX(), player->getY());
            // Update enemies with player tracking
            world->updateEnemies(deltaTime, player->getX(), player->getY());
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
                    for (auto& enemyPtr : enemies) {
                        if (!enemyPtr || enemyPtr->isDead()) continue;
                        SDL_Rect eRect = enemyPtr->getCollisionRect();
                        SDL_Rect inter;
                        if (SDL_IntersectRect(&hitbox, &eRect, &inter)) {
                            if (player->consumeMeleeHitIfActive()) {
                                enemyPtr->takeDamage(player->getMeleeDamage());
                            }
                            // do not break: allow hitting first valid enemy; keep break to single
                            break;
                        }
                    }
                }
            }

            // 2) Enemy contact damage to player (simple melee)
            for (auto& enemyPtr : enemies) {
                if (!enemyPtr || enemyPtr->isDead()) continue;
                SDL_Rect eRect = enemyPtr->getCollisionRect();
                SDL_Rect playerRect{static_cast<int>(player->getX()), static_cast<int>(player->getY()), player->getWidth(), player->getHeight()};
                SDL_Rect inter;
                if (SDL_IntersectRect(&playerRect, &eRect, &inter)) {
                    if (enemyPtr->isAttackReady()) {
                        player->takeDamage(enemyPtr->getContactDamage());
                        enemyPtr->consumeAttackCooldown();
                        if (player->isDead()) {
                            // Reset enemies to idle spawn when player dies
                            for (auto& e2 : enemies) {
                                if (e2) e2->resetToSpawn();
                            }
                        }
                    }
                }
            }
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
}

void Game::render() {
    // Clear screen
    SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
    SDL_RenderClear(sdlRenderer);
    
    // Set camera
    if (demoMode) {
        renderer->setCamera(0, 0);
    } else if (player) {
        int playerX = static_cast<int>(player->getX()); // Player position is already in pixels
        int playerY = static_cast<int>(player->getY());
        
        // Center camera on player
        int cameraX = playerX - (WINDOW_WIDTH / 2);
        int cameraY = playerY - (WINDOW_HEIGHT / 2);
        
        // Set camera position
        renderer->setCamera(cameraX, cameraY);
    }
    
    // Render world or autotile demo
    if (demoMode) {
        autotileDemo->render(renderer.get());
    } else if (world) {
        world->render(renderer.get());
    }
    
    // Render player
    if (player) {
        player->render(renderer.get());
        player->renderProjectiles(renderer.get());
    }
    
    // Render UI
    if (uiSystem) {
        uiSystem->renderPlayerStats(player.get());
        uiSystem->renderDebugInfo(player.get());
        
        // Render FPS counter
        uiSystem->renderFPSCounter(currentFPS, averageFPS, frameTime);
        
        // Render death popup with respawn button if dead
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
                deathPopupFade = 0.0f;
            }
        }

        // Boss health bar while engaged
        if (world && player) {
            auto& enemies = world->getEnemies();
            // For now: show the first enemy as Demon Boss if aggroed or in range
            if (!enemies.empty() && enemies[0]) {
                Enemy* boss = enemies[0].get();
                // Consider engaged if boss is aggroed or player within attack range
                bool engaged = boss->getIsAggroed() || boss->isWithinAttackRange(player->getX(), player->getY());
                if (!boss->isDead() && engaged) {
                    int outW = 0, outH = 0;
                    SDL_GetRendererOutputSize(sdlRenderer, &outW, &outH);
                    uiSystem->renderBossHealthBar("Demon", boss->getHealth(), boss->getMaxHealth(), (outW > 0 ? outW : WINDOW_WIDTH));
                }
            }
        }

        // Debug draw melee and enemy hitboxes (F3 toggle)
        if (getDebugHitboxes() && player) {
            SDL_SetRenderDrawBlendMode(sdlRenderer, SDL_BLENDMODE_BLEND);
            // Player melee hitbox
            SDL_Rect melee = player->getMeleeHitbox();
            SDL_SetRenderDrawColor(sdlRenderer, 0, 255, 0, 160);
            if (melee.w > 0 && melee.h > 0) SDL_RenderDrawRect(sdlRenderer, &melee);
            // Enemy hitboxes
            if (world) {
                auto& enemies = world->getEnemies();
                for (auto& e : enemies) {
                    if (!e) continue;
                    SDL_Rect er = e->getCollisionRect();
                    SDL_SetRenderDrawColor(sdlRenderer, 255, 0, 0, 160);
                    SDL_RenderDrawRect(sdlRenderer, &er);
                }
            }
        }
        
        // Render interaction prompt if player is near an interactable object
        if (player) {
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
    
    // Present
    SDL_RenderPresent(sdlRenderer);
}

void Game::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                isRunning = false;
                break;
                
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    isRunning = false;
                } else if (event.key.keysym.sym == SDLK_p) {
                    isPaused = !isPaused;
                } else if (event.key.keysym.sym == SDLK_F1) {
                    demoMode = !demoMode;
                } else if (event.key.keysym.sym == SDLK_F3) {
                    setDebugHitboxes(!getDebugHitboxes());
                }
                inputManager->handleKeyDown(event.key);
                break;
                
            case SDL_KEYUP:
                inputManager->handleKeyUp(event.key);
                break;
                
            case SDL_MOUSEBUTTONDOWN:
                inputManager->handleMouseDown(event.button);
                break;
                
            case SDL_MOUSEBUTTONUP:
                inputManager->handleMouseUp(event.button);
                break;
                
            case SDL_MOUSEMOTION:
                inputManager->handleMouseMotion(event.motion);
                break;
        }
    }
}

void Game::initializeObjects() {
    if (!world || !assetManager) {
        return;
    }
    
    // OPTIMIZATION: Reduced object count and implemented intelligent spawning
    // Only spawn objects in a reasonable area around the player starting position
    const int spawnRadius = 15; // Reduced from previous large area
    const int maxObjects = 8;   // Limit total objects for performance
    
    std::cout << "Initializing optimized object spawning..." << std::endl;
    
    // Create a more controlled object spawning system
    std::vector<std::pair<int, int>> spawnPositions = {
        {20, 10},  // Nearby chest
        {21, 11},  // Nearby pot
        {20, 8},   // Flag
        {18, 20},  // Bonfire
        {25, 10},  // Wood sign
        {15, 12},  // Clay pot
        {8, 15},   // Wood crate
        {12, 18}   // Steel crate
    };
    
    // Limit the number of objects to spawn
    int objectsSpawned = 0;
    for (const auto& pos : spawnPositions) {
        if (objectsSpawned >= maxObjects) break;
        
        int x = pos.first;
        int y = pos.second;
        
        // Check if position is within spawn radius
        if (std::abs(x - 20) <= spawnRadius && std::abs(y - 11) <= spawnRadius) {
            std::unique_ptr<Object> object;
            
            // Create different object types based on position
            if (x == 20 && y == 10) {
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
