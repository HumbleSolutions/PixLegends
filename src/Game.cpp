#include "Game.h"
#include "Renderer.h"
#include "InputManager.h"
#include "AssetManager.h"
#include "Player.h"
#include "Projectile.h"
#include "World.h"
#include "UISystem.h"
#include "AudioManager.h"
#include "Object.h"
#include <iostream>

Game::Game() : window(nullptr), sdlRenderer(nullptr), isRunning(false), isPaused(false), 
               lastFrameTime(0), accumulator(0.0f) {
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
    world = std::make_unique<World>();
    uiSystem = std::make_unique<UISystem>(sdlRenderer);
    audioManager = std::make_unique<AudioManager>();
    
    // Preload assets
    assetManager->preloadAssets();
    
    // Create player after other systems are initialized
    player = std::make_unique<Player>(this);
    
    // Initialize objects in the world
    initializeObjects();
    
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
        
        // Cap frame rate
        Uint32 frameTime = SDL_GetTicks() - currentTime;
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
    }
    
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
    
    // Render world
    if (world) {
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
    
    // Create some sample objects in the world with loot
    auto chest = std::make_unique<Object>(ObjectType::CHEST_UNOPENED, 10, 10, "assets/FULL_Fantasy Forest/Objects/chest_unopened.png");
    chest->setTexture(assetManager->getTexture("assets/FULL_Fantasy Forest/Objects/chest_unopened.png"));
    chest->setAssetManager(assetManager.get());
    chest->addLoot(Loot(LootType::GOLD, 50, "Gold Coins"));
    chest->addLoot(Loot(LootType::EXPERIENCE, 25, "Experience"));
    world->addObject(std::move(chest));
    
    auto clayPot = std::make_unique<Object>(ObjectType::CLAY_POT, 15, 12, "assets/FULL_Fantasy Forest/Objects/clay_pot.png");
    clayPot->setTexture(assetManager->getTexture("assets/FULL_Fantasy Forest/Objects/clay_pot.png"));
    clayPot->setAssetManager(assetManager.get());
    clayPot->addLoot(Loot(LootType::GOLD, 10, "Gold Coins"));
    world->addObject(std::move(clayPot));
    
    // Add objects closer to player starting position (around tile 20, 11)
    auto nearbyChest = std::make_unique<Object>(ObjectType::CHEST_UNOPENED, 20, 10, "assets/FULL_Fantasy Forest/Objects/chest_unopened.png");
    nearbyChest->setTexture(assetManager->getTexture("assets/FULL_Fantasy Forest/Objects/chest_unopened.png"));
    nearbyChest->setAssetManager(assetManager.get());
    nearbyChest->addLoot(Loot(LootType::GOLD, 100, "Gold Coins"));
    nearbyChest->addLoot(Loot(LootType::EXPERIENCE, 50, "Experience"));
    world->addObject(std::move(nearbyChest));
    
    auto nearbyPot = std::make_unique<Object>(ObjectType::CLAY_POT, 21, 11, "assets/FULL_Fantasy Forest/Objects/clay_pot.png");
    nearbyPot->setTexture(assetManager->getTexture("assets/FULL_Fantasy Forest/Objects/clay_pot.png"));
    nearbyPot->setAssetManager(assetManager.get());
    nearbyPot->addLoot(Loot(LootType::GOLD, 25, "Gold Coins"));
    world->addObject(std::move(nearbyPot));
    
    auto flag = std::make_unique<Object>(ObjectType::FLAG, 20, 8, "assets/FULL_Fantasy Forest/Objects/flag.png");
    flag->setTexture(assetManager->getTexture("assets/FULL_Fantasy Forest/Objects/flag.png"));
    flag->setAssetManager(assetManager.get());
    world->addObject(std::move(flag));
    
    auto woodCrate = std::make_unique<Object>(ObjectType::WOOD_CRATE, 8, 15, "assets/FULL_Fantasy Forest/Objects/wood_crate.png");
    woodCrate->setTexture(assetManager->getTexture("assets/FULL_Fantasy Forest/Objects/wood_crate.png"));
    woodCrate->setAssetManager(assetManager.get());
    woodCrate->addLoot(Loot(LootType::GOLD, 25, "Gold Coins"));
    woodCrate->addLoot(Loot(LootType::HEALTH_POTION, 20, "Health Potion"));
    world->addObject(std::move(woodCrate));
    
    auto steelCrate = std::make_unique<Object>(ObjectType::STEEL_CRATE, 12, 18, "assets/FULL_Fantasy Forest/Objects/steel_crate.png");
    steelCrate->setTexture(assetManager->getTexture("assets/FULL_Fantasy Forest/Objects/steel_crate.png"));
    steelCrate->setAssetManager(assetManager.get());
    steelCrate->addLoot(Loot(LootType::GOLD, 75, "Gold Coins"));
    steelCrate->addLoot(Loot(LootType::EXPERIENCE, 50, "Experience"));
    steelCrate->addLoot(Loot(LootType::MANA_POTION, 30, "Mana Potion"));
    world->addObject(std::move(steelCrate));
    
    auto woodFence = std::make_unique<Object>(ObjectType::WOOD_FENCE, 5, 5, "assets/FULL_Fantasy Forest/Objects/wood_fence.png");
    woodFence->setTexture(assetManager->getTexture("assets/FULL_Fantasy Forest/Objects/wood_fence.png"));
    woodFence->setAssetManager(assetManager.get());
    world->addObject(std::move(woodFence));
    
    auto woodFenceBroken = std::make_unique<Object>(ObjectType::WOOD_FENCE_BROKEN, 6, 5, "assets/FULL_Fantasy Forest/Objects/wood_fence_broken.png");
    woodFenceBroken->setTexture(assetManager->getTexture("assets/FULL_Fantasy Forest/Objects/wood_fence_broken.png"));
    woodFenceBroken->setAssetManager(assetManager.get());
    world->addObject(std::move(woodFenceBroken));
    
    auto woodSign = std::make_unique<Object>(ObjectType::WOOD_SIGN, 25, 10, "assets/FULL_Fantasy Forest/Objects/wood_sign.png");
    woodSign->setTexture(assetManager->getTexture("assets/FULL_Fantasy Forest/Objects/wood_sign.png"));
    woodSign->setAssetManager(assetManager.get());
    world->addObject(std::move(woodSign));
    
    auto bonfire = std::make_unique<Object>(ObjectType::BONFIRE, 18, 20, "assets/FULL_Fantasy Forest/Objects/Bonfire.png");
    bonfire->setSpriteSheet(assetManager->getSpriteSheet("assets/FULL_Fantasy Forest/Objects/Bonfire.png"));
    bonfire->setAssetManager(assetManager.get());
    world->addObject(std::move(bonfire));
    
    std::cout << "Objects initialized in the world with loot!" << std::endl;
    std::cout << "Added nearby objects at tiles (20,10) and (21,11) for testing!" << std::endl;
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
