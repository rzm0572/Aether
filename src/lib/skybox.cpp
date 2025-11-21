#include "Skybox.h"
#include <iostream>
#include <glad/glad.h> // 或 glew.h，根据你的项目
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" // 确保路径正确
#include "shader.h"
#include "utils/path_handler.h"


class Skybox::CubemapTexture{

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




class Skybox::SkyboxMesh
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


Skybox::Skybox(const std::string &posX, const std::string &negX,
        const std::string &posY, const std::string &negY,
        const std::string &posZ, const std::string &negZ,
        ShaderManager *global_shaderManager){
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

void Skybox::Render(){
    // 记录过去深度测试状态，因为要修改
    GLint OldCullFaceMode;
    glGetIntegerv(GL_CULL_FACE_MODE, &OldCullFaceMode);
    GLint OldDepthFuncMode;
    glGetIntegerv(GL_DEPTH_FUNC, &OldDepthFuncMode);
    glCullFace(GL_FRONT);// 剔除正面（因为我们在盒子内部，要看到内表面）
    glDepthFunc(GL_LEQUAL);// 允许 Z=1 的像素写入（否则会被丢弃）
    // 着色器
    auto& shader = shaderManager->getShader("skybox");
    shader.useShader();


    // 构建视图矩阵
    glm::mat4 viewNoTrans = glm::mat4(glm::mat3(view)); // 移除平移分量，因为天空盒应当无视相机位置
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


Skybox::~Skybox(){
    delete cubemap;
    delete mesh;
}