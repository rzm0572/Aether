#pragma once

#include "glm/fwd.hpp"
#include <cstddef>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <bitset>

enum class InputKey {
    W, A, S, D,
    H, J, K, L,
    CTRL, SPACE,
    ESC,
    _COUNT
};

class Input {
public:
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
    void pollEvents() {
        glfwPollEvents();
    }

    void setKey(InputKey key, bool is_pressed) {
        key_[(size_t)key] = is_pressed;
        // std::cout << key_.to_string() << std::endl;
    }

    void setMouseMovement(float xpos, float ypos) {
        if (first_mouse_focus_) {
            prev_mouse_pos_ = glm::vec2(xpos, ypos);
            first_mouse_focus_ = false;
        }
        mouse_movement_ = glm::vec2(xpos, ypos) - prev_mouse_pos_;
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

    static void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
        Input* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
        if (!input) {
            return;
        }

        input->setMouseMovement(xpos, ypos);
    }

private:
    std::bitset<(size_t)InputKey::_COUNT> key_;
    std::bitset<(size_t)InputKey::_COUNT> prev_key_;
    glm::vec2 prev_mouse_pos_;
    glm::vec2 mouse_movement_;
    bool first_mouse_focus_ = false;
};

class InputTranslator {
public:
    InputTranslator(const Input& input) : input_(input) {}

    virtual ~InputTranslator() = default;
protected:
    const Input& input_;
};
