#pragma once

#include "utils/macros.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <iostream>

namespace {
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

namespace GL {
    class Window {
    public:
        Window() : m_window_(nullptr) {}
        ~Window() {
            if (m_window_) {
                glfwDestroyWindow(m_window_);
            }
        }

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        Window(int width, int height, const char* title, GLFWframebuffersizefun framebuffer_size_callback) {
            create(width, height, title, framebuffer_size_callback);
        }

        void create(int width, int height, const char* title, GLFWframebuffersizefun framebuffer_size_callback) {
            m_window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
            if (!m_window_) {
                throw std::runtime_error("Failed to create GLFW window");
            }
            setFramebufferSizeCallback(framebuffer_size_callback);
        }

        void makeCurrent() {
            glfwMakeContextCurrent(m_window_);
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
    
    private:
        GLFWwindow* m_window_;
    };
}
