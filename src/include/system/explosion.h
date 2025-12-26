#pragma once

#include "common/game_object.h"
#include "common/renderer.h"
#include "utils/path_handler.h"
#include "service/service_locator.h"
#include "resource/shader.h"
#include "resource/material.h"
#include "resource/explosion.h"

class ExplosionSystem {
    struct ExplosionInstance {
        ExplodedModel* model;
        GameObject* go;
        float start_time;
        float duration;
    };

public:
    ExplosionSystem() = default;

    ~ExplosionSystem() {
        for (auto& explosion : explosions_) {
            delete explosion.go;
            delete explosion.model;
        }

        explosions_.clear();
    }

    void init() {
        auto* material_manager = ServiceLocator<MaterialManager>::get();
        auto* shader_manager = ServiceLocator<ShaderManager>::get();

        if (material_manager == nullptr || shader_manager == nullptr) {
            std::cerr << "Error: Resource Manager is not initialized." << std::endl;
            return;
        }

        shader_manager->registerShader("explosion", getShaderPath("explosion.vert"), getShaderPath("models.frag"));

        material_manager->registerMaterial("explosion",
            std::make_shared<Material>(
                shader_manager->getShader("explosion")
            )
        );
    }

    void addExplosion(const Model& model, GameObject* go, float now, float duration) {
        ExplodedModel* exploded_model = new ExplodedModel(model, 256);
        GameObject* exploded_go = exploded_model->createExplosion(go);
        
        explosions_.emplace_back(
            exploded_model, exploded_go, now, duration
        );
    }

    void update(Renderer& renderer, float now) {
        std::vector<ExplosionInstance> remaining_explosions;
        std::vector<ExplosionInstance> to_remove;
        for (auto& explosion : explosions_) {
            if (now - explosion.start_time >= explosion.duration) {
                to_remove.push_back(explosion);
                continue;
            }

            remaining_explosions.push_back(explosion);
            renderer.submit(explosion.go, now - explosion.start_time);
        }

        explosions_.swap(remaining_explosions);
        for (auto& explosion : to_remove) {
            delete explosion.go;
            delete explosion.model;
        }
    }

private:
    std::vector<ExplosionInstance> explosions_;
};
