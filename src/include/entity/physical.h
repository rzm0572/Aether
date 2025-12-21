#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "interaction/input.h"
#include "utils/profiler.h"

enum class PhysicalInput {
    SPEEDUP,
    PITCH_UP,
    YAW_RIGHT,
    ROLL_RIGHT,
    SLOWDOWN,
    PITCH_DOWN,
    YAW_LEFT,
    ROLL_LEFT,
    _COUNT
};

class PhysicalInputTranslator : public InputTranslator {
public:
    PhysicalInputTranslator(const Input& input) : InputTranslator(input) {}

    using PhysicalInputSet = std::bitset<(size_t)PhysicalInput::_COUNT>;

    PhysicalInputSet translate() const {
        size_t result_raw = 0;

        if (input_.getKeyPressed(InputKey::W)) {
            result_raw |= (1 << (size_t)PhysicalInput::SPEEDUP);
        }

        if (input_.getKeyPressed(InputKey::S)) {
            result_raw |= (1 << (size_t)PhysicalInput::SLOWDOWN);
        }

        if (input_.getKeyPressed(InputKey::D)) {
            result_raw |= (1 << (size_t)PhysicalInput::YAW_RIGHT);
        }

        if (input_.getKeyPressed(InputKey::A)) {
            result_raw |= (1 << (size_t)PhysicalInput::YAW_LEFT);
        }

        if (input_.getKeyPressed(InputKey::SPACE)) {
            result_raw |= (1 << (size_t)PhysicalInput::PITCH_UP);
        }

        if (input_.getKeyPressed(InputKey::SHIFT)) {
            result_raw |= (1 << (size_t)PhysicalInput::PITCH_DOWN);
        }

        if (input_.getKeyPressed(InputKey::E)) {
            result_raw |= (1 << (size_t)PhysicalInput::ROLL_RIGHT);
        }

        if (input_.getKeyPressed(InputKey::Q)) {
            result_raw |= (1 << (size_t)PhysicalInput::ROLL_LEFT);
        }


        size_t half_count = (size_t)PhysicalInput::_COUNT / 2;
        
        size_t lower_half = result_raw & ((1 << half_count) - 1);
        size_t upper_half = result_raw >> half_count;
        size_t mask = lower_half ^ upper_half;

        size_t result = result_raw & (mask | (mask << half_count));

        // std::cout << PhysicalInputSet(result_raw).to_string() << std::endl;
        // std::cout << PhysicalInputSet(result).to_string() << std::endl;

        return PhysicalInputSet(result);
    }
};


class PhysicalComponent {
public:
    PhysicalComponent(
        float mass = 1000.0f,
        float max_thrust_force = 10000.0f,        // mass * max_thrust_to_weight_ratio
        float thrust_power = 7500000.0f,          // max_thrust_force * max_speed
        float lift_coeff = 20.0f,
        float forward_friction = 0.016f,            // forward_friction * speed_forward^2 = drag force
        float lateral_friction = 50.0f,
        float vertical_friction = 25.0f,
        float pitch_force = 8.0f,
        float yaw_force = 2.0f,
        float roll_force = 8.0f,
        float angular_damping = 4.0f,
        float zero_lift_aoa = 3.0f
    ) : mass_(mass), thrust_force_(max_thrust_force), thrust_power_(thrust_power), lift_coeff_(lift_coeff), forward_friction_(forward_friction), lateral_friction_(lateral_friction), vertical_friction_(vertical_friction), pitch_force_(pitch_force), yaw_force_(yaw_force), roll_force_(roll_force), angular_damping_(angular_damping), zero_lift_aoa_(zero_lift_aoa) {}

    void initialize(glm::vec3 position, glm::quat rotation, glm::vec3 velocity = glm::vec3(0, 0, 0), glm::vec3 angular_velocity = glm::vec3(0, 0, 0)) {
        position_ = position;
        rotation_ = rotation;
        velocity_ = velocity;
        angular_velocity_ = angular_velocity;
    }

    glm::vec3 getPosition() const {
        return position_;
    }

    glm::quat getRotation() const {
        return rotation_;
    }

    glm::vec3 getVelocity() const {
        return velocity_;
    }

    glm::vec3 getAngularVelocity() const {
        return angular_velocity_;
    }

    void setPosition(const glm::vec3& position) {
        position_ = position;
    }

    void setRotation(const glm::quat& rotation) {
        rotation_ = rotation;
    }

    void setVelocity(const glm::vec3& velocity) {
        velocity_ = velocity;
    }

    void setAngularVelocity(const glm::vec3& angular_velocity) {
        angular_velocity_ = angular_velocity;
    }

    void update(PhysicalInputTranslator translator, float dt) {
        synchronizeVectors();

        auto inputs = translator.translate();
        glm::vec3 torque = getTorque(inputs);
        glm::vec3 force = getForce(inputs);

        angular_velocity_ += torque * dt;
        angular_velocity_ *= 1 - angular_damping_ * dt;

        float angle = glm::length(angular_velocity_) * dt;

        if (glm::length(angular_velocity_) > 1.0e-3f) {
            glm::vec3 axis = glm::normalize(angular_velocity_);
            glm::quat delta_rotation = glm::angleAxis(angle, axis);
            rotation_ = glm::normalize(rotation_ * delta_rotation);
        }

        glm::vec3 acceleration = force / mass_;
        
        position_ += (velocity_ + 0.5f * acceleration * dt) * dt;
        velocity_ += acceleration * dt;

        synchronizeVectors();
    }

