#include "interaction/camera.h"
#include "common/game_object.h"

glm::vec3 ThirdPersonCamera::getFrontVec() const {
    float pitch = glm::radians(pitch_);
    float yaw = glm::radians(yaw_);
    float cos_pitch = glm::cos(pitch);
    float sin_pitch = glm::sin(pitch);
    float cos_yaw = glm::cos(yaw);
    float sin_yaw = glm::sin(yaw);
    return glm::normalize(rotation_ * glm::vec3(
        cos_pitch * sin_yaw,
        sin_pitch,
        cos_pitch * cos_yaw
    ));
}

glm::vec3 ThirdPersonCamera::getLookAtTargetPosition() const {
    auto transform = target_->getTransformComponent();
    return transform.getPosition() + transform.getRotation() * offset_;
}

glm::vec3 ThirdPersonCamera::getCameraUpVector() const {
    return rotation_ * glm::vec3(0.0f, 1.0f, 0.0f);
}

glm::quat ThirdPersonCamera::getRotation() const {
    return base_rotation_ * target_->getTransformComponent().getRotation();
}
