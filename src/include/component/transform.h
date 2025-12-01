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

    glm::vec3 getPosition() const {
        return position_;
    }

    glm::quat getRotation() const {
        return rotation_;
    }

    glm::vec3 getScale() const {
        return scale_;
    }

    void setPosition(glm::vec3 position) {
        position_ = position;
        local_dirty_ = true;
        markGlobalDirty();
    }

    void setRotation(glm::quat rotation) {
        rotation_ = rotation;
        local_dirty_ = true;
        markGlobalDirty();
    }

    void setScale(glm::vec3 scale) {
        scale_ = scale;
        local_dirty_ = true;
        markGlobalDirty();
    }

    void setlocalModelMatrix(const glm::mat4& local_model_matrix) {
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(local_model_matrix, scale_, rotation_, position_, skew, perspective);
        local_cache_ = local_model_matrix;
        local_dirty_ = false;
        markGlobalDirty();
    }

    void translate(glm::vec3 translation) {
        position_ += translation;
        local_dirty_ = true;
        markGlobalDirty();
    }

    void rotate(glm::quat rotation) {
        rotation_ = glm::normalize(rotation_ * rotation);
        local_dirty_ = true;
        markGlobalDirty();
    }

    void rotate(glm::vec3 axis, float angle) {
        if (glm::abs(angle) < 1.0e-5f) {
            return;
        }
        rotate(glm::angleAxis(angle, axis));
    }

    void apply_angluar_velocity(glm::vec3 angular_velocity, float dt) {
        float angle = glm::length(angular_velocity) * dt;
        rotate(glm::normalize(angular_velocity), angle);
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