    glm::vec3 GetVelocity(){
        return velocity_;
    }
    glm::vec3 GetForward(){
        return forward_;
    }
    void output() {
        std::cout << "Position: " << position_ << std::endl;
        std::cout << "Rotation: " << rotation_ << std::endl;
        std::cout << "Velocity: " << velocity_ << std::endl;
        std::cout << "Angular Velocity: " << angular_velocity_ << std::endl;
    }

private:
    void synchronizeVectors() {
        forward_ = rotation_ * glm::vec3(1, 0, 0);
        up_ = rotation_ * glm::vec3(0, 1, 0);
        right_ = rotation_ * glm::vec3(0, 0, 1);
    }

    float getThrustForce() {
        float forward_speed = glm::length(velocity_);
        return glm::min(thrust_force_, thrust_power_ / forward_speed);
    }

    float getDragForce() {
        // float speed_sqr = glm::dot(velocity_, velocity_);
        // if (speed_sqr < 1e-4f) {
        //     return 0.0f;
        // }

        float speed_forward = glm::dot(velocity_, forward_);
        if (speed_forward < 1.0e-3f) {
            return 0.0f;
        }
        
        return -forward_friction_ * speed_forward * speed_forward;
    }

    float getSideDragForce() {
        float side_speed = glm::dot(velocity_, right_);
        if (glm::abs(side_speed) < 1.0e-3f) {
            return 0.0f;
        }
        return -lateral_friction_ * side_speed * glm::abs(side_speed);
    }

    float getVerticalDragForce() {
        float vertical_speed = glm::dot(velocity_, up_);
        if (glm::abs(vertical_speed) < 1.0e-3f) {
            return 0.0f;
        }
        return -vertical_friction_ * vertical_speed * glm::abs(vertical_speed);
    }

    float getLiftForce() {
        float speed_sqr = glm::dot(velocity_, velocity_);
        if (speed_sqr < 1.0e-6f) {
            return 0.0f;
        }

        glm::vec3 velocity_normalized = glm::normalize(velocity_);
        float geometric_aoa_factor = -glm::dot(velocity_normalized, up_);
        float zero_lift_aoa_factor = glm::sin(glm::radians(zero_lift_aoa_));
        return (geometric_aoa_factor + zero_lift_aoa_factor) * lift_coeff_ * speed_sqr;
    }

    glm::vec3 getForce(PhysicalInputTranslator::PhysicalInputSet inputs) {
        glm::vec3 force {0, 0, 0};
        force -= glm::vec3(0, mass_ * gravity, 0);
        if (inputs.test((size_t)PhysicalInput::SPEEDUP)) {
            force += forward_ * getThrustForce();
        }
        
        if (glm::length(velocity_) > 1.0e-3f) {
            force += forward_ * getDragForce();
            force += right_ * getSideDragForce();
            force += up_ * getVerticalDragForce();
        }
        force += up_ * getLiftForce();

        // std::cout << "Force: " << force << std::endl;
        // std::cout << "ThrustForce: " << getThrustForce() << std::endl;
        // std::cout << "DragForce: " << getDragForce() << std::endl;
        // std::cout << "SideDragForce: " << getSideDragForce() << std::endl;
        // std::cout << "VerticalDragForce: " << getVerticalDragForce() << std::endl;
        // std::cout << "LiftForce: " << getLiftForce() << std::endl;
        // std::cout << forward_ << " " << up_ << " " << right_ << std::endl;

        return force;
    }

    glm::vec3 getTorque(PhysicalInputTranslator::PhysicalInputSet inputs) {
        glm::vec3 torque {0, 0, 0};

        const glm::vec3 axis_yaw {0, 1, 0};
        const glm::vec3 axis_roll {1, 0, 0};
        const glm::vec3 axis_pitch {0, 0, 1};

        if (inputs.test((size_t)PhysicalInput::YAW_RIGHT)) {
            torque -= axis_yaw * yaw_force_;
        }
        if (inputs.test((size_t)PhysicalInput::YAW_LEFT)) {
            torque += axis_yaw * yaw_force_;
        }

        if (inputs.test((size_t)PhysicalInput::PITCH_UP)) {
            torque += axis_pitch * pitch_force_;
        }
        if (inputs.test((size_t)PhysicalInput::PITCH_DOWN)) {
            torque -= axis_pitch * pitch_force_;
        }

        if (inputs.test((size_t)PhysicalInput::ROLL_RIGHT)) {
            torque += axis_roll * roll_force_;
        }
        if (inputs.test((size_t)PhysicalInput::ROLL_LEFT)) {
            torque -= axis_roll * roll_force_;
        }
        
        // std::cout << std::endl;
        // std::cout << "Torque: " << torque << std::endl;
        // std::cout << "forward: " << forward_ << std::endl;

        return torque;
    }

private:
    static constexpr float gravity { 9.8f };      // m / s^2
    static constexpr glm::vec3 MODEL_FORWARD { 1, 0, 0 };
    static constexpr glm::vec3 MODEL_UP { 0, 1, 0 };
    static constexpr glm::vec3 MODEL_RIGHT { 0, 0, 1 };

    glm::vec3 position_ {0, 0, 0};
    glm::quat rotation_ {1, 0, 0, 0};

    glm::vec3 velocity_ {0, 0, 0};
    glm::vec3 angular_velocity_ {0, 0, 0};

    float mass_ { 1000.0f };

    float thrust_force_ { 10000.0f };
    float thrust_power_ { 7500000.0f };
    float lift_coeff_ { 0.5f };
    float forward_friction_ { 0.5f };
    float lateral_friction_ { 5.0f };
    float vertical_friction_ { 1.0f };

    float pitch_force_ { 8.0f };
    float yaw_force_ { 2.0f };
    float roll_force_ { 8.0f };
    float angular_damping_ { 4.0f };
    float zero_lift_aoa_ { 3.0f };

    glm::vec3 forward_;
    glm::vec3 up_;
    glm::vec3 right_;
};
