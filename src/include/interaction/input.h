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
    H, J, K, L, MOUSE_LEFT, MOUSE_RIGHT,Y,U,// 武器控制
    Q, E,
    B, N, M, COMMA, PERIOD,// 控制光照，分别为 RGB，增加亮度，减少亮度
    LEFT_BRACKET, RIGHT_BRACKET, BACKSLASH,// 控制光线方向  [ ] 控制y方向太阳高度角，\\ 符号可以控制昼夜变换
    SEMICOLON, APOSTROPHE,// 控制光线方向  ; ' 控制x方向太阳高度角
    SLASH,// 控制光线方向，/ 回到正午时分
    SHIFT, CTRL, SPACE,
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
        {GLFW_KEY_Q, InputKey::Q},
        {GLFW_KEY_E, InputKey::E},
        // 武器控制
        {GLFW_KEY_H, InputKey::H},// 空射导弹
        {GLFW_KEY_J, InputKey::J},// 机炮+魔法
        {GLFW_KEY_K, InputKey::K},// 火力支援
        {GLFW_KEY_L, InputKey::L},// 主动防御
        {GLFW_MOUSE_BUTTON_LEFT, InputKey::MOUSE_LEFT},// 开火
        {GLFW_MOUSE_BUTTON_RIGHT, InputKey::MOUSE_RIGHT},// 切换武器
        {GLFW_KEY_Y, InputKey::Y},// 开火
        {GLFW_KEY_U, InputKey::U},// 切换武器
        // 光照控制
        {GLFW_KEY_B, InputKey::B},// B增加红色光照
        {GLFW_KEY_N, InputKey::N},// N增加绿色光照
        {GLFW_KEY_M, InputKey::M},// M增加蓝色光照
        {GLFW_KEY_COMMA, InputKey::COMMA},// ,减小亮度
        {GLFW_KEY_PERIOD, InputKey::PERIOD},// .增加亮度
        // 光线方向控制
        {GLFW_KEY_LEFT_BRACKET, InputKey::LEFT_BRACKET},// [ 控制y方向太阳高度角向-y方向移动
        {GLFW_KEY_RIGHT_BRACKET, InputKey::RIGHT_BRACKET},//  ] 控制y方向太阳高度角向+y方向移动
        {GLFW_KEY_BACKSLASH, InputKey::BACKSLASH},// \\ 控制昼夜变换
        {GLFW_KEY_SEMICOLON, InputKey::SEMICOLON},// ;控制x方向太阳高度角向-x方向移动
        {GLFW_KEY_APOSTROPHE, InputKey::APOSTROPHE},// ' 控制x方向太阳高度角向+x方向移动
        {GLFW_KEY_SLASH, InputKey::SLASH},// / 回到正午时分
        // end of 光照控制
        {GLFW_KEY_LEFT_SHIFT, InputKey::SHIFT},
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
        std::cout << key_[(size_t)InputKey::MOUSE_LEFT] << std::endl;
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

    static void mouseButtonCallback(GLFWwindow* window, int button, int action) {
        Input* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
        if (!input) {
            return;
        }

        if (action == GLFW_REPEAT) {
            return;
        }

        bool pressed = action == GLFW_PRESS;
        InputKey button_enum = Input::glfwKeyToInputKey(button);
        if (button_enum == InputKey::_COUNT) {
            return;
        }

        input->setMouseButtonCallback(button_enum, pressed);
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

    void setMouseButtonCallback(InputKey button, bool is_pressed) {
        key_[(size_t)button] = is_pressed;
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
