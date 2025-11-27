#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

class GameObject;

class TransformComponent {
public:
    TransformComponent() {}
    TransformComponent(glm::vec3 position, glm::quat rotation, glm::vec3 scale):
        position_(position), rotation_(rotation), scale_(scale) {}
    TransformComponent(const glm::mat4& local_model_matrix) {
        setlocalModelMatrix(local_model_matrix);
    }
    
    glm::mat4 getLocalModelMatrix();
    glm::mat4 getGlobalModelMatrix();

    void setPosition(glm::vec3 position) {
        position_ = position;
        local_dirty_ = true;
        global_dirty_ = true;
    }

    void setRotation(glm::quat rotation) {
        rotation_ = rotation;
        local_dirty_ = true;
        global_dirty_ = true;
    }

    void setScale(glm::vec3 scale) {
        scale_ = scale;
        local_dirty_ = true;
        global_dirty_ = true;
    }

    void setlocalModelMatrix(const glm::mat4& local_model_matrix) {
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(local_model_matrix, scale_, rotation_, position_, skew, perspective);
        local_cache_ = local_model_matrix;
        local_dirty_ = false;
        global_dirty_ = true;
    }

    void setParent(GameObject* parent, GameObject* owner);

    const std::vector<GameObject*>& getChildren() const {
        return children_;
    }

private:
    void updateLocalCache();
    void updateGlobalCache();
    void markGlobalDirty();

    glm::vec3 position_ {0.0f, 0.0f, 0.0f};
    glm::quat rotation_ {1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale_ {1.0f, 1.0f, 1.0f};

    glm::mat4 local_cache_ {1.0f};
    glm::mat4 global_cache_ {1.0f};

    bool local_dirty_ {true};
    bool global_dirty_ {true};

    GameObject* parent_ {nullptr};
    std::vector<GameObject*> children_;
};
