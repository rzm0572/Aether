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
#include "world/light.h"
#include "utils/profiler.h"

struct ModelNode {
    std::string name;
    glm::mat4 local_transform;
    std::vector<unsigned int> meshes;
    std::vector<ModelNode> children;
};

// Model class
// Provide a high-level interface for loading 3D models from file
// Model class holds the ownership of meshes. For materials, it only holds the shared_ptr to material objects, which are holded by MaterialManager.
/**
 * @warning: 请务必注意设置好包括相机、视角、光照信息等所有的条件
*/
class Model {
    friend class GameObject;

public:
    Model() {
        glGenVertexArrays(1, &VAO_);
    }

    ~Model() {
        glDeleteVertexArrays(1, &VAO_);
    }

    Model(const std::string& filename,int change_kind) {
        glGenVertexArrays(1, &VAO_);
        loadModel(filename,change_kind);
    }

    // Load model from file
    bool loadModel(const std::string& filepath,int change_kind) {
        destroyModel();    // If model is already loaded, destroy it first

        // Read file via ASSIMP
        bool success = false;
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
            filepath.c_str(),
            aiProcess_Triangulate |         // 所有多边形转成三角形
            aiProcess_GenSmoothNormals |            // 自动生成平滑法线
            aiProcess_FlipUVs |                     // 翻转 UV（适配 OpenGL）
            aiProcess_JoinIdenticalVertices         // 合并重复顶点，节省内存
        );

        if (scene && scene->HasMeshes()) {
            std::filesystem::path path(filepath);
            success = initFromScene(scene, path.parent_path(),change_kind);
        } else {
            std::cerr << "Error parsing '" << filepath << "': '" << importer.GetErrorString() << "'" << std::endl;
        }

        return success;
    }

    const Mesh* getMesh(unsigned int index) {
        return &meshes_[index];
    }

    // ! [will be deprecated] 渲染工作在之后会被移到 Renderer 类中统一实现，模型作为资源层不参与渲染
    // ! 之后将会由 GameObject 持有 TransformComponent 和 RenderComponent, TransformComponent 记录 GameObject 的层级关系，RenderComponent 持有指向 Mesh 和 Material 的指针，然后由 Renderer 读取并渲染
    // Render the model
    void render(glm::mat4 model, glm::mat4 view, glm::mat4 projection, const Light& light) const {
        glBindVertexArray(VAO_);

        auto shader = ServiceLocator<ShaderManager>::get()->useShader("model");
        shader->setUniform("view", view);
        shader->setUniform("projection", projection);
        // shader->setUniform("TextureDiffuse", 0);
        // 光照所需的输入
        light.use(shader);

        renderModelTree(shader, root_node_, model);

        glBindVertexArray(INVALID_VAO);
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

    // Debugging function
    void outputModelTree() const {
        outputModelTree(root_node_);
    }

    void outputModelTree(const ModelNode& node) const {
        std::cout << node.name << " (" << node.meshes.size() << " meshes, " << node.children.size() << " children)" << std::endl;
        std::cout << "\tlocal_transform: \n" << node.local_transform << std::endl;
        for (const auto& mesh_index : node.meshes) {
            std::cout << "\t" << mesh_map_.at(meshes_[mesh_index].name_)->toString() << std::endl;
        }
        std::cout << "\tchild: ";
        for (const auto& child : node.children) {
            std::cout << child.name << " ";
        }
        std::cout << std::endl;
        for (const auto& child : node.children) {
            outputModelTree(child);
        }
    }

protected:
    // Allocate GPU resources for rendering
    void allocGPU(std::vector<Vertex>& vertices, std::vector<vIndex>& indices) {
        releaseGPU();

        glBindVertexArray(VAO_);

        glGenBuffers(1, &VBO_);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        glGenBuffers(1, &EBO_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(vIndex), indices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, Position));
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, TexCoords));
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, Normal));

        glBindVertexArray(INVALID_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, INVALID_VBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, INVALID_EBO);
    }

    // Release GPU resources
    void releaseGPU() {
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
        if (glIsBuffer(VBO_)) {
            glDeleteBuffers(1, &VBO_);
            VBO_ = INVALID_VBO;
        }
        if (glIsBuffer(EBO_)) {
            glDeleteBuffers(1, &EBO_);
            EBO_ = INVALID_EBO;
        }
    }

    void renderModelNode(const Shader* shader, const ModelNode& node, const glm::mat4& global_transform) const {
        shader->setUniform("model", global_transform);
        for (unsigned int mesh_index : node.meshes) {
            const unsigned int material_index = meshes_[mesh_index].material_index_;

            if (material_index < default_materials_.size()) {
                auto material = default_materials_[material_index];
                if (material) {
                    // std::cout << "Applying " << material->toString() << " to " << meshes_[i].toString() << std::endl;
                    material->apply();
                }
            }

            glDrawElements(GL_TRIANGLES, meshes_[mesh_index].getNumIndices(), GL_UNSIGNED_INT, 0);
        }
    }

    void renderModelTree(const Shader* shader, const ModelNode& node, const glm::mat4& parent_transform) const {
        glm::mat4 global_transform = parent_transform * node.local_transform;
        renderModelNode(shader, node, global_transform);
        for (const auto& child : node.children) {
            renderModelTree(shader, child, global_transform);
        }
    }

    ModelNode& getRootNode() {
        return root_node_;
    }

    unsigned int insertMesh(Mesh&& mesh) {
        std::string mesh_name = mesh.name_;
        meshes_.emplace_back(std::move(mesh));
        mesh_map_[mesh_name] = &meshes_.back();
        return meshes_.size() - 1;
    }

    unsigned int insertMaterial(std::shared_ptr<Material> material) {
        default_materials_.push_back(material);
        return default_materials_.size() - 1;
    }

