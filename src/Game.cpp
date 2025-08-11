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
#include "Database.h"
#include <iostream>

Game::Game() : window(nullptr), sdlRenderer(nullptr), isRunning(false), isPaused(false), 
               lastFrameTime(0), accumulator(0.0f), frameTime(0), currentFPS(0.0f), averageFPS(0.0f) {
    initializeSystems();
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
    database = std::make_unique<Database>();
    database->initialize("data");
    // Preload remembered login if any
    {
        auto remember = database->loadRememberState();
        if (remember.remember) {
            loginRemember = true;
            loginUsername = remember.username;
            loginPassword = remember.password;
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
        // Try WAV first (works without SDL_mixer), then OGG (works when SDL_mixer is available)
        audioManager->loadMusic("main_theme", "assets/Sound/Music/main_theme.wav");
        audioManager->loadMusic("main_theme", "assets/Sound/Music/main_theme.ogg");
        audioManager->loadMusic("boss_music", "assets/Sound/Music/boss_music.wav");
    }
    
    // Create player after other systems are initialized
    player = std::make_unique<Player>(this);
    // Start at login screen; defer save load until authenticated
    loadOrCreateDefaultUserAndSave();
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

    // Autotiling demo: press F1 to toggle; default off
    demoMode = false;
    autotileDemo = std::make_unique<AutotileDemo>(assetManager.get(), 20, 12, 32);

    // Spawn enemies: Demon and Wizard
    if (world && assetManager) {
        // Demon near start
        float demonX = 26.0f * world->getTileSize();
        float demonY = 11.0f * world->getTileSize();
        world->addEnemy(std::make_unique<Enemy>(demonX, demonY, assetManager.get(), EnemyKind::Demon));
        // Wizard nearby
        float wizX = 30.0f * world->getTileSize();
        float wizY = 14.0f * world->getTileSize();
        world->addEnemy(std::make_unique<Enemy>(wizX, wizY, assetManager.get(), EnemyKind::Wizard));
    }
    
    // Set initial visibility and generate initial visible chunks around player starting position
    if (world && player) {
        world->updateVisibility(player->getX(), player->getY());
        world->updateVisibleChunks(player->getX(), player->getY());
    }
    // Start main theme at game start
    // Start main theme at game start if available
    if (audioManager && audioManager->hasMusic("main_theme")) {
        audioManager->playMusic("main_theme");
        currentMusicTrack = "main_theme";
    } else {
        currentMusicTrack.clear();
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
    // Update player
    if (player) {
        player->update(deltaTime);
    }
    
    // Update world
    if (world) {
        bool playedPlayerMeleeSfx = false; // prevent boss melee SFX from overriding on the same frame
        world->update(deltaTime);
        
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

            // 2) Enemy contact damage to player (simple melee) and wizard projectiles
            for (auto& enemyPtr : enemies) {
                if (!enemyPtr || enemyPtr->isDead()) continue;
                SDL_Rect eRect = enemyPtr->getCollisionRect();
                SDL_Rect playerRect = player->getCollisionRect();
                SDL_Rect inter;
                if (SDL_IntersectRect(&playerRect, &eRect, &inter)) {
                    if (enemyPtr->isAttackReady()) {
                        player->takeDamage(enemyPtr->getContactDamage());
                        if (audioManager && !playedPlayerMeleeSfx) { 
                            audioManager->playSound("boss_melee");
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
    if (demoMode) {
        renderer->setCamera(0, 0);
    } else if (player) {
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
            // Prefer to show Demon bar if a Demon is engaged; otherwise show first engaged enemy
            Enemy* engagedEnemy = nullptr;
            for (auto& e : enemies) {
                if (!e || e->isDead()) continue;
                // Use player center for engagement check to match enemy AI distance math
                float pcx = player->getX() + player->getWidth() * 0.5f;
                float pcy = player->getY() + player->getHeight() * 0.5f;
                bool engaged = e->getIsAggroed() || e->isWithinAttackRange(pcx, pcy);
                if (!engaged) continue;
                engagedEnemy = e.get();
                if (std::string(engagedEnemy->getDisplayName()) == "Demon") break;
            }
            if (engagedEnemy) {
                    int outW = 0, outH = 0;
                    SDL_GetRendererOutputSize(sdlRenderer, &outW, &outH);
                uiSystem->renderBossHealthBar(engagedEnemy->getDisplayName(), engagedEnemy->getHealth(), engagedEnemy->getMaxHealth(), (outW > 0 ? outW : WINDOW_WIDTH));
                // Start boss music if available
                if (audioManager && audioManager->hasMusic("boss_music") && currentMusicTrack != "boss_music") { 
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
                        if (audioManager->hasMusic("main_theme")) {
                            if (currentMusicTrack != "main_theme") {
                                audioManager->fadeToMusic("main_theme", 500, 400);
                                currentMusicTrack = "main_theme";
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
        if (getDebugHitboxes() && player) {
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
    }
}

void Game::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
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
                                if (loginRemember) database->saveRememberState(Database::RememberState{loginUsername, loginPassword, true}); else database->clearRememberState();
                                auto save = database->loadPlayerState(loggedInUserId);
                                if (save) player->applySaveState(*save);
                            } else {
                                loginError = err.empty()? std::string("Invalid credentials") : err;
                            }
                        }
                        break;
                    }
                } else if (!optionsOpen && event.key.keysym.sym == SDLK_p) {
                    isPaused = !isPaused;
                } else if (!optionsOpen && event.key.keysym.sym == SDLK_F1) {
                    demoMode = !demoMode;
                } else if (!optionsOpen && event.key.keysym.sym == SDLK_F3) {
                    setDebugHitboxes(!getDebugHitboxes());
                } else if (!optionsOpen && event.key.keysym.sym == SDLK_F5) {
                    setInfinitePotions(!getInfinitePotions());
                    std::cout << "Infinite potions: " << (getInfinitePotions() ? "ON" : "OFF") << std::endl;
                }
                // Only forward to game input when not on login screen
                if (!optionsOpen && !loginScreenActive) inputManager->handleKeyDown(event.key);
                break;
                
            case SDL_KEYUP:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    optionsOpen = !optionsOpen;
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
                                if (loginRemember) database->saveRememberState(Database::RememberState{loginUsername, loginPassword, true}); else database->clearRememberState();
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
                                if (loginRemember) database->saveRememberState(Database::RememberState{loginUsername, loginPassword, true}); else database->clearRememberState();
                                // Create initial save
                                database->savePlayerState(loggedInUserId, player->makeSaveState());
                            } else {
                                loginError = err.empty()? std::string("Registration failed") : err;
                            }
                        }
                    } else {
                        loginActiveField = LoginField::None;
                    }
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
    // Basic flags from window/renderer
    Uint32 winFlags = SDL_GetWindowFlags(window);
    bool fullscreen = (winFlags & SDL_WINDOW_FULLSCREEN) || (winFlags & SDL_WINDOW_FULLSCREEN_DESKTOP);
    bool vsync = true; // We created renderer with PRESENTVSYNC
    int mx, my; Uint32 mouse = SDL_GetMouseState(&mx, &my);
    bool mouseDown = (mouse & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    UISystem::MenuHitResult hit;
    static UISystem::OptionsTab activeTab = UISystem::OptionsTab::Main;
    uiSystem->renderOptionsMenu(optionsSelectedIndex, master, music, sound, fullscreen, vsync, activeTab, mx, my, mouseDown, hit);
    if (hit.newTabIndex >= 0 && hit.newTabIndex <= 2) {
        activeTab = static_cast<UISystem::OptionsTab>(hit.newTabIndex);
    }
    if (audioManager) {
        if (hit.changedMaster) audioManager->setMasterVolume(hit.newMaster);
        if (hit.changedMusic)  audioManager->setMusicVolume(hit.newMusic);
        if (hit.changedSound)  audioManager->setSoundVolume(hit.newSound);
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
        // Also reset music to main theme
        if (audioManager) {
            if (audioManager->hasMusic("main_theme")) {
                audioManager->fadeToMusic("main_theme", 400, 400);
                currentMusicTrack = "main_theme";
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
        if (database && loggedInUserId > 0 && player) {
            database->savePlayerState(loggedInUserId, player->makeSaveState());
        }
        // Persist remember state on logout based on checkbox
        if (database) {
            if (loginRemember) {
                database->saveRememberState(Database::RememberState{ loginUsername, loginPassword, true });
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

void Game::saveCurrentUserState() {
    if (database && loggedInUserId > 0 && player) {
        database->savePlayerState(loggedInUserId, player->makeSaveState());
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
