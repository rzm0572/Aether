#pragma once

#include "common/game_object.h"
#include "common/renderer.h"
#include "utils/path_handler.h"
#include "service/service_locator.h"
#include "resource/shader.h"
#include "resource/material.h"
#include "resource/explosion.h"

class Plane;

struct ExplosionMonitor {
    const Model& model;
    Plane* go;
    float duration;
};

class ExplosionSystem {
    struct ExplosionInstance {
        std::unique_ptr<ExplodedModel> model;
        std::unique_ptr<GameObject> go;
        float start_time;
        float duration;
    };

public:
    ExplosionSystem() = default;

    ~ExplosionSystem() {
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

    void clear() {
        explosions_.clear();
    }

    void addExplosion(const Model& model, GameObject* go, float now, float duration) {
        auto exploded_model = std::make_unique<ExplodedModel>(model, 256);
        auto exploded_go = std::unique_ptr<GameObject>(exploded_model->createExplosion(go));
        
        explosions_.emplace_back(
            std::move(exploded_model), std::move(exploded_go), now, duration
        );
    }

    void addMonitor(const Model& model, Plane* go, float duration);

    void removeMonitor(UUID_t uuid) {
        monitors_.erase(uuid);
    }

    void update(Renderer& renderer, float now) {
        handleMonitor(now);

        size_t remaining_size = 0;
        std::vector<ExplosionInstance> to_remove;
        for (size_t i = 0; i < explosions_.size(); ++i) {
            auto& explosion = explosions_[i];
            if (now - explosion.start_time >= explosion.duration) {
                to_remove.emplace_back(std::move(explosion));
                continue;
            }

            explosions_[remaining_size] = std::move(explosion);
            renderer.submit(explosions_[remaining_size].go.get(), now - explosion.start_time);
            remaining_size++;
        }

        explosions_.resize(remaining_size);
    }

private:
    void handleMonitor(float now);

    std::vector<ExplosionInstance> explosions_;
    std::unordered_map<UUID_t, ExplosionMonitor> monitors_;
};
