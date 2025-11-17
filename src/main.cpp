#include "utils/macros.h"
#include "utils/path_handler.h"
#include "shader.h"
#include "window.h"
#include <iostream>
#include <cmath>
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string.h>
#define STB_IMAGE_IMPLEMENTATION//必须定义才能使用stb_image.h
#include <stb_image.h>

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



class CubemapTexture{
/**
 * @brief CubemapTexture 天空盒纹理类
 * 通过文件路径导入六张图片形成一个盒子
 * 
 * 
 * @note 重要注意事项或使用限制
 * @warning 需要特别警惕的问题
 * 

 * @example 示例：首先以相对于 asset 目录的六张正方形特定天空盒图片相对路径为参数的 CubemapTexture 构造函数的使用示例：
 * CubemapTexture skyboxTexture("skybox/right.jpg", "skybox/left.jpg", "skybox/top.jpg", "skybox/bottom.jpg", "skybox/front.jpg", "skybox/back.jpg");
 * cubemap->load()
 * cubemap->Bind(GL_TEXTURE0);
 * 
 * @see 目前当且仅当在 Skybox 中用于导入天空盒纹理时，才会使用到此类。

来源：一步步学OpenGL(25) -《Skybox天空盒子》 - Kam92.J的文章 - 知乎 https://zhuanlan.zhihu.com/p/150570683

将图像接口替换为 stb_image.h，参考 stb_image图像解码库 - 李钢蛋的文章 - 知乎 https://zhuanlan.zhihu.com/p/466294684
*/
public:
    /**
     * @brief 构造一个立方体贴图纹理对象
     *
     * @param Directory 包含立方体贴图纹理文件的目录路径
     * @param PosXFilename 正X面纹理文件名
     * @param NegXFilename 负X面纹理文件名
     * @param PosYFilename 正Y面纹理文件名
     * @param NegYFilename 负Y面纹理文件名
     * @param PosZFilename 正Z面纹理文件名
     * @param NegZFilename 负Z面纹理文件名
     *
     * @note 构造函数会将目录路径与各面文件名拼接成完整路径，
     *       并初始化纹理对象(m_textureObj)为0
     */
    CubemapTexture(const std::string &PosXFilename,
                   const std::string &NegXFilename,
                   const std::string &PosYFilename,
                   const std::string &NegYFilename,
                   const std::string &PosZFilename,
                   const std::string &NegZFilename)
    {
        // 将输入的路径和图片名拼接成文件名存入数组
        m_fileNames[0] =PosXFilename;
        m_fileNames[1] =NegXFilename;
        m_fileNames[2] =PosYFilename;
        m_fileNames[3] =NegYFilename;
        m_fileNames[4] =PosZFilename;
        m_fileNames[5] =NegZFilename;

        m_textureObj = 0;// 初始化纹理对象ID为0
    }

    ~CubemapTexture(){
        if (m_textureObj != 0) {
            glDeleteTextures(1, &m_textureObj); // 释放 OpenGL 纹理资源
            m_textureObj = 0;
        }
    }

