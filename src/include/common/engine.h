#pragma once

#include "resource/model.h"
#include "resource/texture.h"
#include "resource/material.h"
#include "service/service_locator.h"
#include "utils/config.h"

// Game engine class
// This class is responsible for initializing and cleaning up all the managers and services
class GameEngine {
public:
    GameEngine(): shader_manager(), texture_manager(), material_manager()
    {
        // Initialize managers and services and register them in service locators
        config.init();
        ServiceLocator<Config>::provide(&config);

        shader_manager.init();
        ServiceLocator<ShaderManager>::provide(&shader_manager);

        texture_manager.init();
        ServiceLocator<TextureManager>::provide(&texture_manager);

        material_manager.init();
        ServiceLocator<MaterialManager>::provide(&material_manager);
    }

    ~GameEngine() {
        // Terminate services and clean up managers
        ServiceLocator<MaterialManager>::provide(nullptr);
        material_manager.clear();

        ServiceLocator<TextureManager>::provide(nullptr);
        texture_manager.clear();

        ServiceLocator<ShaderManager>::provide(nullptr);
        shader_manager.clear();

        ServiceLocator<Config>::provide(nullptr);
        config.clear();
    }

private:
    Config config;
    ShaderManager shader_manager;
    TextureManager texture_manager;
    MaterialManager material_manager;
};
