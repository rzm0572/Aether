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
#include "skybox.h"

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


#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <string>
#include "stb_image.h"
// TODO: 注释

// 主要参考了 一步步学OpenGL(22) -《OpenGL使用Assimp库导入3d模型》 - Kam92.J的文章 - 知乎 https://zhuanlan.zhihu.com/p/150570465
// 修改了片段着色器的输入，使其能够接受纯色输入，否则会失去颜色，这是模型常用的做法即纯色模型加上细节贴图
// TODO: 金属度贴图（Metalness）
// TODO: 粗糙度贴图（Roughness）
// TODO: 不透明度（Opacity）
// TODO: 自发光（Emission）
// TODO: 法线贴图（Normal Map）
// TODO: 材质捕捉（Material Capture）
// TODO: 自阴影（Self-shadowing）

// TODO: 线框调试
// TODO: 顶点法线


class Texture {
public:
    // 构造函数：指定纹理类型（通常是 GL_TEXTURE_2D）和文件路径
    Texture(GLenum type, const std::string& filepath)
        : m_type(type), m_filepath(filepath), m_textureID(0), m_width(0), m_height(0), m_channels(0) {}
    
    ~Texture(){
        if (m_textureID) {
            glDeleteTextures(1, &m_textureID);
        }
    }

    // 加载纹理（返回是否成功）
    bool Load(){
        // 加载图像数据
        stbi_set_flip_vertically_on_load(true); // OpenGL 原点在左下，需翻转
        unsigned char* data = stbi_load(m_filepath.c_str(), &m_width, &m_height, &m_channels, 0);

        if (!data) {
            std::cerr << "Failed to load texture: " << m_filepath << std::endl;
            return false;
        }

        // 确定格式
        GLenum format;
        if (m_channels == 1)
            format = GL_RED;
        else if (m_channels == 3)
            format = GL_RGB;
        else if (m_channels == 4)
            format = GL_RGBA;
        else {
            std::cerr << "Unsupported number of channels: " << m_channels << " in " << m_filepath << std::endl;
            stbi_image_free(data);
            return false;
        }

        // 生成并配置纹理
        glGenTextures(1, &m_textureID);
        glBindTexture(m_type, m_textureID);

        glTexParameteri(m_type, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(m_type, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(m_type, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(m_type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(m_type, 0, format, m_width, m_height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(m_type);

        stbi_image_free(data);
        glBindTexture(m_type, 0);

        std::cout << "Loaded texture: " << m_filepath << " (" << m_width << "x" << m_height << ", " << m_channels << " channels)" << std::endl;
        return true;
    }
    // 绑定纹理到指定纹理单元（如 GL_TEXTURE0）
    void Bind(GLenum textureUnit = GL_TEXTURE0) const{
        glActiveTexture(textureUnit);
        glBindTexture(m_type, m_textureID);
    }

    // 获取 OpenGL 纹理 ID（用于调试或高级用途）
    GLuint getID() const { return m_textureID; }

private:
    GLenum m_type;
    std::string m_filepath;
    GLuint m_textureID;
    int m_width, m_height, m_channels;
};



/*
Assimp框架和我们的OpenGL程序的接口，这个类的对象使用模型文件名作为其LoadMesh()函数的参数，
加载模型然后创建模型中包含的且我们的程序能够理解的顶点缓冲，索引缓冲和纹理对象数据。
*/
class Mesh_model{
public:
    Mesh_model(GL::ShaderManager* shadermanager){
        shaderManager = shadermanager;
        shaderManager->registerShader("models", getShaderPath("models.vert"), getShaderPath("models.frag"));
    }
    ~Mesh_model() { Clear(); }
    //在栈上创建了Assimp::Importer类的一个实例，并调用其ReadFile方法来读取文件。
    bool LoadMesh(const std::string& Filename){
        // Release the previously loaded mesh (if it exists)
        Clear();

        bool Ret = false;
        Assimp::Importer importer;


        const aiScene* scene = importer.ReadFile(Filename.c_str(),
            aiProcess_Triangulate |      // 所有多边形转成三角形
            aiProcess_GenSmoothNormals | // 自动生成平滑法线
            aiProcess_FlipUVs |          // 翻转 UV（适配 OpenGL）
            aiProcess_JoinIdenticalVertices| // 合并重复顶点，节省内存
            aiProcess_PreTransformVertices  // 预先对顶点进行变换，刚体模型的gltf文件必须要增加，可以将几组部件自动计算后组合
        );
        if (scene) {
            Ret = InitFromScene(scene, Filename);
        }
        else {
            printf("Error parsing '%s': '%s'\n", Filename.c_str(), importer.GetErrorString());
        }

        return Ret;
    }

    void Render(){
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);



        const auto& shader_models = shaderManager->getShader("models");
        shader_models.useShader();
        shader_models.setUniform("model", model);
        shader_models.setUniform("view", view);
        shader_models.setUniform("projection", projection);
        shader_models.setUniform("ourTexture", 0);

        for (unsigned int i = 0 ; i < m_Entries.size() ; i++) {
            glBindBuffer(GL_ARRAY_BUFFER, m_Entries[i].VB);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)12);
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)20);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Entries[i].IB);

            const unsigned int MaterialIndex = m_Entries[i].MaterialIndex;

            // 设置 baseColorFactor 基础颜色
            glm::vec4 baseColor = (MaterialIndex < m_BaseColors.size()) ? m_BaseColors[MaterialIndex] : glm::vec4(1.0f);
            shader_models.setUniform("baseColorFactor", baseColor);

            // 设置 hasTexture 是否有纹理
            bool hasTex = (MaterialIndex < m_HasTexture.size()) && m_HasTexture[MaterialIndex];
            shader_models.setUniform("hasTexture", hasTex);
            // 修改：有纹理才绑定
            if (hasTex && MaterialIndex < m_Textures.size() && m_Textures[MaterialIndex]) {
                m_Textures[MaterialIndex]->Bind(GL_TEXTURE0);
            }

            glDrawElements(GL_TRIANGLES, m_Entries[i].NumIndices, GL_UNSIGNED_INT, 0);
        }
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
    }
    void setModel(glm::mat4 model){
        this->model = model;
    }
    void setView(glm::mat4 view){
        this->view = view;
    }
    void setProjection(glm::mat4 projection){
        this->projection = projection;
    }

