#pragma once

#include <glad/glad.h>
#include <stdexcept>
#include <string>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <unordered_map>

namespace GL {
    using ShaderProgramID = unsigned int;

    class Shader {
    public:
        Shader(): ID(0) {}
        Shader(const char* vertexShaderPath, const char* fragmentShaderPath);

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

            // if constexpr (std::is_same<T, float>::value && count == 4) {
            //     std::cout << ID << " " << name << std::endl;
            // }

            int location = glGetUniformLocation(ID, name.c_str());
            if (location == -1) {
                throw std::runtime_error("Cannot find uniform variable " + name);
                // std::cout << "\033[31mCannot find uniform variable " << name << "\033[0m" << std::endl;
                // return false;
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
        unsigned int setVertexShader(const char* vertexShaderSource);
        unsigned int setFragmentShader(const char* fragmentShaderSource);
        void linkShaderProgram(unsigned int vertexShaderID, unsigned int fragmentShaderID);

        ShaderProgramID ID;

        friend class ShaderManager;
    };

    class ShaderManager {
    public:
        ShaderManager() = default;
        ~ShaderManager() = default;

        const Shader& getShader(ShaderProgramID id) {
            return shaders[id];
        }

        const Shader& getShader(const std::string& name) {
            return *shaderRegistry.at(name);
        }

        void registerShader(std::string name, const char* vertexShaderPath, const char* fragmentShaderPath) {
            Shader shader(vertexShaderPath, fragmentShaderPath);
            ShaderProgramID id = shader.ID;
            shaders[id] = std::move(shader);
            shaderRegistry[name] = &shaders[id];
        }

        void registerShader(std::string name, std::string vertexShaderPath, std::string fragmentShaderPath) {
            registerShader(name.c_str(), vertexShaderPath.c_str(), fragmentShaderPath.c_str());
        }

    private:
        std::unordered_map<ShaderProgramID, Shader> shaders;
        std::unordered_map<std::string, Shader*> shaderRegistry;
    };
}
