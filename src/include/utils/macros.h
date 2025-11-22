#pragma once

#include <glad/glad.h>
#include <iostream>
#include <string>

#define _CAT(a, b) a##b
#define CAT(a, b) _CAT(a, b)

#define _TO_STR(a) #a
#define TO_STR(a) _TO_STR(a)

#define OPENGL_VERSION_MAJOR 4
#define OPENGL_VERSION_MINOR 1

#define INVALID_VAO 0
#define INVALID_VBO 0
#define INVALID_EBO 0

#define INVALID_MATERIAL 0xFFFFFFFF

inline GLenum glCheckError_(const char* file, int line) {
    GLenum error_code;
    bool has_error = false;
    while ((error_code = glGetError()) != GL_NO_ERROR) {
        has_error = true;
        std::string error;
        switch (error_code) {
            case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
            case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }
        std::cout << "\033[31m" << error << "\033[0m" << " | " << file << " (" << line << ")" << "\033[0m" << std::endl;
    }
    if (!has_error) {
        std::cout << "\033[32m" << "NO_ERROR" << "\033[0m" << " | " << file << " (" << line << ")" << std::endl;
    }
    return error_code;
}

#define glCheckError() glCheckError_(__FILE__, __LINE__)

