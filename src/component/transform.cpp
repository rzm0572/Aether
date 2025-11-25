#include "component/transform.h"
#include "common/game_object.h"

glm::mat4 TransformComponent::getLocalModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position_);
    model = glm::rotate(model, glm::angle(rotation_), glm::axis(rotation_));
    model = glm::scale(model, scale_);
    return model;
}

glm::mat4 TransformComponent::getGlobalModelMatrix() const {
    glm::mat4 parent_global_model = parent_ ? parent_->getTransformComponent().getGlobalModelMatrix() : glm::mat4(1.0f);
    return parent_global_model * getLocalModelMatrix();
}
