#include <vector>
#include "resource/shader.h"
#include <glm/glm.hpp>
#include "service/service_locator.h"
#include "resource/shader.h"
#include "utils/path_handler.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

class HealthBar {
public:    
    /**
     * @brief 构造函数，传入粒子数量，随机种子，贴图路径
     * @param particle_num 粒子数量
     * @param random_seed 随机种子，用于随机生成粒子的方向等
     * @param texture_path 贴图路径
    */
    HealthBar(){

        std::vector<float> vertices;
        std::vector<float> corners={-1.0f,-1.0f,1.0f,-1.0f,1.0f,1.0f,-1.0f,1.0f};
        std::vector<unsigned int> indices;
        for(int i = 0; i < 1; i++){


            for(int j=0;j<4;j++){
                vertices.push_back(corners[j*2]);
                vertices.push_back(corners[j*2+1]);
            }
            for(int j=0;j<3;j++){                
                indices.push_back(i*4+j);
            }
            for(int j=2;j<5;j++){
                indices.push_back(i*4+(j&3));
            }
        }

        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        if (!ServiceLocator<ShaderManager>::get()->isShaderRegistered("healthbar")) {
            ServiceLocator<ShaderManager>::get()->registerShader("healthbar", getShaderPath("particles/healthbar.vert"), getShaderPath("particles/healthbar.frag"));
        }
        glBindVertexArray(0);
    }
    ~HealthBar(){
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
    void draw(glm::vec3 World_pos,glm::mat4 view, glm::mat4 proj, glm::vec3 camera_pos,float scale,float height,float health){
        const Shader* shader = ServiceLocator<ShaderManager>::get()->useShader("healthbar");

        shader->setUniform("uExplosionPos", World_pos);
        shader->setUniform("uView", view);
        shader->setUniform("uProjection", proj);
        shader->setUniform("uCameraPos", camera_pos);
        shader->setUniform("scale", scale);
        shader->setUniform("health",health);
        shader->setUniform("height",height);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

private:

    unsigned int VAO {0};
    unsigned int VBO {0};
    unsigned int EBO {0};
};

