#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "glm/ext/quaternion_geometric.hpp"
#include "glm/fwd.hpp"
#include "glm/trigonometric.hpp"
#include "interaction/input.h"

class GameObject;

// Base class for all cameras
// A camera is responsible for providing a view matrix and a set of methods for controlling the camera's position and orientation
class Camera {
public:
    virtual ~Camera() = default;
    virtual glm::mat4 getViewMatrix() const = 0;
    virtual glm::vec3 getFrontVec() const = 0;
    virtual glm::vec3 getRightVec() const = 0;
    virtual glm::vec3 getUpVec() const = 0;
    virtual glm::vec3 getPosition() const = 0;
};

// -------------------------------------------------------------------------
// Free camera
// We can use this camera to move around in a 3D world, as a free observer.
// -------------------------------------------------------------------------

// Translator for free camera input
// For keyboard input, we translate W, A, S, D, space and ctrl keys to movement in the camera's forward, left, backward, right, up and down directions, respectively.
// For mouse input, we just transfer them to FreeCamera class.
class FreeCameraInputTranslator : public InputTranslator {
public:
    // Moving directions
    enum class Movement {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
        UP,
        DOWN,
        _COUNT
    };

    // Use a bitset to store the moving directions that are currently active
    using MovementSet = std::bitset<(size_t)Movement::_COUNT>;

    FreeCameraInputTranslator(const Input& input) : InputTranslator(input) {}
    
    // Get the input from Input class
    MovementSet getKeyInput() const {
        MovementSet result;

        // If a key is pressed, set the corresponding bit in the result
        // If keys that represent the opposite direction are pressed, we just cancel them out
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

    // Get the mouse input from Input class
    glm::vec2 getMouseInput() const {
        return input_.getMouseMovement();
    }
};

// 
class FreeCamera : public Camera {
public:
    FreeCamera(glm::vec3 position, glm::vec3 world_up = glm::vec3(0.0f, 1.0f, 0.0f), float pitch = -90.0f, float yaw = 0.0f, float speed = 5.0f, float sensitivity = 0.06f) : position_(position), world_up_(world_up), pitch_(pitch), yaw_(yaw), speed_(speed), sensitivity_(sensitivity) {
        updateRotation();
    }

    // Interface implementation

    // Get the view matrix of the camera
    glm::mat4 getViewMatrix() const override {
        // View = T^(-1) * R^(-1)
        glm::mat4 rotation_matrix = glm::transpose(glm::mat4_cast(rotation_));
        glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), -position_);
        return rotation_matrix * translation_matrix;
    }

    glm::vec3 getFrontVec() const override {
        return glm::normalize(glm::mat3_cast(rotation_) * DEFAULT_FRONT);
    }

    glm::vec3 getRightVec() const override {
        // right = front x world_up
        return glm::normalize(glm::cross(getFrontVec(), world_up_));
    }

    glm::vec3 getUpVec() const override {
        // up = right x front
        glm::vec3 front = getFrontVec();
        glm::vec3 right = glm::cross(front, world_up_);
        return glm::normalize(glm::cross(right, front));
    }
    glm::vec3 getPosition() const override {
        return position_;
    }

    // Setters and getters
    void setSpeed(float speed) {
        speed_ = speed;
    }

    void setSensitivity(float sensitivity) {
        sensitivity_ = sensitivity;
    }

    // Input processing methods
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

        updateRotation();
    }

    // Update the camera's position and orientation based on input
    // This method should be called every rendering frame
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
    // Helper methods
    // Update the camera's rotation based on the current pitch and yaw values
    void updateRotation() {
        glm::quat yaw_rot = glm::angleAxis(glm::radians(yaw_), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::quat pitch_rot = glm::angleAxis(glm::radians(pitch_), glm::vec3(1.0f, 0.0f, 0.0f));

        rotation_ = glm::normalize(yaw_rot * pitch_rot);
    }

    static constexpr glm::vec3 DEFAULT_FRONT = glm::vec3(0.0f, 0.0f, -1.0f);

    glm::vec3 position_;
    glm::quat rotation_;
    glm::vec3 world_up_;

    glm::vec3 prev_position_;
    glm::quat prev_rotation_;

    float pitch_ {0.0f};
    float yaw_ {-90.0f};

    float speed_;          // Camera movement speed
    float sensitivity_;    // Mouse sensitivity
};


class ThirdPersonCamera : public Camera {
public:
    ThirdPersonCamera(
        GameObject* target,
        float distance = 10.0f,
        float pitch = 0.0f,
        float yaw = 0.0f,
        float smooth_factor = 5.0f,
        float sensitivity = 0.06f,
        glm::vec3 offset = glm::vec3(0.0f, 0.0f, 0.0f),
        glm::quat base_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f)
    );

    glm::mat4 getViewMatrix() const override {
        // std::cout << camera_position_ << " " << lookat_position_ << " " << camera_up_ << std::endl;
        return glm::lookAt(camera_position_, getLookAtTargetPosition(), camera_up_);
    }

    glm::vec3 getFrontVec() const override;

    glm::vec3 getRightVec() const override {
        return glm::normalize(glm::cross(getFrontVec(), camera_up_));
    }

    glm::vec3 getUpVec() const override {
        return glm::normalize(glm::cross(getRightVec(), getFrontVec()));
    }

    glm::vec3 getPosition() const override {
        return camera_position_;
    }

    glm::vec3 getUp() const {
        return camera_up_;
    }

    void update(glm::vec2 mouse_offset, float dt) {
        processMouseInput(mouse_offset);

        rotation_ = getRotation();
        camera_up_ = getCameraUpVector();
        glm::vec3 lookat_position_interp = getLookAtPosition(dt);
        camera_position_ = lookat_position_interp - getFrontVec() * distance_;
        lookat_position_ = lookat_position_interp;
    }

    void processMouseInput(glm::vec2 look_pos_offset, bool constrain_pitch = true) {
        float x_offset = look_pos_offset.x;
        float y_offset = look_pos_offset.y;

        yaw_ -= x_offset * sensitivity_;
        pitch_ -= y_offset * sensitivity_;

        if (constrain_pitch) {
            pitch_ = glm::clamp(pitch_, -89.0f, 89.0f);
        }
    }

private:
    glm::vec3 getLookAtTargetPosition() const;

    glm::vec3 getLookAtPosition(float dt) const {
        float param = dt * smooth_factor_;
        if (param > 1.0f) {
            param = 1.0f;
        }
        return glm::mix(lookat_position_, getLookAtTargetPosition(), param);
    }

    glm::vec3 getCameraUpVector() const;

    glm::quat getRotation() const;

    glm::vec3 lookat_position_;

    float distance_ { 5.0f };
    float pitch_ { 0.0f };
    float yaw_ { 0.0f };
    
    float smooth_factor_ { 5.0f };
    float sensitivity_ { 0.06f };
    glm::vec3 offset_ { 0.0f, 0.0f, 0.0f };
    glm::quat base_rotation_ { 1.0f, 0.0f, 0.0f, 0.0f };
    
    GameObject* target_ = nullptr;

    glm::vec3 camera_up_ { 0.0f, 1.0f, 0.0f };
    glm::vec3 camera_position_ { 0.0f, 0.0f, 0.0f };
    glm::quat rotation_ { 1.0f, 0.0f, 0.0f, 0.0f };
};
