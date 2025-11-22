#include "utils/macros.h"
#include "utils/path_handler.h"
#include "resource/shader.h"
#include "window.h"
#include <iostream>
#include <cmath>
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const int vertexAttribLocation = 0;

float vertices2[] = {
    0.5f, 0.5f, 0.0f,
    0.5f, -0.5f, 0.0f,
    -0.5f, -0.5f, 0.0f,
    -0.5f, 0.5f, 0.0f
};

unsigned int indices[] = {
    0, 1, 2,
    0, 3, 2
};


void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

int main() {
    const unsigned int SCR_WIDTH = 800;
    const unsigned int SCR_HEIGHT = 600;

    Window window(SCR_WIDTH, SCR_HEIGHT, "Triangle");
    
    window.setFramebufferSizeCallback([](GLFWwindow* window, int width, int height) {
        glViewport(0, 0, width, height);
    });

    window.makeCurrent();

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    ShaderManager shaderManager;
    shaderManager.registerShader("trans", getShaderPath("trans.vert"), getShaderPath("trans.frag"));
    auto shader = shaderManager.getShader("trans");

    unsigned int VAO, VBO, EBO;
    // Generate vertex array object and vertex buffer object
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // Bind the vertex array object first
    glBindVertexArray(VAO);

    // Bind and set the vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices2), vertices2, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Configure vertex attribute
    glVertexAttribPointer(vertexAttribLocation, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(vertexAttribLocation);

    // Unbind the VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // unbind the VAO
    glBindVertexArray(0);

    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::translate(view, glm::vec3(0.0f, 0.0f, -5.0f));
    
    while (!window.shouldClose()) {
        processInput(window.getWindow());

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shader->useShader();

        float timeValue = glfwGetTime();
        float greenValue = (std::sin(timeValue) / 2.0f) + 0.5f;

        // glm::mat4 trans = glm::mat4(1.0f);
        // trans = glm::rotate(trans, (float)timeValue, glm::vec3(0.0f, 0.0f, 1.0f));

        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -0.05f));

        shader->setUniform("ourColor", 0.0f, greenValue, 0.0f, 1.0f);

        // shader.setUniform("transform", trans);

        shader->setUniform("model", model);
        shader->setUniform("view", view);
        shader->setUniform("projection", projection);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

        // glDrawArrays(GL_TRIANGLES, 0, 3);   // glDrawArrays will read data from the currently bound VAO
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);

        window.swapBuffers();
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    return 0;
}