#include "interaction/camera.h"
#include "common/game_object.h"
#include "component/transform.h"

ThirdPersonCamera::ThirdPersonCamera(
    GameObject* target,
    float distance,
    float pitch,
    float yaw,
    float smooth_factor,
    float sensitivity,
    glm::vec3 offset,
    glm::quat base_rotation
): distance_(distance), pitch_(pitch), yaw_(yaw), smooth_factor_(smooth_factor), sensitivity_(sensitivity), target_(target), offset_(offset), base_rotation_(base_rotation) {
    lookat_position_ = getLookAtTargetPosition();
    rotation_ = getRotation();
}

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
