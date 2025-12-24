#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "common/game_object.h"
#include "resource/model.h"
#include "entity/physical.h"

class Plane : public GameObject {
public:
    Plane(const Input& input, const Model& model, const glm::vec3& position = glm::vec3(0.0f), const glm::quat& rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3 velocity = glm::vec3(0.0f), glm::vec3 angular_velocity = glm::vec3(0.0f), glm::mat4 bias_transform = glm::mat4(1.0f),PhysicalComponent physical_component = PhysicalComponent()): translator_(input), physical_component_(physical_component) {
        GameObject::createFromModel(this, model, bias_transform);
        
        auto& transform = getTransformComponent();
        auto& render = getRenderComponent();

        transform.setPosition(position);
        transform.setRotation(rotation);
        transform.setParent(nullptr, this);

        render.renderable_ = false;

        physical_component_.initialize(position, rotation, velocity, angular_velocity);
    }

    void update(float dt,int obj_kind,glm::vec3 target) {
        physical_component_.update(translator_,ai_translator_,obj_kind,target, dt);
        synchronizeTransform();
        // std::cout << physical_component_.getPosition() << " " << physical_component_.getVelocity() << std::endl;
    }

    PhysicalComponent& physical_component() {
        return physical_component_;
    }

private:
    void synchronizeTransform() {
        auto& transform = getTransformComponent();
        transform.setPosition(physical_component_.getPosition());
        transform.setRotation(physical_component_.getRotation());
    }

    enum class Controller {
        PLAYER = 0,
        AI = 1
    } controller_;

    PhysicalInputTranslator translator_;
    PhysicalAITranslator ai_translator_;
    PhysicalComponent physical_component_;
};