    /**
     * @brief 加载立方体贴图纹理
     *
     * 该方法加载6张图片作为立方体贴图的各个面纹理，并将它们上传到GPU。
     * 使用stb_image库加载图片数据，并设置适当的纹理参数。
     *
     * @return bool 加载成功返回true，任何一张图片加载失败则返回false
     *
     * @example 示例：cubemapTexture.Load()
     *
     * @note 该方法会为立方体贴图设置以下纹理参数：
     * - 放大过滤器：GL_LINEAR
     * - 缩小过滤器：GL_LINEAR
     * - 各向环绕模式：GL_CLAMP_TO_EDGE
     *
     * @warning 调用此方法前必须确保m_fileNames数组已正确初始化，包含6个有效的图片路径
     *
     * @throws 无显式抛出异常，但会在加载失败时输出错误信息到标准错误流
     */
    bool Load()
    {                                                     // 导入图片作为天空盒的纹理特征，会读取6张图片，上传到GPU
        glGenTextures(1, &m_textureObj);                  // 生成纹理对象
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureObj); // 绑定纹理对象到 GL_TEXTURE_CUBE_MAP
        GLenum faceTargets[6] = {//应该贴图到哪个面,定义立方体贴图六个面的标准 OpenGL 枚举常量。
            GL_TEXTURE_CUBE_MAP_POSITIVE_X,
            GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
            GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
            GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
            GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
            GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
        };
        for (unsigned int i = 0 ; i < 6 ; i++) { // 循环来遍历cubemap的六个面枚举
            //分别加载图片文件
            // 使用stb_image.h RGBA四通道加载背景图像,得到图像 data,宽,高,通道数，最后一位可以改为3，即三通道，因为天空盒只需要RGB
            unsigned char* data = stbi_load(m_fileNames[i].c_str(), &width, &height, &channels, 4);
            

            if (data) {        
                glTexImage2D(faceTargets[i], 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);// 上传到对应面
                stbi_image_free(data); // 释放CPU内存
            }
            else {
                std::cerr << "Failed to load cubemap texture: " << m_fileNames[i] << std::endl;
                return false;
            }
        }
        // 设置纹理参数，即采样时候采用线性过滤，边缘拉伸，天空盒的原理是根据视角选择不同面，所以需要拉伸边缘
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        return true;
    }

    /**
     * @brief 将立方体贴图纹理绑定到指定的纹理单元
     *
     * @param TextureUnit 要绑定的纹理单元 (例如 GL_TEXTURE0)
     *
     * @example 示例：cubemapTexture.Bind(GL_TEXTURE0);
     */
    void Bind(GLenum TextureUnit)
    { // 绑定纹理对象到指定纹理单元
        glActiveTexture(TextureUnit);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureObj);
    }

private:
    int width, height, channels;// 导入的图片宽高和通道数
    std::string m_fileNames[6];// 6张图片文件路径
    unsigned int m_textureObj;//  纹理对象ID，加载cubemap纹理
};



// 简单的立方体网格（用于天空盒）
/**
 * @brief 天空盒网格类
 * 
 * 用于渲染天空盒的立方体网格，由8个顶点组成，每个顶点有3个坐标值(x,y,z)。
 * @note 构造函数会自动完成顶点的分配，
 * 
 * @see 目前当且仅当在 Skybox 中用于渲染天空盒网格时，才会使用到此类。

*/
class SkyboxMesh
{
public:
    /**
     * @brief 构造函数，初始化天空盒网格
     *
     * 创建一个以原点为中心、边长为200的立方体网格(顶点坐标乘以100)，因为视觉范围就是0~100。
     * 初始化VAO(顶点数组对象)和VBO(顶点缓冲对象)，并将顶点数据上传到GPU。
     * 顶点数据包含36个顶点(6个面，每个面2个三角形)，用于渲染立方体贴图。
     *
     * 顶点属性布局：
     * - 位置：location 0，3个浮点数(x,y,z)
     *
     * 注意：构造后会自动绑定VAO和VBO，并在最后解绑VAO。
     * @example 示例：SkyboxMesh skyboxMesh;
     *          skyboxMesh.Render(); // 绘制天空盒
     * 

     */
    // TODO: 若效果不佳更改为球形天球
    SkyboxMesh()
    {
        // 立方体8个顶点（以原点为中心，边长为2）
        float skyboxVertices[] = {
            // positions
            -1.0f, 1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, 1.0f, -1.0f,
            -1.0f, 1.0f, -1.0f,

            -1.0f, -1.0f, 1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f, 1.0f, -1.0f,
            -1.0f, 1.0f, -1.0f,
            -1.0f, 1.0f, 1.0f,
            -1.0f, -1.0f, 1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 1.0f,
            -1.0f, -1.0f, 1.0f,

            -1.0f, 1.0f, -1.0f,
            1.0f, 1.0f, -1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, 1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, 1.0f,
            1.0f, -1.0f, 1.0f};
        for (int i = 0; i < 36*3; ++i) skyboxVertices[i] *= 100.0f;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glBindVertexArray(0);
    }

