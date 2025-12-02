#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

class GameObject;

/**
 * @brief Transform component for GameObjects
 * 
 * This component manages the position, rotation, and scale of a GameObject in 3D space.
 * It supports hierarchical transformations with parent-child relationships and uses
 * lazy evaluation with dirty flags to optimize matrix calculations.
 */
class TransformComponent {
public:
    /**
     * @brief Default constructor
     * 
     * Initializes the transform with identity values (position at origin, no rotation, scale of 1).
     */
    TransformComponent() {}
    
    /**
     * @brief Construct a new Transform Component with specific values
     * 
     * @param position The initial position in 3D space
     * @param rotation The initial rotation as a quaternion
     * @param scale The initial scale along each axis
     */
    TransformComponent(glm::vec3 position, glm::quat rotation, glm::vec3 scale):
        position_(position), rotation_(rotation), scale_(scale) {}
    
    /**
     * @brief Construct a new Transform Component from a transformation matrix
     * 
     * @param local_model_matrix The local model matrix to decompose into position, rotation, and scale
     */
    TransformComponent(const glm::mat4& local_model_matrix) {
        setlocalModelMatrix(local_model_matrix);
    }
    
    /**
     * @brief Get the local model matrix
     * 
     * Returns the transformation matrix in local space. The matrix is computed from
     * position, rotation, and scale and cached until any of these properties change.
     * 
     * @return glm::mat4 The local transformation matrix
     */
    glm::mat4 getLocalModelMatrix();
    
    /**
     * @brief Get the global model matrix
     * 
     * Returns the transformation matrix in world space. This is computed by
     * multiplying the parent's global matrix with this object's local matrix.
     * 
     * @return glm::mat4 The global transformation matrix
     */
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

    /**
     * @brief Set the transform from a local model matrix
     * 
     * Decompose the matrix into position, rotation, and scale components.
     * 
     * @param local_model_matrix The transformation matrix to decompose
     */
    void setlocalModelMatrix(const glm::mat4& local_model_matrix) {
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(local_model_matrix, scale_, rotation_, position_, skew, perspective);
        local_cache_ = local_model_matrix;
        local_dirty_ = false;
        markGlobalDirty();
    }

    /**
     * @brief Translate the transform by a given vector
     * 
     * @param translation The translation vector to add to the current position
     */
    void translate(glm::vec3 translation) {
        position_ += translation;
        local_dirty_ = true;
        markGlobalDirty();
    }

    /**
     * @brief Translate the transform by a given velocity over a time step
     * 
     * @param velocity The translation velocity vector (direction is translation, magnitude is speed)
     * @param dt The time step in seconds
     */
    void apply_velocity(glm::vec3 velocity, float dt) {
        translate(velocity * dt);
    }

    /**
     * @brief Rotate by a quaternion
     * 
     * @param rotation The rotation quaternion to apply
     */
    void rotate(glm::quat rotation) {
        rotation_ = glm::normalize(rotation_ * rotation);
        local_dirty_ = true;
        markGlobalDirty();
    }

    /**
     * @brief Rotate around an axis by an angle
     * 
     * @param axis The axis of rotation (should be normalized)
     * @param angle The rotation angle in radians
     */
    void rotate(glm::vec3 axis, float angle) {
        if (glm::abs(angle) < 1.0e-5f) {
            return;
        }
        rotate(glm::angleAxis(angle, axis));
    }

    /**
     * @brief Apply angular velocity over a time step
     * 
     * @param angular_velocity The angular velocity vector (direction is axis, magnitude is angular speed)
                               e.g. (PI/2, 0, 0) for a rotation around the X axis by 90 degrees per second
     * @param dt The time step in seconds
     */
    void apply_angluar_velocity(glm::vec3 angular_velocity, float dt) {
        float angle = glm::length(angular_velocity) * dt;
        rotate(glm::normalize(angular_velocity), angle);
    }

    /**
     * @brief Set the parent GameObject
     * 
     * Updates the parent-child relationship, removing this object from the old parent's
     * children list and adding it to the new parent's children list.
     * 
     * @param parent The new parent GameObject (or nullptr for no parent)
     * @param owner The GameObject that owns this transform component
     */
    void setParent(GameObject* parent, GameObject* owner);

    /**
     * @brief Get all child GameObjects
     * 
     * @return const std::vector<GameObject*>& Reference to the vector of child GameObjects
     */
    const std::vector<GameObject*>& getChildren() const {
        return children_;
    }

private:
    /**
     * @brief Update the cached local transformation matrix
     * 
     * Recomputes the local matrix from position, rotation, and scale.
     */
    void updateLocalCache();
    
    /**
     * @brief Update the cached global transformation matrix
     * 
     * Recursively updates global matrices for this transform and all children.
     */
    void updateGlobalCache();
    
    /**
     * @brief Mark global matrix as dirty
     * 
     * Recursively marks this transform and all children as needing global matrix recalculation.
     */
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
