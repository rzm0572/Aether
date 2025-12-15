#pragma once

#include "utils/macros.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <iostream>

namespace {
    // GLFW context life manager
    // This class ensures that GLFW is initialized and terminated only once.
    // The globalInit() function will be called when the program starts and the static object is created, and globalTerminate() when it is destroyed, just before the program exits.
    static class GLFWContext {
    public:
        GLFWContext() {
            globalInit();
        }

        GLFWContext(const GLFWContext&) = delete;
        GLFWContext& operator=(const GLFWContext&) = delete;

        ~GLFWContext() {
            globalTerminate();
        }

        static void globalInit() {
            std::cout << "Starting GLFW context, OpenGL " TO_STR(OPENGL_VERSION_MAJOR) "." TO_STR(OPENGL_VERSION_MINOR) << std::endl;
            glfwInit();
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, OPENGL_VERSION_MAJOR);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, OPENGL_VERSION_MINOR);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

#ifdef _DEBUG
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif
        }

        static void globalTerminate() {
            std::cout << "Terminating GLFW context" << std::endl;
            glfwTerminate();
        }
    } GLFWContextLifeManager;
}

// This class is a wrapper for GLFW windows
// It provides a simple way to manage GLFW window pointers and ensure that they are properly initialized and terminated.
class Window {
public:
    Window() : m_window_(nullptr) {}
    ~Window() {
        if (m_window_) {
            glfwDestroyWindow(m_window_);
        }
    }

    // Window is not copyable
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    Window(int width, int height, const char* title) {
        create(width, height, title);
    }

    void create(int width, int height, const char* title) {
        m_window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (!m_window_) {
            throw std::runtime_error("Failed to create GLFW window");
        }
    }

    void makeCurrent() {
        glfwMakeContextCurrent(m_window_);
    }

    void setWindowUserPointer(void* user_pointer) {
        glfwSetWindowUserPointer(m_window_, user_pointer);
    }

    void setKeyCallback(GLFWkeyfun callback) {
        glfwSetKeyCallback(m_window_, callback);
    }

    void setCursorPosCallback(GLFWcursorposfun callback) {
        glfwSetInputMode(m_window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetCursorPosCallback(m_window_, callback);
    }

    void setFramebufferSizeCallback(GLFWframebuffersizefun callback) {
        glfwSetFramebufferSizeCallback(m_window_, callback);
    }

    void swapBuffers() {
        glfwSwapBuffers(m_window_);
    }

    GLFWwindow* getWindow() {
        return m_window_;
    }

    bool shouldClose() {
        return glfwWindowShouldClose(m_window_);
    }

    void setWindowShouldClose() {
        glfwSetWindowShouldClose(m_window_, GLFW_TRUE);
    }

    void getFramebufferSize(int& width, int& height) {
        glfwGetFramebufferSize(m_window_, &width, &height);
    }

private:
    GLFWwindow* m_window_;
};