private:
    //扫描aiScene对象的mMeshes数组并依次初始化mesh实体对象。
    bool InitFromScene(const aiScene* pScene, const std::string& Filename){
        m_Entries.resize(pScene->mNumMeshes);
        m_Textures.resize(pScene->mNumMaterials);

        // Initialize the meshes in the scene one by one
        for (unsigned int i = 0 ; i < m_Entries.size() ; i++) {
            const aiMesh* paiMesh = pScene->mMeshes[i];
            InitMesh(i, paiMesh);
        }

        return InitMaterials(pScene, Filename);
    }

    void InitMesh(unsigned int Index, const aiMesh* paiMesh){
        m_Entries[Index].MaterialIndex = paiMesh->mMaterialIndex;

        std::vector<Vertex> Vertices;
        std::vector<unsigned int> Indices;
        const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);


        for (unsigned int i = 0 ; i < paiMesh->mNumVertices ; i++) {
            const aiVector3D* pPos = &(paiMesh->mVertices[i]);
            const aiVector3D* pNormal = paiMesh->HasNormals() ? &(paiMesh->mNormals[i]) : &Zero3D;
            const aiVector3D* pTexCoord = paiMesh->HasTextureCoords(0) ? &(paiMesh->mTextureCoords[0][i]) : &Zero3D;

            Vertex v(
                glm::vec3(pPos->x, pPos->y, pPos->z),
                glm::vec2(pTexCoord->x, pTexCoord->y),
                glm::vec3(pNormal->x, pNormal->y, pNormal->z)
            );
            Vertices.push_back(v);
        }
        for (unsigned int i = 0 ; i < paiMesh->mNumFaces ; i++) {
            const aiFace& Face = paiMesh->mFaces[i];
            assert(Face.mNumIndices == 3);
            Indices.push_back(Face.mIndices[0]);
            Indices.push_back(Face.mIndices[1]);
            Indices.push_back(Face.mIndices[2]);
        }
        m_Entries[Index].Init(Vertices, Indices);
    }
    bool InitMaterials(const aiScene* pScene, const std::string& Filename){

        // 获取模型所在目录（用于拼接外部纹理路径），这种思路模型的纹理会另外下载在某个文件夹中
        size_t lastSlash = Filename.find_last_of("/\\");
        std::string Dir = (lastSlash == std::string::npos) ? "." : Filename.substr(0, lastSlash);

        m_Textures.resize(pScene->mNumMaterials, nullptr);
        m_HasTexture.resize(pScene->mNumMaterials, false);      // 新增必要的初始化
        m_BaseColors.resize(pScene->mNumMaterials, glm::vec4(1.0f)); // 新增必要的初始化
        for (unsigned int i = 0; i < pScene->mNumMaterials; ++i) {
            const aiMaterial* pMaterial = pScene->mMaterials[i];
            m_Textures[i] = nullptr;

            bool hasTexture=false;// 这也是新增的，用于记录到底需不需要纹理

            if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
                aiString Path;
                if (pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &Path) == AI_SUCCESS) {
                    const char* texPath = Path.C_Str();      
                    std::string FullPath = Dir + "/" + Path.C_Str();
                    m_Textures[i] = new Texture(GL_TEXTURE_2D, FullPath.c_str());
                    if (!m_Textures[i]->Load()) {
                        std::cerr << "Error loading texture: " << FullPath << std::endl;
                        delete m_Textures[i];
                        m_Textures[i] = nullptr;
                    }
                    else{
                        hasTexture=true;
                    }
                }
            }
            m_HasTexture[i] = hasTexture; // 记录是否真有纹理

            // 增加颜色的处理逻辑，因为有些模型就是会用颜色基础+细节贴图的方式，不能只有贴图
            // 这里我们把颜色存进 m_BaseColors 数组，后面在片段着色器中读取并使用
            aiColor4D baseColor(1.0f, 1.0f, 1.0f, 1.0f);
            if (AI_SUCCESS != pMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor)) {
                baseColor = aiColor4D(1.0f, 1.0f, 1.0f, 1.0f); // 默认白色
            }
            m_BaseColors[i] = glm::vec4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);

        }
        return true;
    }


    #define INVALID_MATERIAL 0xFFFFFFFF
    struct Vertex {
        glm::vec3 Position;
        glm::vec2 TexCoords;
        glm::vec3 Normal;

        Vertex(const glm::vec3& pos, const glm::vec2& tex, const glm::vec3& normal)
            : Position(pos), TexCoords(tex), Normal(normal) {}
    };


    struct MeshEntry {
        GLuint VB = 0;
        GLuint IB = 0;
        unsigned int NumIndices = 0;
        unsigned int MaterialIndex = 0xFFFFFFFF;

        bool Init(const std::vector<Vertex>& Vertices, const std::vector<unsigned int>& Indices) {
            // 创建 VBO
            glGenBuffers(1, &VB);
            glBindBuffer(GL_ARRAY_BUFFER, VB);
            glBufferData(GL_ARRAY_BUFFER, Vertices.size() * sizeof(Vertex), Vertices.data(), GL_STATIC_DRAW);

            // 创建 IBO
            glGenBuffers(1, &IB);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IB);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, Indices.size() * sizeof(unsigned int), Indices.data(), GL_STATIC_DRAW);

            NumIndices = static_cast<unsigned int>(Indices.size());
            return true;
        }
    };

    std::vector<MeshEntry> m_Entries;
    std::vector<Texture*> m_Textures;
    std::vector<glm::vec4> m_BaseColors;
    std::vector<bool> m_HasTexture;
    GL::ShaderManager* shaderManager;// 着色器管理
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;

    void Clear() {
        for (auto* tex : m_Textures) {
            delete tex;
        }
        m_Textures.clear();
        for (auto& entry : m_Entries) {
            if (entry.VB) glDeleteBuffers(1, &entry.VB);
            if (entry.IB) glDeleteBuffers(1, &entry.IB);
        }
        m_Entries.clear();
        m_BaseColors.clear();
        m_HasTexture.clear();
    }
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
    // TODO: 使用合适的图片作为天空盒
    Skybox skybox(
        getAssetPath("skybox/right.jpg"), 
        getAssetPath("skybox/left.jpg"),
        getAssetPath("skybox/top.jpg"),   
        getAssetPath("skybox/bottom.jpg"),
        getAssetPath("skybox/front.jpg"), 
        getAssetPath("skybox/back.jpg"), 
        &shaderManager
        );

    // 加载模型
    Mesh_model model_test(&shaderManager);
    if (!model_test.LoadMesh(getAssetPath("models/j10/scene.gltf"))) {
        std::cerr << "Failed to load model!" << std::endl;
        return -1;
    }
    


    // 开启深度测试
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
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

        // 绘制模型
        model_test.setModel(model);
        model_test.setView(view);
        model_test.setProjection(projection);
        model_test.Render();
// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // 显示线框
        // 渲染天空盒（必须在其他物体之后渲染以优化性能）
        // skybox.changeProjection(projection);
        // skybox.changeView(view);
        // skybox.Render();
        // 天空盒渲染完成
        
        window.swapBuffers();
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    return 0;
}