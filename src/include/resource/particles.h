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



class Particle_Explosion {
    /**
     * @example 首先，你需要设定粒子数量，随机种子，贴图路径等参数，使用构造函数创建一个Particle_Explosion对象
     *          例如 Particle_Explosion explosion(1000, 1234, "explosion.png");就是创建一个1000个粒子的爆炸，随机种子为1234，贴图路径为"explosion.png"
     * @example 如果你想要从头开始展现爆炸形成的效果，你需要调用explosion.start_()函数，重置粒子系统
     *          例如：explosion.start_();
     * @example 然后，你需要在渲染循环中调用explosion.draw()函数，渲染爆炸效果
     *          例如：explosion.draw(plane->getTransformComponent().getPosition(),view, projection, third_person_camera.getPosition(),3.0f,3.0f,0.1f);
     *          就是将爆炸中心设定为 plane->getTransformComponent().getPosition()，视角设定为 view，投影设定为 projection，
     *          第三人称摄像机位置设定为 third_person_camera.getPosition()，这个参数是为了实现billboard效果，即粒子始终朝向摄像机
     *          爆炸半径设定为3.0个单位，爆炸每个粒子飞行时间设定为3.0秒，爆炸粒子大小设定为0.1f
    */
public:    
    /**
     * @brief 构造函数，传入粒子数量，随机种子，贴图路径
     * @param particle_num 粒子数量
     * @param random_seed 随机种子，用于随机生成粒子的方向等
     * @param texture_path 贴图路径
     * 具体设计思路如下
    */
    Particle_Explosion(int particle_num,int mars_num,
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
        // 多个火球的爆心、起爆时间需要初始化
        std::vector<float> offset_x_option;
        std::vector<float>  offset_y_option;
        std::vector<float> offset_z_option;
        std::vector<float> time_bias_option;
        for(int i=0;i<6;i++){
            // 随机生成火球的爆心
            float offset_x = (static_cast<float>(rand()%10000)/10000.0f-0.5f);
            float offset_y = (static_cast<float>(rand()%10000)/10000.0f-0.5f);
            float offset_z = (static_cast<float>(rand()%10000)/10000.0f-0.5f);
            offset_x_option.push_back(offset_x * 0.5);
            offset_y_option.push_back(offset_y * 0.5);
            offset_z_option.push_back(offset_z * 0.5);
            // 随机生成火球的起爆时间
            float Time_bias=static_cast<float>(rand()%10000)/10000.0f;
            time_bias_option.push_back(Time_bias* 0.1f);
        }

        // 火星的方向，起爆时间需要初始化
        std::vector<float> mars_Velocity_x_option;
        std::vector<float> mars_Velocity_y_option;
        std::vector<float> mars_Velocity_z_option;
        std::vector<float> mars_Time_bias_option;
        for(int i=0;i<15;i++){// 随机 15 个速度方向产生火星
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
            mars_Velocity_x_option.push_back(V_x);
            mars_Velocity_y_option.push_back(V_y);
            mars_Velocity_z_option.push_back(V_z);

            // 随机生成火星的起爆时间
            float Time_bias=static_cast<float>(rand()%10000)/10000.0f;
            mars_Time_bias_option.push_back(Time_bias*0.2f);
        }

        for(int i=0;i<particle_num;i++){// 初始化粒子，对于每个粒子只要初始化一遍即可
            float Time_bias=static_cast<float>(rand()%10000)/10000.0f;// 随机生成一个发射时间，在 [0,1] 之间
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

            // V_len = static_cast<float>(rand()%10000)/12500.0f+0.6f;// 随机生成粒子速度，这样不会都在表面上
            // V_x*=V_len;
            // V_y*=V_len;
            // V_z*=V_len;
            // float offset_x = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.3f;
            // float offset_y = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.3f;
            // float offset_z = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.3f;
            int offset_index = rand()%(offset_x_option.size());
            float offset_x = offset_x_option[offset_index];
            float offset_y = offset_y_option[offset_index];
            float offset_z = offset_z_option[offset_index];
            // 四个顶点
            float RandNum = static_cast<float>(rand()%10000)/10000.0f;
            int mars_index = rand()%mars_Velocity_x_option.size();
            for(int j=0;j<4;j++){
                // 第一位是预制菜随机数，在 [0,1] 之间
                vertices.push_back(RandNum);
                // 顶点数两位是顶点位置
                vertices.push_back(corners[j*2]);
                vertices.push_back(corners[j*2+1]);

                // 后面颜色或许可以做不一样的

                
                
                // 速度
                if(i<mars_num){// 属于火星的粒子
                    // 火星起爆位置在中心
                    vertices.push_back(0.0f);
                    vertices.push_back(0.0f);
                    vertices.push_back(0.0f);
                    // 爆炸有先后，第一个表示爆炸之后才会飞溅，第二个参数表示随机一个时间用于制造尾迹，第三项表示不同的火星开启时间不同
                    vertices.push_back(0.1f + Time_bias*0.7f + mars_Time_bias_option[mars_index]*1.5f);                   
                    vertices.push_back(mars_Velocity_x_option[mars_index]);
                    vertices.push_back(mars_Velocity_y_option[mars_index]);
                    vertices.push_back(mars_Velocity_z_option[mars_index]);
                    vertices.push_back(1.0);// 种类为火星
                }
                else{// 不属于火星的粒子
                    // 多个火球中心位置随机
                    vertices.push_back(offset_x);
                    vertices.push_back(offset_y);
                    vertices.push_back(offset_z);
                    // 爆炸有先后，第一个表示爆炸层次，第二项表示不同的火球开启时间不同
                    vertices.push_back(Time_bias * 0.3f + time_bias_option[offset_index]);
                    vertices.push_back(V_x);
                    vertices.push_back(V_y);
                    vertices.push_back(V_z);
                    vertices.push_back(0);// 种类为非火星                    
                }
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
        glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // 顶点位置(二维坐标，之后会对准相机的)
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,11 * sizeof(float), (void*)(1 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // 预留发射偏移量
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        // 延迟发射时间
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(3);
        // 初始速度
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(7 * sizeof(float)));
        glEnableVertexAttribArray(4);
        // 粒子种类
        glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(10 * sizeof(float)));
        glEnableVertexAttribArray(5);

        // 着色器
        ServiceLocator<ShaderManager>::get()->registerShader("explosion", getShaderPath("particles/explosion.vert"), getShaderPath("particles/explosion.frag"));
        // 解绑
        glBindVertexArray(0);
    }
    ~Particle_Explosion(){
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
        float time_now = glfwGetTime();
        if (time_now - start_time > 2*life_time) {
            end_();
        }
        if(is_exploding){
            // 在绘制粒子前开启混合
            glEnable(GL_BLEND);
            // glDepthMask(GL_FALSE);// 关闭深度
            // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // 标准透明混合
            glBlendFunc(GL_SRC_ALPHA, GL_ONE); // 加深透明度混合，这是为了实现爆炸的亮度变化
            // 启动着色器
            const Shader* shader = ServiceLocator<ShaderManager>::get()->useShader("explosion");

            // 设置uniform变量

            shader->setUniform("uTime", time_now);// 设定时间，用于计算经过了多长时间，控制粒子移动距离
            shader->setUniform("uExplosionTime", start_time);// 设定爆炸开始时间，用于控制粒子开始飞行时间
            shader->setUniform("uExplosionPos", World_pos);// 设定爆炸中心位置
            shader->setUniform("uView", view);// 设定视角矩阵
            shader->setUniform("uProjection", proj);// 设定投影矩阵
            shader->setUniform("uCameraPos", camera_pos);// 设定第三人称摄像机位置，用于实现billboard效果
            shader->setUniform("scale", scale);// 设定火球半径
            shader->setUniform("raw_life_time", life_time);// 设定粒子能飞多长时间
            shader->setUniform("Max_size", max_size);// 设定粒子最大尺寸

            // TODO:绑定纹理
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureID);
            shader->setUniform("uSpriteTex", 0); 

            // 绘制
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, particle_num*6, GL_UNSIGNED_INT, 0);// 因为一个粒子四个顶点
            glBindVertexArray(0);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // 还原混合模式
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
 * @example 首先，你需要设定粒子数量，随机种子，贴图路径等参数，使用构造函数创建一个Particle_Ribbon对象
 *          例如 Particle_Ribbon ribbon(10000, 42, getAssetPath("textures/particles/particle200.png"),glm::vec3(0.0f, 0.0f, 0.0f),1.0f);
 *          就是创建一个10000个粒子的尾迹，随机种子为42，贴图路径为"particle200.png"，相对于物体的速度为(0,0,0)，尾迹半径为1.0f
 * @example 如果你想要从头开始展现尾迹形成的效果，你需要调用ribbon.start_()函数，重置粒子系统
 *          例如：ribbon.start_();
 * @example 然后，你需要在渲染循环中调用ribbon.draw()函数，渲染尾迹效果
 *          例如：ribbon.draw(plane->getTransformComponent().getPosition()-glm::vec3(3.0f,1.0f,0.0f), view, projection, third_person_camera.getPosition(), 4.0f, 1.5f, 0.1f,velocity);
 *          这里的plane是你要渲染尾迹的物体，view, projection, third_person_camera.getPosition()分别是视角矩阵，投影矩阵，第三人称摄像机位置
 *          接下来三个参数是尾迹长度，粒子生命周期，粒子最大尺寸
 *          最后一个参数velocity是物体速度，用于实现尾焰和尾迹效果，相反的步骤已经在着色器中实现
 * 
*/
class Particle_Flareback {

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
    Particle_Flareback(int particle_num,
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
        ServiceLocator<ShaderManager>::get()->registerShader("flareback", getShaderPath("particles/flareback.vert"), getShaderPath("particles/flareback.frag"));
        // 解绑
        glBindVertexArray(0);
    }
    ~Particle_Flareback(){
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
            const Shader* shader = ServiceLocator<ShaderManager>::get()->useShader("flareback");

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


// Ribbon 效果
class Particle_Ribbon {
public:
    struct Particle {
        glm::vec3 pos;         // 世界位置
        float time;            // 发射时间
        glm::vec2 offset;      // 横向偏移（在局部圆内随机）
        float spread_rate;     // 扩散速率（可调）
        float life_time;       // 总寿命
    };

    Particle_Ribbon(int max_particles,
                    const std::string& texture_path,
                    float initial_width = 0.5f,
                    float max_width = 2.0f,
                    float life_time = 3.0f)
        : max_particles(max_particles),
          initial_width(initial_width),
          max_width(max_width),
          life_time(life_time) {

        // 加载纹理
        int w, h, ch;
        unsigned char* data = stbi_load(texture_path.c_str(), &w, &h, &ch, STBI_rgb_alpha);
        if (!data) {
            loaded_texture = false;
            std::cerr << "Failed to load ribbon texture: " << texture_path << std::endl;
            return;
        }
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);

        // 初始化粒子缓冲区
        particles.resize(max_particles);
        for(auto& p : particles){
            p.pos = glm::vec3(-1000.0f, -1000.0f, -1000.0f);
            p.time = 0.0f;
            p.offset = glm::vec2(0.0f, 0.0f);
            p.spread_rate = 0.1f;
            p.life_time = life_time;
        }
        active_count = 0;

        // 创建 VAO/VBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, max_particles * 6 * (3 + 1) * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

        // 属性设置
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);

        ServiceLocator<ShaderManager>::get()->registerShader(
            "ribbon", 
            getShaderPath("particles/ribbon.vert"), 
            getShaderPath("particles/ribbon.frag")
        );
    }

    ~Particle_Ribbon() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        if (loaded_texture){
            glDeleteTextures(1, &textureID);
        } 
    }

    // 批量添加粒子
    void addParticles(const glm::vec3& emit_pos, int num_particles = 8) {
        float now = glfwGetTime();
        for (int i = 0; i < num_particles; ++i) {
            int idx = active_count;
            active_count = (active_count + 1) % max_particles;
            // 随机角度和半径（初始小，后期扩散）
            float angle = static_cast<float>(rand()) / RAND_MAX * 2.0f * M_PI;
            float r = static_cast<float>(rand()) / RAND_MAX * initial_width * 0.5f;

            particles[idx] = {
                emit_pos,
                now,
                glm::vec2(r * cos(angle), r * sin(angle)),
                0.1f, // TODO:扩散速率
                life_time
            };
            // std::cout << "Add particle " << idx << " at " << particles[idx].pos.x << " " << particles[idx].pos.y << " " << particles[idx].pos.z << std::endl;
        }
        // std::cout << "Add " << num_particles << " particles to ribbon." << std::endl;
        // std::cout << "Active count: " << active_count << std::endl;
    }

    void start_() {
        start_time = glfwGetTime();
        is_active = true;
        active_count = 0;
    }

    void stop() {
        is_active = false;
    }

    bool isActive() const { return is_active; }

    void draw(glm::mat4 view, glm::mat4 proj, glm::vec3 camera_pos) {
        if (!is_active) return;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        const Shader* shader = ServiceLocator<ShaderManager>::get()->useShader("ribbon");
        shader->setUniform("uView", view);
        shader->setUniform("uProjection", proj);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        shader->setUniform("uSpriteTex", 0);

        // 构建所有活跃粒子的顶点数据（每个粒子为一个 quad）
        std::vector<float> vertices;
        vertices.reserve(max_particles * 4 * 4); // 4顶点 * 4属性（pos+time）

        for (int i = 0; i < max_particles; ++i) {
            const auto& p = particles[i];
            float age = glfwGetTime() - p.time;
            if (age > p.life_time) continue;

            float t = age / p.life_time; // [0,1]
            float current_radius = initial_width * (1.0f - t) + max_width * t; // 线性插值扩展

            // 计算当前扩散后的偏移
            glm::vec2 expanded_offset = p.offset * (1.0f + p.spread_rate * age);

            // 基础位置 + 扩展偏移
            glm::vec3 center = p.pos + glm::vec3(expanded_offset.x, expanded_offset.y, 0.0f);

            // Billboard 面向相机
            glm::vec3 to_camera = normalize(camera_pos - center);
            glm::vec3 world_up = glm::vec3(0, 1, 0);
            glm::vec3 right = normalize(cross(to_camera, world_up));
            glm::vec3 up = cross(right, to_camera);

            float size = current_radius * 0.5f; // 半宽

            // 四个角点
            std::array<glm::vec3, 4> corners = {
                center + right * -size + up * -size,
                center + right * size + up * -size,
                center + right * -size + up * size,
                center + right * size + up * size
            };

            // 添加四个顶点
            glm::vec3 quad_vertices[6] = {
                corners[0], corners[1], corners[2],
                corners[0], corners[2], corners[3]
            };

            for (int j = 0; j < 6; ++j) {
                vertices.push_back(quad_vertices[j].x);
                vertices.push_back(quad_vertices[j].y);
                vertices.push_back(quad_vertices[j].z);
                vertices.push_back(t);
                // std::cout<< "Add vertex: " << quad_vertices[j].x << " " << quad_vertices[j].y << " " << quad_vertices[j].z << " " << t << std::endl;
            }
        }
        // std::cout<< "Draw " << vertices.size() / 4 << " vertices." << std::endl;
        // 更新 VBO
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());

        // 绘制（每个粒子是独立 quad，所以是 GL_TRIANGLES）
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size() / 4));
        glBindVertexArray(0);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

private:
    int max_particles;
    float initial_width;
    float max_width;
    float life_time;
    bool is_active = false;
    float start_time = 0.0f;

    std::vector<Particle> particles;
    int active_count = 0;

    unsigned int VAO = 0, VBO = 0, textureID = 0;
    bool loaded_texture = true;
};