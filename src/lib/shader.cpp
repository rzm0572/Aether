#include "shader.h"
#include <sstream>
#include <string>
#include <iostream>
#include <fstream>

Shader::Shader(const char* vertexShader, const char* fragmentShader) {
    std::ifstream vertexShaderFile;
    std::ifstream fragmentShaderFile;

    std::string vertexShaderSource;
    std::string fragmentShaderSource;

    vertexShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fragmentShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        vertexShaderFile.open(vertexShader);
        fragmentShaderFile.open(fragmentShader);

        std::stringstream vertexShaderStream;
        std::stringstream fragmentShaderStream;
        vertexShaderStream << vertexShaderFile.rdbuf();
        fragmentShaderStream << fragmentShaderFile.rdbuf();
        vertexShaderFile.close();
        fragmentShaderFile.close();

        vertexShaderSource = vertexShaderStream.str();
        fragmentShaderSource = fragmentShaderStream.str();

        // std::cout << vertexShaderSource << std::endl;
        // std::cout << fragmentShaderSource << std::endl;
    } catch (std::ifstream::failure exception) {
        std::cout << "ERROR::SHADER::COULD_NOT_READ_SHADER_SOURCE_FILE" << std::endl;
    }

    unsigned int vertexShaderID = setVertexShader(vertexShaderSource.c_str());
    unsigned int fragmentShaderID = setFragmentShader(fragmentShaderSource.c_str());
    linkShaderProgram(vertexShaderID, fragmentShaderID);

    glDeleteShader(vertexShaderID);
    glDeleteShader(fragmentShaderID);
}


unsigned int Shader::setVertexShader(const char* vertexShaderSource) {
    int success;
    char infoLog[512];

    int vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShaderID, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShaderID);

    glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(vertexShaderID, 512, NULL, infoLog);
        std::cout << "\033[31mERROR::SHADER::VERTEX::COMPILATION_FAILED\033[0m\n" << infoLog << std::endl;
    }
    return vertexShaderID;
}

unsigned int Shader::setFragmentShader(const char* fragmentShaderSource) {
    int success;
    char infoLog[512];

    int fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShaderID, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShaderID);

    glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(fragmentShaderID, 512, NULL, infoLog);
        std::cout << "\033[31mERROR::SHADER::FRAGMENT::COMPILATION_FAILED\033[0m\n" << infoLog << std::endl;
    }
    return fragmentShaderID;
}

void Shader::linkShaderProgram(unsigned int vertexShaderID, unsigned int fragmentShaderID) {
    int success;
    char infoLog[512];

    ID = glCreateProgram();
    glAttachShader(ID, vertexShaderID);
    glAttachShader(ID, fragmentShaderID);
    glLinkProgram(ID);

    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(ID, 512, NULL, infoLog);
        std::cout << "\033[31mERROR::SHADER::PROGRAM::LINKING_FAILED\033[0m\n" << infoLog << std::endl;
    }
}

void Shader::useShader() const {
    // std::cout << "Use shader program: " << ID << std::endl;
    glUseProgram(ID);
}
