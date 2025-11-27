#include "component/transform.h"
#include "common/game_object.h"

glm::mat4 TransformComponent::getLocalModelMatrix() {
    if (local_dirty_) {
        updateLocalCache();
    }
    return local_cache_;
}

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

void TransformComponent::updateLocalCache() {
    local_cache_ = glm::mat4(1.0f);
    local_cache_ = glm::translate(local_cache_, position_);
    local_cache_ = glm::rotate(local_cache_, glm::angle(rotation_), glm::axis(rotation_));
    local_cache_ = glm::scale(local_cache_, scale_);
    local_dirty_ = false;
}

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

void TransformComponent::markGlobalDirty() {
    if (global_dirty_) {
        return;
    }

    global_dirty_ = true;
    for (auto* child : children_) {
        child->getTransformComponent().markGlobalDirty();
    }
}

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
