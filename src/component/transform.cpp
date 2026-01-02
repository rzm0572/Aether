#include "component/transform.h"
#include "common/game_object.h"

/**
 * @brief Get the local model matrix
 * 
 * Returns the cached local transformation matrix. If the cache is dirty,
 * it will be updated before returning.
 * 
 * @return glm::mat4 The local transformation matrix
 */
glm::mat4 TransformComponent::getLocalModelMatrix() {
    if (local_dirty_) {
        updateLocalCache();
    }
    return local_cache_;
}

/**
 * @brief Get the global model matrix
 * 
 * Returns the cached global transformation matrix in world space.
 * If the cache is dirty, it will be updated by multiplying the parent's
 * global matrix with this object's local matrix.
 * 
 * @return glm::mat4 The global transformation matrix
 */
glm::mat4 TransformComponent::getGlobalModelMatrix() {
    if (global_dirty_) {
        if (local_dirty_) {
            updateLocalCache();
        }

        if (parent_ == nullptr) {
            global_cache_ = local_cache_;
        } else {
            global_cache_ = parent_->getTransformComponent().getGlobalModelMatrix() * local_cache_;
        }

        global_dirty_ = false;
    }

    return global_cache_;
}

/**
 * @brief Update the local transformation matrix cache
 * 
 * Recomputes the local matrix by applying translation, rotation, and scale
 * transformations in sequence. Marks the local cache as clean after updating.
 */
void TransformComponent::updateLocalCache() {
    local_cache_ = glm::mat4(1.0f);
    local_cache_ = glm::translate(local_cache_, position_);
    local_cache_ = glm::rotate(local_cache_, glm::angle(rotation_), glm::axis(rotation_));
    local_cache_ = glm::scale(local_cache_, scale_);
    local_dirty_ = false;
}

/**
 * @brief Update the global transformation matrix cache
 * 
 * Recursively updates the global transformation matrix for this transform
 * and all its children. If the parent is dirty, updates the parent first.
 * The global matrix is computed by multiplying the parent's global matrix
 * with the local matrix.
 */
void TransformComponent::updateGlobalCache() {
    if (parent_ != nullptr && parent_->getTransformComponent().global_dirty_) {
        parent_->getTransformComponent().updateGlobalCache();
    } else {
        if (local_dirty_) {
            updateLocalCache();
        }

        if (parent_ == nullptr) {
            global_cache_ = local_cache_;
        } else {
            global_cache_ = parent_->getTransformComponent().global_cache_ * local_cache_;
        }

        global_dirty_ = false;
        for (auto* child : children_) {
            child->getTransformComponent().updateGlobalCache();
        }
    }
}

/**
 * @brief Mark the global transformation matrix as dirty
 * 
 * Recursively marks this transform and all its children as needing
 * global matrix recalculation. Uses early return if already marked dirty
 * to avoid redundant traversal.
 */
void TransformComponent::markGlobalDirty() {
    if (global_dirty_) {
        return;
    }

    global_dirty_ = true;
    for (auto* child : children_) {
        child->getTransformComponent().markGlobalDirty();
    }
}

/**
 * @brief Set the parent GameObject
 * 
 * Updates the parent-child relationship by:
 * 1. Removing this object from the old parent's children list (if any)
 * 2. Marking the global matrix as dirty
 * 3. Setting the new parent
 * 4. Adding this object to the new parent's children list (if any)
 * 
 * @param parent The new parent GameObject (or nullptr for no parent)
 * @param owner The GameObject that owns this transform component
 */
void TransformComponent::setParent(GameObject* parent, GameObject* owner) {
    if (parent_ == parent) {
        return;
    }

    if (parent_ != nullptr) {
        auto& parent_children = parent_->getTransformComponent().children_;
        auto it = std::find(parent_children.begin(), parent_children.end(), owner);
        if (it != parent_children.end()) {
            parent_children.erase(it);
        }
    }

    markGlobalDirty();
    parent_ = parent;

    if (parent_ != nullptr) {
        auto& parent_children = parent_->getTransformComponent().children_;
        parent_children.push_back(owner);
    }
}
