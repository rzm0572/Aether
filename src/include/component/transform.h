#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class GameObject;

class TransformComponent {
public:
    TransformComponent() {}
    TransformComponent(glm::vec3 position, glm::quat rotation, glm::vec3 scale):
        position_(position), rotation_(rotation), scale_(scale) {}
    
    glm::mat4 getLocalModelMatrix() const;

    glm::mat4 getGlobalModelMatrix() const;

public:
    glm::vec3 position_ {0.0f, 0.0f, 0.0f};
    glm::quat rotation_ {1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale_ {1.0f, 1.0f, 1.0f};

    GameObject* parent_ = nullptr;
    std::vector<GameObject*> children_;
};