    /**
     * @brief 绘制天空盒
     *
     * 该函数负责绑定顶点数组对象(VAO)并绘制天空盒的36个顶点(立方体的6个面，每个面2个三角形)。
     * 渲染完成后会自动解绑VAO。
     *
     * @note 调用此函数前必须确保:
     * - VAO已正确初始化并包含有效的顶点数据
     * - 适当的着色器程序已启用
     * - OpenGL上下文已正确设置
     *
     * @warning 此函数不会处理以下内容:
     * - 深度测试状态设置
     * - 面剔除状态设置
     * - 着色器uniform变量的设置
     */
    void Render()
    {
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }

    ~SkyboxMesh() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }

private:
    unsigned int VAO, VBO;
};

/**
 * @brief 天空盒类，可以实现根据图片位置构造天空盒，调节视角、相机等，可以绘制天空盒
 * @warning 你需要构造天空盒，修改相机、视角等必要参数之后才能合理的绘制天空盒
 * @note 你可以在循环外面定义这个类，然后在渲染循环中实例化并调用它的Render方法，这个类仅暴露了天空盒图片和相机的接口
 * @warning 你需要在每一帧，即每次循环前清除深度  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); 
 * @example 示例：Skybox skybox(getAssetPath("skybox/right.jpg"), getAssetPath("skybox/left.jpg"), getAssetPath("skybox/top.jpg"), getAssetPath("skybox/bottom.jpg"), getAssetPath("skybox/front.jpg"), getAssetPath("skybox/back.jpg"), &shaderManager);
 *              skybox.changeCameraPos(glm::vec3(0.0f, 0.0f, 3.0f)); // 设置相机位置
 *              skybox.changeProjection(glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f)); // 设置投影矩阵
 *              skybox.changeView(glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f))); // 设置视角矩阵
 *              skybox.Render(); // 绘制天空盒
*/
class Skybox
{
public:
    /**
     * @brief 构造一个天空盒对象
     *
     * @param posX 立方体贴图+X面(右)的纹理文件路径
     * @param negX 立方体贴图-X面(左)的纹理文件路径
     * @param posY 立方体贴图+Y面(上)的纹理文件路径
     * @param negY 立方体贴图-Y面(下)的纹理文件路径
     * @param posZ 立方体贴图+Z面(前)的纹理文件路径
     * @param negZ 立方体贴图-Z面(后)的纹理文件路径
     * @param global_shaderManager 全局着色器管理器指针，用于注册天空盒着色器
     *
     *
     * @throws std::runtime_error 如果加载立方体贴图纹理失败
     *
     * @note 构造函数会初始化天空盒的立方体贴图、网格和着色器，
     *       并设置默认相机位置为(0,0,3)
     */
    Skybox(const std::string &posX, const std::string &negX,
           const std::string &posY, const std::string &negY,
           const std::string &posZ, const std::string &negZ,
           GL::ShaderManager *global_shaderManager)
    {
        // 创建立方体贴图
        cubemap = new CubemapTexture(posX, negX, posY, negY, posZ, negZ);
        if (!cubemap->Load()) {
            std::cerr << "Failed to load skybox textures!" << std::endl;
        }

        mesh = new SkyboxMesh();// 创建天空盒网格
        // 注册天空盒着色器
        shaderManager=global_shaderManager;
        shaderManager->registerShader("skybox", getShaderPath("skybox.vert"), getShaderPath("skybox.frag"));

        // 相机默认参数
        cameraPos   = glm::vec3(0.0f, 0.0f, 3.0f);
    }

    ~Skybox(){
        delete cubemap;
        delete mesh;
    }

