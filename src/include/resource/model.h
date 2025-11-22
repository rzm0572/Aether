#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <cassert>
#include <filesystem>
#include <glad/glad.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include "resource/mesh.h"
#include "resource/material.h"
#include "resource/shader.h"
#include "service/service_locator.h"
#include "utils/macros.h"

// Model class
// Provide a high-level interface for loading 3D models from file
// Model class holds the ownership of meshes. For materials, it only holds the shared_ptr to material objects, which are holded by MaterialManager.
class Model {
public:
    Model() {
        glGenVertexArrays(1, &VAO_);
    }

    ~Model() {
        glDeleteVertexArrays(1, &VAO_);
    }

    Model(const std::string& filename) {
        glGenVertexArrays(1, &VAO_);
        loadModel(filename);
    }

    // Load model from file
    bool loadModel(const std::string& filepath) {
        destroyModel();    // If model is already loaded, destroy it first

        // Read file via ASSIMP
        bool success = false;
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
            filepath.c_str(),
            aiProcess_Triangulate |         // 所有多边形转成三角形
            aiProcess_GenSmoothNormals |            // 自动生成平滑法线
            aiProcess_FlipUVs |                     // 翻转 UV（适配 OpenGL）
            aiProcess_JoinIdenticalVertices |       // 合并重复顶点，节省内存
            aiProcess_PreTransformVertices          // 预先对顶点进行变换，刚体模型的gltf文件必须要增加，可以将几组部件自动计算后组合
        );
        // ! [will be deprecated] aiProcess_PreTransformVertices 将会被移除，以适配之后要实现的 GameObject 的层级关系

        if (scene && scene->HasMeshes()) {
            std::filesystem::path path(filepath);
            success = initFromScene(scene, path.parent_path());
        } else {
            std::cerr << "Error parsing '" << filepath << "': '" << importer.GetErrorString() << "'" << std::endl;
        }

        return success;
    }

    // ! [will be deprecated] 渲染工作在之后会被移到 Renderer 类中统一实现，模型作为资源层不参与渲染
    // ! 之后将会由 GameObject 持有 TransformComponent 和 RenderComponent, TransformComponent 记录 GameObject 的层级关系，RenderComponent 持有指向 Mesh 和 Material 的指针，然后由 Renderer 读取并渲染
    // Render the model
    void render() const {
        glBindVertexArray(VAO_);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        auto shader = ServiceLocator<ShaderManager>::get()->getShader("model");
        shader->useShader();
        shader->setUniform("model", model);
        shader->setUniform("view", view);
        shader->setUniform("projection", projection);
        shader->setUniform("ourTexture", 0);

        for (unsigned int i = 0; i < meshes_.size(); ++i) {
            glBindBuffer(GL_ARRAY_BUFFER, meshes_[i].VBO_);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, Position));
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, TexCoords));
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, Normal));

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshes_[i].EBO_);

            const unsigned int material_index = meshes_[i].material_index_;

            if (material_index < default_materials_.size()) {
                auto material = default_materials_[material_index];
                if (material) {
                    // std::cout << "Applying " << material->toString() << " to " << meshes_[i].toString() << std::endl;
                    material->apply(TextureType::kDIFFUSE, GL_TEXTURE0);
                }
            }

            glDrawElements(GL_TRIANGLES, meshes_[i].getNumIndices(), GL_UNSIGNED_INT, 0);
        }

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);

        glBindVertexArray(INVALID_VAO);
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

    const std::string toString() const {
        std::string str = "Model(meshes: [";
        for (const auto& mesh : meshes_) {
            str += "\n\t" + mesh.toString() + ",";
        }
        str += "\n], materials: [";
        for (auto material : default_materials_) {
            if (material) {
                str += "\n\t" + material->toString() + ",";
            } else {
                str += "\n\tnullptr,";
            }
        }
        str += "\n])";
        return str;
    }

private:
    bool initFromScene(const aiScene* scene, const std::filesystem::path& directory) {
        meshes_.resize(scene->mNumMeshes);
        default_materials_.resize(scene->mNumMaterials);

        glBindVertexArray(VAO_);
        for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
            const aiMesh* mesh = scene->mMeshes[i];
            initMesh(i, mesh);
        }

        for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
            const aiMaterial* material = scene->mMaterials[i];
            initMaterial(i, material, directory);
        }

        return true;
    }

    // 根据 Assimp 网格数据初始化模型中的网格
    void initMesh(unsigned int index, const aiMesh* ai_mesh) {
        meshes_[index].material_index_ = ai_mesh->mMaterialIndex;
        
        std::string mesh_name = ai_mesh->mName.C_Str();
        std::vector<Vertex> vertices;
        std::vector<vIndex> indices;
        const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

        // 遍历 Assimp 网格中的所有顶点，将其转换为模型使用的顶点格式
        for (vIndex i = 0; i < ai_mesh->mNumVertices; ++i) {
            const aiVector3D* pos = &(ai_mesh->mVertices[i]);
            const aiVector3D* normal = ai_mesh->HasNormals() ? &(ai_mesh->mNormals[i]) : &Zero3D;
            const aiVector3D* tex_coord = ai_mesh->HasTextureCoords(UV_CHANNEL_DIFFUSE) ? 
                                          &(ai_mesh->mTextureCoords[UV_CHANNEL_DIFFUSE][i]) : &Zero3D;
            
            vertices.emplace_back(
                glm::vec3(pos->x, pos->y, pos->z),
                glm::vec2(tex_coord->x, tex_coord->y),
                glm::vec3(normal->x, normal->y, normal->z)
            );
        }

        // 遍历 Assimp 网格中的所有面，提取顶点索引并存储到模型的索引列表中
        for (unsigned int i = 0; i < ai_mesh->mNumFaces; ++i) {
            const aiFace& face = ai_mesh->mFaces[i];
            assert(face.mNumIndices == 3);

            indices.push_back(face.mIndices[0]);
            indices.push_back(face.mIndices[1]);
            indices.push_back(face.mIndices[2]);
        }

        // 使用转换后的顶点和索引数据初始化模型中的网格对象
        meshes_[index].initMesh(mesh_name, vertices, indices);
    }

    bool initMaterial(unsigned int index, const aiMaterial* material, const std::filesystem::path& directory) {
        auto shader = ServiceLocator<ShaderManager>::get()->getShader("model");
        // std::cout << "Material " << index << " loaded with shader: " << shader->toString() << ", directory: " << directory.string() << ", name: " << material->GetName().C_Str() << std::endl;

        default_materials_[index] = ServiceLocator<MaterialManager>::get()->loadMaterial(
            shader, material, directory
        );

        // std::cout << "Material " << index << " loaded: " << success << std::endl;

        // default_materials_[index] = ServiceLocator<MaterialManager>::get()->getMaterial(
        //     directory.string()
        // );

        return true;
    }

    void destroyModel() {
        for (auto& mesh : meshes_) {
            mesh.releaseGPU();
        }
        meshes_.clear();
        default_materials_.clear();
    }

private:
    static constexpr unsigned int UV_CHANNEL_DIFFUSE = 0;

    GLuint VAO_;
    std::vector<Mesh> meshes_;
    std::vector<std::shared_ptr<Material>> default_materials_;

    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
};
