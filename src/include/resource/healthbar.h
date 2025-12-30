#include <vector>
#include "resource/shader.h"
#include <glm/glm.hpp>
#include "service/service_locator.h"
#include "resource/shader.h"
#include "utils/path_handler.h"
#include "resource/texture.h"

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <string.h>
class HealthBar{
public:    
    /**
     * @brief 构造函数，传入粒子数量，随机种子，贴图路径
     * @param particle_num 粒子数量
     * @param random_seed 随机种子，用于随机生成粒子的方向等
     * @param texture_path 贴图路径
     * 具体设计思路如下
    */
    HealthBar(){

        std::vector<float> vertices;// 顶点数组
        std::vector<float> corners={-1.0f,-1.0f,1.0f,-1.0f,1.0f,1.0f,-1.0f,1.0f};// 四个顶点
        std::vector<unsigned int> indices;// 索引数组
        for(int i=0;i<1;i++){// 初始化粒子，对于每个粒子只要初始化一遍即可


            for(int j=0;j<4;j++){
                // 顶点数两位是顶点位置
                vertices.push_back(corners[j*2]);
                vertices.push_back(corners[j*2+1]);
            }
            // 索引数组，每个粒子有六个顶点，即两个三角形片元，每个顶点有两个坐标
            for(int j=0;j<3;j++){                
                indices.push_back(i*4+j);
            }
            for(int j=2;j<5;j++){
                indices.push_back(i*4+(j&3));
            }
        }
        // 顶点数组对象
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
        // 顶点缓冲对象
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
        // 索引缓冲对象
        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        // 顶点属性指针
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // 着色器
        if (!ServiceLocator<ShaderManager>::get()->isShaderRegistered("healthbar")) {
            // 注册着色器
            ServiceLocator<ShaderManager>::get()->registerShader("healthbar", getShaderPath("particles/healthbar.vert"), getShaderPath("particles/healthbar.frag"));
        }
        // 解绑
        glBindVertexArray(0);
    }
    ~HealthBar(){
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
    void draw(glm::vec3 World_pos,glm::mat4 view, glm::mat4 proj, glm::vec3 camera_pos,float scale,float height,float health){

        // 启动着色器
        const Shader* shader = ServiceLocator<ShaderManager>::get()->useShader("healthbar");

        // 设置uniform变量


        shader->setUniform("uExplosionPos", World_pos);// 设定中心位置
        shader->setUniform("uView", view);// 设定视角矩阵
        shader->setUniform("uProjection", proj);// 设定投影矩阵
        shader->setUniform("uCameraPos", camera_pos);// 设定第三人称摄像机位置，用于实现billboard效果
        shader->setUniform("scale", scale);// 设定火球半径eed);
        shader->setUniform("health",health);
        shader->setUniform("height",height);

        // 绘制
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);// 因为一个粒子四个顶点
        glBindVertexArray(0);
    }

private:

    unsigned int VAO=0;// 顶点数组对象
    unsigned int VBO=0;// 顶点缓冲对象
    unsigned int EBO=0;// 索引缓冲对象
};