private:
    bool initFromScene(const aiScene* scene, const std::filesystem::path& directory,int change_kind) {
        meshes_.resize(scene->mNumMeshes);
        default_materials_.resize(scene->mNumMaterials);

        std::vector<Vertex> global_vertices;
        std::vector<vIndex> global_indices;

        for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
            const aiMesh* mesh = scene->mMeshes[i];
            initMesh(i, mesh, global_vertices, global_indices,change_kind);
        }

        std::cout << "Loaded " << global_vertices.size() << " vertices and " << global_indices.size() << " indices" << std::endl;

        allocGPU(global_vertices, global_indices);

        for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
            const aiMaterial* material = scene->mMaterials[i];
            initMaterial(i, material, directory);
        }

        loadModelTree(scene->mRootNode, root_node_);

        return true;
    }

    // 根据 Assimp 网格数据初始化模型中的网格
    void initMesh(unsigned int index, const aiMesh* ai_mesh, std::vector<Vertex>& global_vertices, std::vector<vIndex>& global_indices,int change_kind) {
        unsigned int vertex_offset = (unsigned int)global_vertices.size();
        size_t index_offset = global_indices.size();
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
            /*
            就模型坐标来说，
            x 是向前
            y 是向左
            z是向上

            我们可以根据模型在翻转和横转时候的相对于尾焰的坐标变化来确定与中心的偏转
            */
            switch(change_kind){
                case 1:// 导弹：绕y轴将z轴正半轴旋转到x轴正半轴的变换，喷口位置等于导弹位置
                    vertices.emplace_back(
                        glm::vec3( (-pos->y+98.0f)/6.0f, (pos->x-200.4f)/6.0f,(pos->z-3.9f)/6.0f),
                        glm::vec2(tex_coord->x, tex_coord->y),
                        glm::vec3( -normal->y, normal->x,normal->z)
                    );
                break;
                case 2:
                    vertices.emplace_back(
                        glm::vec3(pos->x + 100.0f, pos->y+100.0f, pos->z+100.0f),
                        glm::vec2(tex_coord->x, tex_coord->y),
                        glm::vec3(normal->x, normal->y, normal->z)
                    );
                break;
                default:
                    vertices.emplace_back(
                        glm::vec3(pos->x, pos->y, pos->z),
                        glm::vec2(tex_coord->x, tex_coord->y),
                        glm::vec3(normal->x, normal->y, normal->z)
                    );
                break;
            }
        }

        // 遍历 Assimp 网格中的所有面，提取顶点索引并存储到模型的索引列表中
        for (unsigned int i = 0; i < ai_mesh->mNumFaces; ++i) {
            const aiFace& face = ai_mesh->mFaces[i];
//             if (face.mNumIndices != 3) {
        //     std::cerr << "Non-triangular face found! mNumIndices = " << face.mNumIndices 
        //               << " in mesh '" << ai_mesh->mName.C_Str() 
        //               << "', face index: " << i << std::endl;
        // }
            // assert(face.mNumIndices == 3);
            if(face.mNumIndices == 3){
                indices.push_back(face.mIndices[0]);
                indices.push_back(face.mIndices[1]);
                indices.push_back(face.mIndices[2]);
            }

        }

        global_vertices.insert(global_vertices.end(), vertices.begin(), vertices.end());
        for (auto index : indices) {
            global_indices.push_back(index + vertex_offset);
        }

        // 使用转换后的顶点和索引数据初始化模型中的网格对象
        meshes_[index].initMesh(mesh_name, vertices, indices, index_offset, ai_mesh->mMaterialIndex);
        mesh_map_[mesh_name] = &meshes_[index];
    }

    bool initMaterial(unsigned int index, const aiMaterial* material, const std::filesystem::path& directory) {
        auto shader = ServiceLocator<ShaderManager>::get()->getShader("model");
        // std::cout << "Material " << index << " loaded with shader: " << shader->toString() << ", directory: " << directory.string() << ", name: " << material->GetName().C_Str() << std::endl;

        default_materials_[index] = ServiceLocator<MaterialManager>::get()->loadMaterial(
            shader, material, directory
        );

        // std::cout << "Material " << index << " loaded: " << success << std::endl;

        return true;
    }

    void destroyModel() {
        releaseGPU();
        meshes_.clear();
        default_materials_.clear();
    }

    void loadModelTree(const aiNode* node, ModelNode& model_node) {
        model_node.name = node->mName.C_Str();
        model_node.local_transform = glm::transpose(glm::make_mat4(&node->mTransformation.a1));
        model_node.meshes.insert(model_node.meshes.end(), node->mMeshes, node->mMeshes + node->mNumMeshes);
        for (unsigned int i = 0; i < node->mNumChildren; ++i) {
            ModelNode child_node;
            loadModelTree(node->mChildren[i], child_node);
            model_node.children.push_back(child_node);
        }
    }

private:
    static constexpr unsigned int UV_CHANNEL_DIFFUSE = 0;

    GLuint VAO_ {INVALID_VAO};
    GLuint VBO_ {INVALID_VBO};
    GLuint EBO_ {INVALID_EBO};
    std::vector<Mesh> meshes_;
    std::vector<std::shared_ptr<Material>> default_materials_;

    std::unordered_map<std::string, const Mesh*> mesh_map_;
    ModelNode root_node_;
};
