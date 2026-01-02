#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "common/game_object.h"
#include "resource/model.h"
#include "entity/physical.h"
#include "entity/collision.h"
#include "entity/health.h"
#include "service/service_locator.h"
#include "system/explosion.h"

enum class Owner {
    PLAYER = 0,
    ENEMY = 1
};

class Plane : public CollisionableObject {
public:
    Plane(
        const Owner owner,
        const Input& input,
        const Model& model,
        const glm::vec3& position = glm::vec3(0.0f),
        const glm::quat& rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
        glm::vec3 velocity = glm::vec3(0.0f),
        glm::vec3 angular_velocity = glm::vec3(0.0f),
        const CollisionConfig& collision_config = CollisionConfig(),
        glm::mat4 bias_transform = glm::mat4(1.0f),
        PhysicalComponent physical_component = PhysicalComponent()
    ): CollisionableObject(collision_config), owner_(owner), translator_(input), physical_component_(physical_component) {
        GameObject::createFromModel(this, model, bias_transform);
        
        auto& transform = getTransformComponent();
        auto& render = getRenderComponent();
        auto& collision = getCollisionComponent();

        transform.setPosition(position);
        transform.setRotation(rotation);
        transform.setParent(nullptr, this);

        render.renderable_ = false;

        physical_component_.initialize(position, rotation, velocity, angular_velocity);

        collision.setType(CollisionObjectType::PLANE);
        if (owner_ == Owner::PLAYER) {
            collision.setLayer(CollisionLayer::LAYER_PLAYER);
        } else if (owner_ == Owner::ENEMY) {
            collision.setLayer(CollisionLayer::LAYER_ENEMY);
        } else {
            collision.setLayer(CollisionLayer::LAYER_NEUTRAL);
        }

        auto* explosion_system = ServiceLocator<ExplosionSystem>::get();
        if (explosion_system != nullptr) {
            explosion_system->addMonitor(model, this, 5.0f);
        }
    }

    void update(float dt,int obj_kind,glm::vec3 target) {
        if (health_component_.isDead()) {
            return;
        }

        physical_component_.update(translator_,ai_translator_,obj_kind,target, dt);
        synchronizeTransform();
        // std::cout << physical_component_.getPosition() << " " << physical_component_.getVelocity() << std::endl;
    }

    PhysicalComponent& physical_component() {
        return physical_component_;
    }

    HealthComponent& getHealthComponent() {
        return health_component_;
    }

    virtual void handleCollision() override {
        auto& collision = getCollisionComponent();
        const auto& collision_events = collision.getCollisionEvents();
        CollisionObjectType this_type = collision.getType();

        for (const auto& event : collision_events) {
            if (event.layer == CollisionLayer::LAYER_PLAYER && owner_ == Owner::PLAYER) {
                continue;
            }
            if (event.layer == CollisionLayer::LAYER_ENEMY && owner_ == Owner::ENEMY) {
                continue;
            }

            switch (event.type) {
                case CollisionObjectType::BULLET: {
                    if (this_type == CollisionObjectType::PLANE) {
                        health_component_.doDamage(event.custom_data);
                    }
                    break;
                }
                case CollisionObjectType::TERRAIN:
                case CollisionObjectType::PLANE:
                case CollisionObjectType::MISSILE:
                case CollisionObjectType::BOMB: {
                    health_component_.kill();
                    break;
                }
                default: break;
            }
        }

        collision.clearEvents();
    }
    void synchronizeTransform() {
        auto& transform = getTransformComponent();
        transform.setPosition(physical_component_.getPosition());
        transform.setRotation(physical_component_.getRotation());
    }
private:


    Owner owner_;

    PhysicalInputTranslator translator_;
    PhysicalAITranslator ai_translator_;
    PhysicalComponent physical_component_;
    HealthComponent health_component_;
};
