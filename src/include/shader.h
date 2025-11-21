#pragma once

#include <glad/glad.h>
#include <stdexcept>
#include <string>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <unordered_map>

using ShaderProgramID = unsigned int;

class Shader {
public:
    Shader(): ID(0) {}
    Shader(const char* vertex_shader_path, const char* fragment_shader_path);

    ~Shader() {
        if (ID != 0) {
            glDeleteProgram(ID);
        }
    }

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Shader&& other) {
        ID = other.ID;
        other.ID = 0;
    }

    Shader& operator=(Shader&& other) {
        if (this != &other) {
            if (ID != 0) {
                glDeleteProgram(ID);
            }

            ID = other.ID;
            other.ID = 0;
        }
        return *this;
    }

    void useShader() const;

    template<typename T, typename... Args>
    bool setUniform(const std::string& name, T x, Args... args) const {
        constexpr int count = 1 + sizeof...(args);
        if constexpr (count > 4) {
            static_assert(sizeof(T) == 0, "Invalid number of arguments");
        }

        int location = glGetUniformLocation(ID, name.c_str());
        if (location == -1) {
            throw std::runtime_error("Cannot find uniform variable " + name);
        }

        if constexpr (std::is_same<T, float>::value) {
            if constexpr (count == 1) {
                glUniform1f(location, x);
            } else if constexpr (count == 2) {
                glUniform2f(location, x, args...);
            } else if constexpr (count == 3) {
                glUniform3f(location, x, args...);
            } else {
                glUniform4f(location, x, args...);
            }
        } else if constexpr (std::is_same<T, double>::value) {
            if constexpr (count == 1) {
                glUniform1d(location, x);
            } else if constexpr (count == 2) {
                glUniform2d(location, x, args...);
            } else if constexpr (count == 3) {
                glUniform3d(location, x, args...);
            } else {
                glUniform4d(location, x, args...);
            }
        } else if constexpr (std::is_same<T, int>::value) {
            if constexpr (count == 1) {
                glUniform1i(location, x);
            } else if constexpr (count == 2) {
                glUniform2i(location, x, args...);
            } else if constexpr (count == 3) {
                glUniform3i(location, x, args...);
            } else {
                glUniform4i(location, x, args...);
            }
        } else if constexpr (std::is_same<T, bool>::value) {
            glUniform1i(location, (int)x);
        } else if constexpr (std::is_same<T, glm::vec2>::value) {
            glUniform2fv(location, 1, glm::value_ptr(x));
        } else if constexpr (std::is_same<T, glm::vec3>::value) {
            glUniform3fv(location, 1, glm::value_ptr(x));
        } else if constexpr (std::is_same<T, glm::vec4>::value) {
            glUniform4fv(location, 1, glm::value_ptr(x));
        } else if constexpr (std::is_same<T, glm::mat2>::value) {
            glUniformMatrix2fv(location, 1, GL_FALSE, glm::value_ptr(x));
        } else if constexpr (std::is_same<T, glm::mat3>::value) {
            glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(x));
        } else if constexpr (std::is_same<T, glm::mat4>::value) {
            glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(x));
        } else {
            static_assert(sizeof(T) == 0, "setUniform called with an unsupported type");
        }
        return true;
    }

private:
    unsigned int setVertexShader(const char* vertex_shader_source);
    unsigned int setFragmentShader(const char* fragment_shader_source);
    void linkShaderProgram(unsigned int vertex_shader_ID, unsigned int fragment_shader_ID);

    ShaderProgramID ID;

    friend class ShaderManager;
};

class ShaderManager {
public:
    ShaderManager() = default;
    ~ShaderManager() = default;

    const Shader& getShader(ShaderProgramID id) {
        return shaders_[id];
    }

    const Shader& getShader(const std::string& name) {
        return *shader_registry_.at(name);
    }

    void registerShader(std::string name, const char* vertex_shader_path, const char* fragment_shader_path) {
        Shader shader(vertex_shader_path, fragment_shader_path);
        ShaderProgramID id = shader.ID;
        shaders_[id] = std::move(shader);
        shader_registry_[name] = &shaders_[id];
    }

    void registerShader(std::string name, std::string vertex_shader_path, std::string fragment_shader_path) {
        registerShader(name.c_str(), vertex_shader_path.c_str(), fragment_shader_path.c_str());
    }

private:
    std::unordered_map<ShaderProgramID, Shader> shaders_;
    std::unordered_map<std::string, Shader*> shader_registry_;
};
