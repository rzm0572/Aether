#pragma once

#include "resource/model.h"
#include "resource/texture.h"
#include "resource/material.h"
#include "system/explosion.h"
#include "system/collision.h"
#include "service/service_locator.h"
#include "utils/config.h"

// Game engine class
// This class is responsible for initializing and cleaning up all the managers and services
class GameEngine {
public:
    GameEngine(Config& config): config(config), shader_manager(), texture_manager(), material_manager(), explosion_system()
    {
        // Initialize managers and services and register them in service locators
        shader_manager.init();
        ServiceLocator<ShaderManager>::provide(&shader_manager);

        texture_manager.init();
        ServiceLocator<TextureManager>::provide(&texture_manager);

        material_manager.init();
        ServiceLocator<MaterialManager>::provide(&material_manager);

        explosion_system.init();
        ServiceLocator<ExplosionSystem>::provide(&explosion_system);

        collision_system.init();
        ServiceLocator<CollisionSystem>::provide(&collision_system);
    }

    ~GameEngine() {
        // Terminate services and clean up managers
        ServiceLocator<CollisionSystem>::provide(nullptr);
        collision_system.clear();

        ServiceLocator<ExplosionSystem>::provide(nullptr);
        explosion_system.clear();

        ServiceLocator<MaterialManager>::provide(nullptr);
        material_manager.clear();

        ServiceLocator<TextureManager>::provide(nullptr);
        texture_manager.clear();

        ServiceLocator<ShaderManager>::provide(nullptr);
        shader_manager.clear();
    }

private:
    Config& config;
    ShaderManager shader_manager;
    TextureManager texture_manager;
    MaterialManager material_manager;
    ExplosionSystem explosion_system;
    CollisionSystem collision_system;
};
