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
/*


*/
// 火球效果
// TODO: 对于不同爆炸使用不同颜色的粒子，即需要传给着色器的颜色参数不同，但是目前颜色是写死在着色器里的
class Particle_Fireball {
    /**
     * @brief 粒子系统，就是制造大量粒子，并渲染
     * @example 首先，你需要设定粒子数量，随机种子，贴图路径等参数，使用构造函数创建一个Particle_Fireball对象
     *          例如 Particle_Fireball fireball(1000, 1234, "fireball.png");就是创建一个1000个粒子的火球，随机种子为1234，贴图路径为"fireball.png"
     * @example 如果你想要从头开始展现火球形成的效果，你需要调用fireball.start_()函数，重置粒子系统
     *          例如：fireball.start_();
     * @example 然后，你需要在渲染循环中调用fireball.draw()函数，渲染火球效果
     *          例如：fireball.draw(plane->getTransformComponent().getPosition(),view, projection, third_person_camera.getPosition(),3.0f,3.0f,0.1f);
     *          就是将火球中心设定为 plane->getTransformComponent().getPosition()，视角设定为 view，投影设定为 projection，
     *          第三人称摄像机位置设定为 third_person_camera.getPosition()，这个参数是为了实现billboard效果，即粒子始终朝向摄像机
     *          火球半径设定为3.0个单位，火球每个粒子飞行时间设定为3.0秒，火球粒子大小设定为0.1f
    */
public:    
    /**
     * @brief 构造函数，传入粒子数量，随机种子，贴图路径
     * @param particle_num 粒子数量
     * @param random_seed 随机种子，用于随机生成粒子的方向等
     * @param texture_path 贴图路径
     * 具体设计思路如下
    */
    Particle_Fireball(int particle_num,
                      unsigned int random_seed,
                        const std::string& texture_path){
        // 加载纹理
        int width, height, channels;
        unsigned char* data = stbi_load(texture_path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!data) {
            this->loaded_texture = false;
            std::cout << "Failed to load texture in"<<texture_path << std::endl;

        }
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        // 设置纹理参数
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // 上传纹理数据
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        stbi_image_free(data);


        /*
        每个粒子实际上是一个有贴图的方块，因此我们要处理方块信息

        这一部分我们给每个点都设置一组参数：
        TODO: 你可以按照以下的注释修改参数，达到不同的效果
        0~0 ：粒子序号
        1~2 ：顶点位置，因为我们通过billboard方式使得二维坐标正对眼，所以位置只要二维
        3~5 ：预留偏移量，暂时用0 TODO: 通过偏移量增加视觉效果
        6~6 ：延迟时间，随机分布于 [0,1] 作用是在粒子的生命周期中随机时刻发射粒子
        7~9 ：速度，随机分布于球面上，作用是粒子的运动方向 TODO: 或许可以考虑不同初速度
        
        每个粒子实际上是一个有贴图的方块，因此索引就是方块位置
        之后我们把数据存入GPU，并绑定到VAO/VBO/EBO上
        */
        srand(random_seed);
        this->particle_num = particle_num;
        std::vector<float> vertices;// 顶点数组
        std::vector<float> corners={-1.0f,-1.0f,1.0f,-1.0f,1.0f,1.0f,-1.0f,1.0f};// 四个顶点
        std::vector<unsigned int> indices;// 索引数组
        for(int i=0;i<particle_num;i++){// 初始化粒子，对于每个粒子只要初始化一遍即可
            float Time_bias=static_cast<float>(rand()%10000)/10000.0f;
            // 在球面随机一个速度
            float V_x=(static_cast<float>(rand()%10000)/10000.0f)*(rand()%2*2-1);
            float V_y=(static_cast<float>(rand()%10000)/10000.0f)*(rand()%2*2-1);
            float V_z=(static_cast<float>(rand()%10000)/10000.0f)*(rand()%2*2-1);
            float V_len=glm::length(glm::vec3(V_x,V_y,V_z));
            if(V_len>=0.000001f){
                V_x/=V_len;
                V_y/=V_len;
                V_z/=V_len;
            }

            V_len = static_cast<float>(rand()%10000)/12500.0f+0.6f;
            V_x*=V_len;
            V_y*=V_len;
            V_z*=V_len;
            // 四个顶点
            for(int j=0;j<4;j++){
                // 第一位是粒子序号，粒子序号暂时用于随机种子，用随机算法标识粒子的发射方向
                vertices.push_back(static_cast<float>(i));
                // 顶点数两位是顶点位置
                vertices.push_back(corners[j*2]);
                vertices.push_back(corners[j*2+1]);
                // 预留三位做发射偏移量
                vertices.push_back(0.0f);
                vertices.push_back(0.0f);
                vertices.push_back(0.0f);
                // 后面颜色或许可以做不一样的

                // 有一个延迟时间，即生命周期中可以等一会才出现
                vertices.push_back(Time_bias);
                // 速度
                vertices.push_back(V_x);
                vertices.push_back(V_y);
                vertices.push_back(V_z);
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
        // 粒子序号
        glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // 顶点位置(二维坐标，之后会对准相机的)
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,10 * sizeof(float), (void*)(1 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // 预留发射偏移量
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        // 延迟发射时间
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(3);
        // 初始速度
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(7 * sizeof(float)));
        glEnableVertexAttribArray(4);

        // 着色器
        ServiceLocator<ShaderManager>::get()->registerShader("fireball", getShaderPath("particles/fireball.vert"), getShaderPath("particles/fireball.frag"));
        // 解绑
        glBindVertexArray(0);
    }
    ~Particle_Fireball(){
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
    /**
     * @brief 开始绘制
     * @param World_pos 火球中心位置
     * @param view 视角矩阵
     * @param proj 投影矩阵
     * @param camera_pos 第三人称摄像机位置，这是为了实现 billboard 效果
     * @param scale 火球半径
     * @param life_time 火球生命周期，单位秒
     * @param max_size 粒子最大尺寸，用于控制粒子大小
    */
    void draw(glm::vec3 World_pos, glm::mat4 view, glm::mat4 proj, glm::vec3 camera_pos,float scale=3.0f,float life_time = 3.0f,float max_size = 0.6f){
        // 在绘制粒子前开启混合
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // 标准透明混合
        float time_now = glfwGetTime();
        // if (time_now - start_time > life_time) {
        //     end_explosion();
        // }
        if(is_exploding){
            // 启动着色器
            const Shader* shader = ServiceLocator<ShaderManager>::get()->useShader("fireball");

            // 设置uniform变量

            shader->setUniform("uTime", time_now);// 设定时间，用于计算经过了多长时间，控制粒子移动距离
            shader->setUniform("uExplosionTime", start_time);// 设定爆炸开始时间，用于控制粒子开始飞行时间
            shader->setUniform("uExplosionPos", World_pos);// 设定爆炸中心位置
            shader->setUniform("uView", view);// 设定视角矩阵
            shader->setUniform("uProjection", proj);// 设定投影矩阵
            shader->setUniform("uCameraPos", camera_pos);// 设定第三人称摄像机位置，用于实现billboard效果
            shader->setUniform("scale", scale);// 设定火球半径
            shader->setUniform("life_time", life_time);// 设定粒子能飞多长时间
            shader->setUniform("Max_size", max_size);// 设定粒子最大尺寸

            // TODO:绑定纹理
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureID);
            shader->setUniform("uSpriteTex", 0); 

            // 绘制
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, particle_num*6, GL_UNSIGNED_INT, 0);// 因为一个粒子四个顶点
            glBindVertexArray(0);
        }
        else{
            std::cout<<"not exploding"<<std::endl;
        }
    }
    void start_(){
        start_time = glfwGetTime();
        is_exploding = true;
    }
    void end_(){
        is_exploding = false;
    }
    bool exists_now(){
        return is_exploding;
    }
private:


    int particle_num;// 粒子数量，越高理论上越精致，但是资源消耗增加
    unsigned int VAO=0;// 顶点数组对象
    unsigned int VBO=0;// 顶点缓冲对象
    unsigned int EBO=0;// 索引缓冲对象
    unsigned int textureID = 0;// 纹理对象
    float start_time=0;// 爆炸开始时间
    bool loaded_texture = 1;
    bool is_exploding=false;// 是否正在爆炸
};




// 尾焰和尾迹效果，唯一的不同是构造的时候需要输入默认的相对于物体的速度，绘制时需要输入物体的速度，因为我们这里用物体相反速度+默认速度来实现尾焰和尾迹效果
// 而且构造的时候需要输入radius_bias 表示尾迹的半径
// TODO: 对于不同爆炸使用不同颜色的粒子，即需要传给着色器的颜色参数不同，但是目前颜色是写死在着色器里的
/**
 * @brief 尾焰和尾迹效果，唯一的不同是构造的时候需要输入默认的相对于物体的速度，以及尾迹的半径，绘制时需要输入物体的速度，因为我们这里用物体相反速度+默认速度来实现尾焰和尾迹效果
 * @example 首先，你需要设定粒子数量，随机种子，贴图路径等参数，使用构造函数创建一个Particle_Bullet对象
 *          例如 Particle_Bullet bullet(10000, 42, getAssetPath("textures/particles/particle200.png"),glm::vec3(0.0f, 0.0f, 0.0f),1.0f);
 *          就是创建一个10000个粒子的尾迹，随机种子为42，贴图路径为"particle200.png"，相对于物体的速度为(0,0,0)，尾迹半径为1.0f
 * @example 如果你想要从头开始展现尾迹形成的效果，你需要调用bullet.start_()函数，重置粒子系统
 *          例如：bullet.start_();
 * @example 然后，你需要在渲染循环中调用bullet.draw()函数，渲染尾迹效果
 *          例如：bullet.draw(plane->getTransformComponent().getPosition()-glm::vec3(3.0f,1.0f,0.0f), view, projection, third_person_camera.getPosition(), 4.0f, 1.5f, 0.1f,velocity);
 *          这里的plane是你要渲染尾迹的物体，view, projection, third_person_camera.getPosition()分别是视角矩阵，投影矩阵，第三人称摄像机位置
 *          接下来三个参数是尾迹长度，粒子生命周期，粒子最大尺寸
 *          最后一个参数velocity是物体速度，用于实现尾焰和尾迹效果，相反的步骤已经在着色器中实现
 * 
*/
class Particle_Bullet {

public:    
    /**
     * @brief 构造函数，传入粒子数量，随机种子，贴图路径
     * @param particle_num 粒子数量
     * @param random_seed 随机种子，用于随机生成粒子的方向等
     * @param texture_path 贴图路径
     * @param origin_v 相对于物体的速度，用于实现尾焰和尾迹效果
     * @param radius_bias 尾迹的半径，用于控制尾迹的大小
     * 具体设计思路如下
    */
    Particle_Bullet(int particle_num,
                      unsigned int random_seed,
                        const std::string& texture_path,
                        glm::vec3 origin_v, float radius_bias){
        // 加载纹理
        int width, height, channels;
        unsigned char* data = stbi_load(texture_path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!data) {
            this->loaded_texture = false;
            std::cout << "Failed to load texture in"<<texture_path << std::endl;

        }
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        // 设置纹理参数
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // 上传纹理数据
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        stbi_image_free(data);


        /*
        每个粒子实际上是一个有贴图的方块，因此我们要处理方块信息

        这一部分我们给每个点都设置一组参数：
        TODO: 你可以按照以下的注释修改参数，达到不同的效果
        0~0 ：粒子序号
        1~2 ：顶点位置，因为我们通过billboard方式使得二维坐标正对眼，所以位置只要二维
        3~5 ：预留偏移量，这里增加一个相对于物体的位移
        6~6 ：延迟时间，随机分布于 [0,1] 作用是在粒子的生命周期中随机时刻发射粒子
        7~9 ：速度，这里直接用给出的速度

        每个粒子实际上是一个有贴图的方块，因此索引就是方块位置
        之后我们把数据存入GPU，并绑定到VAO/VBO/EBO上
        */
        srand(random_seed);
        this->particle_num = particle_num;
        std::vector<float> vertices;// 顶点数组
        std::vector<float> corners={-1.0f,-1.0f,1.0f,-1.0f,1.0f,1.0f,-1.0f,1.0f};// 四个顶点
        std::vector<unsigned int> indices;// 索引数组
        for(int i=0;i<particle_num;i++){// 初始化粒子，对于每个粒子只要初始化一遍即可
            float Time_bias=static_cast<float>(rand()%10000)/10000.0f;
            // 在球面随机一个速度
            // float V_x=(static_cast<float>(rand()%10000)/10000.0f)*(rand()%2*2-1);
            // float V_y=(static_cast<float>(rand()%10000)/10000.0f)*(rand()%2*2-1);
            // float V_z=(static_cast<float>(rand()%10000)/10000.0f)*(rand()%2*2-1);
            float V_x=origin_v.x;
            float V_y=origin_v.y;
            float V_z=origin_v.z;
            float V_len=glm::length(glm::vec3(V_x,V_y,V_z));
            if(V_len>0.0000001f){
                V_x/=V_len;
                V_y/=V_len;
                V_z/=V_len;
            }

            V_len = static_cast<float>(rand()%10000)/12500.0f+0.6f;
            V_x*=V_len;
            V_y*=V_len;
            V_z*=V_len;
            // 四个顶点
            for(int j=0;j<4;j++){
                // 第一位是粒子序号，粒子序号暂时用于随机种子，用随机算法标识粒子的发射方向
                vertices.push_back(static_cast<float>(i));
                // 顶点数两位是顶点位置
                vertices.push_back(corners[j*2]);
                vertices.push_back(corners[j*2+1]);
                // 预留三位做发射偏移量
                vertices.push_back(static_cast<float>(rand()%100000)/100000.0f*radius_bias);
                vertices.push_back(static_cast<float>(rand()%100000)/100000.0f*radius_bias);
                vertices.push_back(static_cast<float>(rand()%100000)/100000.0f*radius_bias);
                // 后面颜色或许可以做不一样的

                // 有一个延迟时间，即生命周期中可以等一会才出现
                vertices.push_back(Time_bias);
                // 速度
                vertices.push_back(V_x);
                vertices.push_back(V_y);
                vertices.push_back(V_z);
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
        // 粒子序号
        glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // 顶点位置(二维坐标，之后会对准相机的)
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,10 * sizeof(float), (void*)(1 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // 预留发射偏移量
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        // 延迟发射时间
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(3);
        // 初始速度
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(7 * sizeof(float)));
        glEnableVertexAttribArray(4);

        // 着色器
        ServiceLocator<ShaderManager>::get()->registerShader("bullet", getShaderPath("particles/bullet.vert"), getShaderPath("particles/bullet.frag"));
        // 解绑
        glBindVertexArray(0);
    }
    ~Particle_Bullet(){
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
    /**
     * @brief 开始绘制
     * @param World_pos 火球中心位置
     * @param view 视角矩阵
     * @param proj 投影矩阵
     * @param camera_pos 第三人称摄像机位置，这是为了实现 billboard 效果
     * @param scale 火球半径
     * @param life_time 火球生命周期，单位秒
     * @param max_size 粒子最大尺寸，用于控制粒子大小
     * @param element_v 物体的速度，用于实现尾焰和尾迹效果
    */
    void draw(glm::vec3 World_pos, glm::mat4 view, glm::mat4 proj, glm::vec3 camera_pos,
                float scale,float life_time,float max_size,
                glm::vec3 element_v){
        // 在绘制粒子前开启混合
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // 标准透明混合
        float time_now = glfwGetTime();
        // if (time_now - start_time > life_time) {
        //     end_explosion();
        // }
        if(is_exploding){
            // 启动着色器
            const Shader* shader = ServiceLocator<ShaderManager>::get()->useShader("bullet");

            // 设置uniform变量

            shader->setUniform("uTime", time_now);// 设定时间，用于计算经过了多长时间，控制粒子移动距离
            shader->setUniform("uExplosionTime", start_time);// 设定爆炸开始时间，用于控制粒子开始飞行时间
            shader->setUniform("uExplosionPos", World_pos);// 设定爆炸中心位置
            shader->setUniform("uView", view);// 设定视角矩阵
            shader->setUniform("uProjection", proj);// 设定投影矩阵
            shader->setUniform("uCameraPos", camera_pos);// 设定第三人称摄像机位置，用于实现billboard效果
            shader->setUniform("scale", scale);// 设定火球半径
            shader->setUniform("life_time", life_time);// 设定粒子能飞多长时间
            shader->setUniform("Max_size", max_size);// 设定粒子最大尺寸
            shader->setUniform("element_Velocity", element_v);// 设定粒子相对物体的速度（实际输入的就是物体的速度）

            // TODO:绑定纹理
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureID);
            shader->setUniform("uSpriteTex", 0); 

            // 绘制
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, particle_num*6, GL_UNSIGNED_INT, 0);// 因为一个粒子四个顶点
            glBindVertexArray(0);
        }
        else{
            std::cout<<"not exploding"<<std::endl;
        }
    }
    void start_(){
        start_time = glfwGetTime();
        is_exploding = true;
    }
    void end_(){
        is_exploding = false;
    }
    bool exists_now(){
        return is_exploding;
    }
private:


    int particle_num;// 粒子数量，越高理论上越精致，但是资源消耗增加
    unsigned int VAO=0;// 顶点数组对象
    unsigned int VBO=0;// 顶点缓冲对象
    unsigned int EBO=0;// 索引缓冲对象
    unsigned int textureID = 0;// 纹理对象
    float start_time=0;// 爆炸开始时间
    bool loaded_texture = 1;
    bool is_exploding=false;// 是否正在爆炸
};


// TODO: Ribbon 效果