    /**
     * @brief 渲染天空盒
     *
     * 该方法负责渲染天空盒，执行以下操作：
     * 1. 保存当前OpenGL的剔除模式和深度测试状态
     * 2. 设置天空盒特有的渲染状态（正面剔除和LEQUAL深度测试）
     * 3. 使用天空盒着色器并设置必要的uniform变量
     * 4. 绑定立方体贴图纹理
     * 5. 渲染天空盒网格
     * 6. 恢复之前保存的OpenGL状态
     *
     * @note 该方法会修改OpenGL状态，调用后会自动恢复原始状态
     * @note 视图矩阵会移除平移分量，确保天空盒始终围绕相机
     *
     * @param view 当前场景的视图矩阵
     * @param projection 当前场景的投影矩阵
     */
    void Render()
    {
        // 记录过去深度测试状态，因为要修改
        GLint OldCullFaceMode;
        glGetIntegerv(GL_CULL_FACE_MODE, &OldCullFaceMode);
        GLint OldDepthFuncMode;
        glGetIntegerv(GL_DEPTH_FUNC, &OldDepthFuncMode);
        glCullFace(GL_FRONT);
        glDepthFunc(GL_LEQUAL);

        // 着色器
        auto& shader = shaderManager->getShader("skybox");
        shader.useShader();


        // 构建视图矩阵
        glm::mat4 viewNoTrans = glm::mat4(glm::mat3(view)); // 移除平移分量
        glm::mat4 skyboxView = projection * viewNoTrans;
        // 输入投影矩阵给着色器
        shader.setUniform("gWVP", skyboxView);
        cubemap->Bind(GL_TEXTURE0);
        shader.setUniform("gCubemapTexture", 0);

        mesh->Render();


        // 恢复深度测试状态
        glCullFace(OldCullFaceMode); 
        glDepthFunc(OldDepthFuncMode);
    }

    /**
     * @brief 设置相机位置
     *
     * @param pos 新的相机位置坐标(glm::vec3类型)
     */

    void changeCameraPos(glm::vec3 pos)
    {
        cameraPos = pos;
    }
    /**
     * @brief 更新投影矩阵
     * 
     * @param proj 新的投影矩阵，将替换当前投影矩阵
     */
    void changeProjection(glm::mat4 proj){
        projection=proj;
    }
    /**
     * @brief 更新视图矩阵
     *
     * @param view_ 新的视图矩阵，用于替换当前视图矩阵
     */
    void changeView(glm::mat4 view_){
        view=view_;
    }

private: 
    CubemapTexture* cubemap;// 天空盒纹理
    SkyboxMesh* mesh;// 天空盒网格
    GL::ShaderManager* shaderManager;// 天空盒着色器管理器，构造函数中注册shader，需要shader时取出即可
    glm::vec3 cameraPos;// 相机位置
    glm::mat4 projection;// 投影矩阵
    glm::mat4 view;// 视角矩阵
};

int main() {
    const unsigned int SCR_WIDTH = 800;
    const unsigned int SCR_HEIGHT = 600;

    GL::Window window(SCR_WIDTH, SCR_HEIGHT, "Main", [](GLFWwindow* window, int width, int height) {
        glViewport(0, 0, width, height);
    });

    window.makeCurrent();

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    GL::ShaderManager shaderManager;
    shaderManager.registerShader("trans", getShaderPath("trans.vert"), getShaderPath("trans.frag"));
    const auto& shader = shaderManager.getShader("trans");

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
    
    // 创建天空盒
    Skybox skybox(
        getAssetPath("skybox/right.jpg"), 
        getAssetPath("skybox/left.jpg"),
        getAssetPath("skybox/top.jpg"),   
        getAssetPath("skybox/bottom.jpg"),
        getAssetPath("skybox/front.jpg"), 
        getAssetPath("skybox/back.jpg"), 
        &shaderManager
        );

    // 开启深度测试
    glEnable(GL_DEPTH_TEST);
    while (!window.shouldClose()) {
        processInput(window.getWindow());

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); 

        shader.useShader();

        float timeValue = glfwGetTime()/10.f;
        float greenValue = (std::sin(timeValue) / 2.0f) + 0.5f;

        // glm::mat4 trans = glm::mat4(1.0f);
        // trans = glm::rotate(trans, (float)timeValue, glm::vec3(0.0f, 0.0f, 1.0f));

        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -0.0005f));

        shader.setUniform("ourColor", 0.0f, greenValue, 0.0f, 1.0f);

        // shader.setUniform("transform", trans);

        shader.setUniform("model", model);
        shader.setUniform("view", view);
        shader.setUniform("projection", projection);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

        // glDrawArrays(GL_TRIANGLES, 0, 3);   // glDrawArrays will read data from the currently bound VAO
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);

        // 渲染天空盒（必须在其他物体之后渲染以优化性能）
        skybox.changeProjection(projection);
        skybox.changeView(view);
        skybox.Render();
        
        window.swapBuffers();
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    return 0;
}