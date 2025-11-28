#pragma once

#include "glm/fwd.hpp"
#include "utils/profiler.h"
#include <cstddef>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <bitset>

// Keyboard keys
enum class InputKey {
    W, A, S, D,
    H, J, K, L,
    CTRL, SPACE,
    ESC,
    _COUNT
};

class Input {
public:
    // A table mapping GLFW keys to InputKeys
    inline static const struct KeyTable {
        int glfw_key;
        InputKey key;
    } key_table[] = {
        {GLFW_KEY_W, InputKey::W},
        {GLFW_KEY_A, InputKey::A},
        {GLFW_KEY_S, InputKey::S},
        {GLFW_KEY_D, InputKey::D},
        {GLFW_KEY_H, InputKey::H},
        {GLFW_KEY_J, InputKey::J},
        {GLFW_KEY_K, InputKey::K},
        {GLFW_KEY_L, InputKey::L},
        {GLFW_KEY_LEFT_CONTROL, InputKey::CTRL},
        {GLFW_KEY_SPACE, InputKey::SPACE},
        {GLFW_KEY_ESCAPE, InputKey::ESC},
    };

public:
    // Call this function in the main loop to update the input state
    void pollEvents() {
        glfwPollEvents();
    }

    bool getKeyPressed(InputKey key) const {
        return key_.test((size_t)key);
    }

    bool getKeyPressedDown(InputKey key) const {
        return key_.test((size_t)key) && !prev_key_.test((size_t)key);
    }

    glm::vec2 getMouseMovement() const {
        return mouse_movement_;
    }

    // After all logic which uses the input state in the gameloop has been executed, call this function to update the previous state
    void endUpdate() {
        prev_key_ = key_;
        prev_mouse_pos_ += mouse_movement_;
        mouse_movement_ = glm::vec2(0.0f);
    }

    static InputKey glfwKeyToInputKey(int glfw_key) {
        for (const auto& entry : key_table) {
            if (entry.glfw_key == glfw_key) {
                return entry.key;
            }
        }
        return InputKey::_COUNT;
    }

    // Keyboard callback function
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
        Input* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
        if (!input) {
            return;
        }

        if (action == GLFW_REPEAT) {
            return;
        }

        bool pressed = action == GLFW_PRESS;
        InputKey key_enum = Input::glfwKeyToInputKey(key);
        if (key_enum == InputKey::_COUNT) {
            return;
        }
        input->setKey(key_enum, pressed);
    }

    // Mouse cursor callback function
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
        Input* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
        if (!input) {
            return;
        }

        input->setMouseMovement(xpos, ypos);
    }

private:
    // Set the state of a key
    // Used in keyboard callback function
    void setKey(InputKey key, bool is_pressed) {
        key_[(size_t)key] = is_pressed;
    }

    // Set the mouse movement
    // Used in mouse cursor callback function
    void setMouseMovement(float xpos, float ypos) {
        if (first_mouse_focus_) {
            prev_mouse_pos_ = glm::vec2(xpos, ypos);
            first_mouse_focus_ = false;
        }
        mouse_movement_ = glm::vec2(xpos, ypos) - prev_mouse_pos_;
        if (std::abs(mouse_movement_.x) > 1.0e4f || std::abs(mouse_movement_.y) > 1.0e4f) {
            mouse_movement_ = glm::vec2(0.0f);
        }
    }

private:
    std::bitset<(size_t)InputKey::_COUNT> key_;
    std::bitset<(size_t)InputKey::_COUNT> prev_key_;
    glm::vec2 prev_mouse_pos_ { 0.0f };
    glm::vec2 mouse_movement_ { 0.0f };
    bool first_mouse_focus_ = false;
};

// Base class for input translators
// Input translators translate raw input from the input system to higher-level actions.
// We can set keyboard mappings in this class.
class InputTranslator {
public:
    InputTranslator(const Input& input) : input_(input) {}

    virtual ~InputTranslator() = default;
protected:
    const Input& input_;
};
