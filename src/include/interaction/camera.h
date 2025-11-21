#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "glm/ext/quaternion_geometric.hpp"
#include "interaction/input.h"

class Camera {
public:
    virtual ~Camera() = default;
    virtual glm::mat4 getViewMatrix() const = 0;
    virtual glm::vec3 getFrontVec() const = 0;
    virtual glm::vec3 getRightVec() const = 0;
    virtual glm::vec3 getUpVec() const = 0;
};

class FreeCameraInputTranslator : public InputTranslator {
public:
    enum class Movement {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
        UP,
        DOWN,
        _COUNT
    };
    using MovementSet = std::bitset<(size_t)Movement::_COUNT>;

    FreeCameraInputTranslator(const Input& input) : InputTranslator(input) {}
    
    MovementSet getKeyInput() const {
        MovementSet result;
        if (input_.getKeyPressed(InputKey::W)) {
            result.set((size_t)Movement::FORWARD);
        }
        if (input_.getKeyPressed(InputKey::S)) {
            if (result.test((size_t)Movement::FORWARD)) {
                result.reset((size_t)Movement::FORWARD);
            } else {
                result.set((size_t)Movement::BACKWARD);
            }
        }

        if (input_.getKeyPressed(InputKey::A)) {
            result.set((size_t)Movement::LEFT);
        }
        if (input_.getKeyPressed(InputKey::D)) {
            if (result.test((size_t)Movement::LEFT)) {
                result.reset((size_t)Movement::LEFT);
            } else {
                result.set((size_t)Movement::RIGHT);
            }
        }

        if (input_.getKeyPressed(InputKey::SPACE)) {
            result.set((size_t)Movement::UP);
        }
        if (input_.getKeyPressed(InputKey::CTRL)) {
            if (result.test((size_t)Movement::UP)) {
                result.reset((size_t)Movement::UP);
            } else {
                result.set((size_t)Movement::DOWN);
            }
        }

        return result;
    }

    glm::vec2 getMouseInput() const {
        return input_.getMouseMovement();
    }
};

class FreeCamera : public Camera {
public:
    FreeCamera(glm::vec3 position, glm::vec3 world_up = glm::vec3(0.0f, 1.0f, 0.0f), glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f), float speed = 5.0f, float sensitivity = 0.06f) : position_(position), rotation_(rotation), world_up_(world_up), speed_(speed), sensitivity_(sensitivity) {}

    glm::mat4 getViewMatrix() const override {
        glm::mat4 rotation_matrix = glm::transpose(glm::mat4_cast(rotation_));
        glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), -position_);
        return rotation_matrix * translation_matrix;
    }

    glm::vec3 getFrontVec() const override {
        return glm::normalize(glm::mat3_cast(rotation_) * DEFAULT_FRONT);
    }

    glm::vec3 getRightVec() const override {
        return glm::normalize(glm::cross(getFrontVec(), world_up_));
    }

    glm::vec3 getUpVec() const override {
        glm::vec3 front = getFrontVec();
        glm::vec3 right = glm::cross(front, world_up_);
        return glm::normalize(glm::cross(right, front));
    }

    void processKeyInput(FreeCameraInputTranslator::MovementSet movement, float delta_time) {
        float dist = speed_ * delta_time;
        glm::vec3 delta_movement = glm::vec3(0.0f);
        
        using FreeCameraMovement = FreeCameraInputTranslator::Movement;
        if (movement.test((size_t)FreeCameraMovement::FORWARD)) {
            delta_movement += getFrontVec();
        } else if (movement.test((size_t)FreeCameraMovement::BACKWARD)) {
            delta_movement -= getFrontVec();
        }

        if (movement.test((size_t)FreeCameraMovement::LEFT)) {
            delta_movement -= getRightVec();
        } else if (movement.test((size_t)FreeCameraMovement::RIGHT)) {
            delta_movement += getRightVec();
        }

        if (movement.test((size_t)FreeCameraMovement::UP)) {
            delta_movement += getUpVec();
        } else if (movement.test((size_t)FreeCameraMovement::DOWN)) {
            delta_movement -= getUpVec();
        }

        if (movement.any()) {
            position_ += glm::normalize(delta_movement) * dist;
        }
    }

    void processMouseInput(glm::vec2 look_pos_offset, bool constrain_pitch = true) {
        float x_offset = look_pos_offset.x;
        float y_offset = look_pos_offset.y;

        yaw_ -= x_offset * sensitivity_;
        pitch_ -= y_offset * sensitivity_;

        if (constrain_pitch) {
            pitch_ = glm::clamp(pitch_, -89.0f, 89.0f);
        }

        glm::quat yaw_rot = glm::angleAxis(glm::radians(yaw_), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::quat pitch_rot = glm::angleAxis(glm::radians(pitch_), glm::vec3(1.0f, 0.0f, 0.0f));

        rotation_ = glm::normalize(yaw_rot * pitch_rot);
    }

    void update(const FreeCameraInputTranslator& input_translator, float delta_time) {
        prev_position_ = position_;
        prev_rotation_ = rotation_;

        auto key_input = input_translator.getKeyInput();
        auto mouse_input = input_translator.getMouseInput();

        processKeyInput(key_input, delta_time);
        processMouseInput(mouse_input);
        // std::cout << "position: " << position_.x << " " << position_.y << " " << position_.z << std::endl;
        // std::cout << "mouse movement: " << mouse_input.x << " " << mouse_input.y << std::endl;
        // std::cout << "pitch: " << pitch_ << ", yaw: " << yaw_ << std::endl;
    }

private:
    static constexpr glm::vec3 DEFAULT_FRONT = glm::vec3(0.0f, 0.0f, -1.0f);

    glm::vec3 position_;
    glm::quat rotation_;
    glm::vec3 world_up_;

    glm::vec3 prev_position_;
    glm::quat prev_rotation_;

    float pitch_;
    float yaw_;

    float speed_;
    float sensitivity_;
};
