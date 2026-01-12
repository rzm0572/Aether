#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <cassert>
#include <fstream>
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

    void serialize(std::ostream& os, const std::string& prefix, int depth = 0) const {
        std::string indent(depth * 4, ' ');
        std::string prefix_str = prefix + indent;

        os << prefix_str << "MODELNODE " << (name.empty() ? "Untitled" : name) << std::endl;
        
        const float* mat = glm::value_ptr(local_transform);
        os << prefix_str << "LOCALTRANS";
        for (int i = 0; i < 16; i++) {
            os << " " << mat[i];
        }
        os << std::endl;

        os << prefix_str << "MESH " << meshes.size();
        for (unsigned int mesh_index : meshes) {
            os << " " << mesh_index;
        }
        os << std::endl;

        os << prefix_str << "CHILDREN " << children.size() << std::endl;

        for (const auto& child : children) {
            child.serialize(os, prefix, depth + 1);
        }
    }

    void deserialize(std::istream& is, const std::string name, const std::string& prefix) {
        std::string line;

        auto getStringStream = [&]() -> std::stringstream {
            std::getline(is, line);
            if (!prefix.empty() && line.find(prefix) == 0) {
                line = line.substr(prefix.length());
            }
            return std::stringstream(line);
        };

        std::string keyword;
        std::stringstream ss;

        this->name = name;

        float mat[16];
        ss = getStringStream();
        ss >> keyword;
        for (int i = 0; i < 16; i++) {
            ss >> mat[i];
        }
        local_transform = glm::make_mat4(mat);

        int num_meshes;
        ss = getStringStream();
        ss >> keyword >> num_meshes;
        meshes.resize(num_meshes);
        for (int i = 0; i < num_meshes; i++) {
            ss >> meshes[i];
        }

        int num_children;
        ss = getStringStream();
        ss >> keyword >> num_children;
        children.resize(num_children);
        for (int i = 0; i < num_children; i++) {
            std::stringstream child_ss = getStringStream();
            std::string child_keyword, child_name;
            child_ss >> child_keyword >> child_name;
            children[i].deserialize(is, child_name, prefix);
        }

        // std::cout << "Deserialize " << name << " with " << num_meshes << " meshes, " << num_children << " children" << std::endl;
    }
};

// Model class
// Provide a high-level interface for loading 3D models from file
// Model class holds the ownership of meshes. For materials, it only holds the shared_ptr to material objects, which are holded by MaterialManager.
/**
 * @warning: 请务必注意设置好包括相机、视角、光照信息等所有的条件
*/
class Model {
    friend class GameObject;
    friend class ExplodedModel;

public:
    Model(bool alloc_gpu = true) : alloc_gpu_(alloc_gpu) {
        if (alloc_gpu_) {
            glGenVertexArrays(1, &VAO_);
        }
    }

    ~Model() {
        if (alloc_gpu_) {
            glDeleteVertexArrays(1, &VAO_);
        }
    }

    Model(const std::string& filename, bool alloc_gpu = true) : alloc_gpu_(alloc_gpu) {
        if (alloc_gpu_) {
            glGenVertexArrays(1, &VAO_);
        }
        loadModel(filename);
    }

    // Load model from file
    bool loadModel(const std::string& filepath) {
        destroyModel();    // If model is already loaded, destroy it first

        std::filesystem::path path(filepath);
        std::string extension = path.extension().string();

        if (extension == ".gltf") {
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
                success = initFromScene(scene, path.parent_path());
            } else {
                std::cerr << "Error parsing '" << filepath << "': '" << importer.GetErrorString() << "'" << std::endl;
            }

            return success;
        } else if (extension == ".obj") {
            std::filesystem::path path(filepath);
            return importObjModel(path);
        }

        return false;
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
    void outputModelTree(std::ostream& os) const {
        outputModelTree(os, root_node_);
    }

    void outputModelTree(std::ostream& os, const ModelNode& node) const {
        os << node.name << " (" << node.meshes.size() << " meshes, " << node.children.size() << " children)" << std::endl;
        os << "\tlocal_transform: \n" << node.local_transform << std::endl;
        for (const auto& mesh_index : node.meshes) {
            os << "\t" << meshes_[mesh_index].toString() << std::endl;
        }
        os << "\tchild: ";
        for (const auto& child : node.children) {
            os << child.name << " ";
        }
        os << std::endl;
        for (const auto& child : node.children) {
            outputModelTree(os, child);
        }
    }

    void exportModelAsObj(const std::string& filepath) {
        std::filesystem::path obj_path(filepath);

        std::string mtl_filepath = obj_path.replace_extension(".mtl").filename().string();
        exportMaterialAsMtl(mtl_filepath);

        std::ofstream obj(filepath);
        if (!obj.is_open()) {
            std::cerr << "[export] Error opening file for writing: " << filepath << std::endl;
            return;
        }

        obj << "mtllib " << mtl_filepath << std::endl;
        vIndex vertex_index = 1;
        for (const auto& mesh : meshes_) {
            obj << "o " << mesh.name_ << std::endl;

            for (const auto& vertex : mesh.vertices_) {
                obj << "v " << vertex.Position.x << " " << vertex.Position.y << " " << vertex.Position.z << std::endl;
            }

            for (const auto& vertex : mesh.vertices_) {
                obj << "vt " << vertex.TexCoords.x << " " << vertex.TexCoords.y << std::endl;
            }

            for (const auto& vertex : mesh.vertices_) {
                obj << "vn " << vertex.Normal.x << " " << vertex.Normal.y << " " << vertex.Normal.z << std::endl;
            }

            if (isMeshHasMaterial(mesh)) {
                std::string material_name = "Material_" + std::to_string(mesh.material_index_);
                auto material = default_materials_[mesh.material_index_];
                if (material) {
                    material_name = material->getName();
                }
                obj << "usemtl " << material_name << std::endl;
            }

            obj << "s off" << std::endl;

            const auto& mesh_indices = mesh.indices_;
            for (size_t i = 0; i < mesh_indices.size(); i += 3) {
                unsigned int i0 = mesh_indices[i] + vertex_index;
                unsigned int i1 = mesh_indices[i + 1] + vertex_index;
                unsigned int i2 = mesh_indices[i + 2] + vertex_index;
                
                obj << "f " << i0 << "/" << i0 << "/" << i0 << " "
                            << i1 << "/" << i1 << "/" << i1 << " "
                            << i2 << "/" << i2 << "/" << i2 << std::endl;
            }

            vertex_index += mesh.vertices_.size();

            obj << std::endl;
        }

        obj << "# BEGIN_MODEL_TREE" << std::endl;
        root_node_.serialize(obj, "# ");
        obj << "# END_MODEL_TREE" << std::endl;

        obj.close();
        std::cout << "[export] Model exported to " << filepath << std::endl;
    }

protected:
    // Allocate GPU resources for rendering
    void allocGPU(const std::vector<Vertex>& vertices, const std::vector<vIndex>& indices) {
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

    bool isMeshHasMaterial(const Mesh& mesh) {
        if (mesh.material_index_ >= default_materials_.size()) {
            return false;
        }
        if (default_materials_[mesh.material_index_] == nullptr) {
            return false;
        }
        return true;
    }

    void exportMaterialAsMtl(const std::string& filepath) {
        std::ofstream mtl(filepath);
        if (!mtl.is_open()) {
            std::cerr << "[export] Error opening file for writing: " << filepath << std::endl;
            return;
        }

        for (size_t i = 0; i < default_materials_.size(); ++i) {
            const auto& material = default_materials_[i];
            if (material == nullptr) {
                continue;
            }

            material->exportToMtl(mtl, "Material_" + std::to_string(i));
        }

        std::cout << "[export] Material exported to " << filepath << std::endl;

        mtl.close();
    }

private:
    bool initFromScene(const aiScene* scene, const std::filesystem::path& directory) {
        meshes_.resize(scene->mNumMeshes);
        default_materials_.resize(scene->mNumMaterials);

        std::vector<Vertex> global_vertices;
        std::vector<vIndex> global_indices;

        for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
            const aiMesh* mesh = scene->mMeshes[i];
            initMesh(i, mesh, global_vertices, global_indices);
        }

        std::cout << "Loaded " << global_vertices.size() << " vertices and " << global_indices.size() << " indices" << std::endl;

        if (alloc_gpu_) {
            allocGPU(global_vertices, global_indices);
        }

        for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
            const aiMaterial* material = scene->mMaterials[i];
            initMaterial(i, material, directory);
        }

        loadModelTree(scene->mRootNode, root_node_);

        return true;
    }

    // 根据 Assimp 网格数据初始化模型中的网格
    void initMesh(unsigned int index, const aiMesh* ai_mesh, std::vector<Vertex>& global_vertices, std::vector<vIndex>& global_indices) {
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
            vertices.emplace_back(
                glm::vec3(pos->x, pos->y, pos->z),
                glm::vec2(tex_coord->x, tex_coord->y),
                glm::vec3(normal->x, normal->y, normal->z)
            );
        }

        // 遍历 Assimp 网格中的所有面，提取顶点索引并存储到模型的索引列表中
        for (unsigned int i = 0; i < ai_mesh->mNumFaces; ++i) {
            const aiFace& face = ai_mesh->mFaces[i];
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
        if (alloc_gpu_) {
            releaseGPU();
        }
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

    bool importObjModel(const std::filesystem::path& filepath) {
        bool success = importObjModelGeometry(filepath);
        if (!success) {
            return false;
        }

        std::cout << "[import] geometry part of model " << filepath << " imported successfully" << std::endl;

        return importObjModelTree(filepath);
    }

    bool importObjModelGeometry(const std::filesystem::path& filepath) {
        std::filesystem::path obj_path(filepath);
        std::filesystem::path base_dir_path = obj_path.parent_path();

        // std::cout << obj_path << std::endl;
        // std::cout << base_dir_path << std::endl;

        std::ifstream obj(filepath);
        if (!obj.is_open()) {
            std::cerr << "[import] Error opening file for reading: " << filepath << std::endl;
            return false;
        }

        std::vector<glm::vec3> position_tmp;
        std::vector<glm::vec2> tex_coord_tmp;
        std::vector<glm::vec3> normal_tmp;

        std::string current_mesh_name;
        std::string current_material_name;
        std::vector<Vertex> vertices;
        std::vector<vIndex> indices;
        std::unordered_map<std::string, unsigned int> material_map;

        std::vector<Vertex> global_vertices;
        std::vector<vIndex> global_indices;

        std::string line;
        while (std::getline(obj, line)) {
            std::stringstream ss(line);
            std::string keyword;
            ss >> keyword;

            if (keyword == "mtllib") {
                std::string mtl_filename;
                ss >> mtl_filename;
                importMtlFile((base_dir_path / mtl_filename).string(), material_map);
            } else if (keyword == "v") {
                glm::vec3 position;
                ss >> position.x >> position.y >> position.z;
                position_tmp.push_back(position);
            } else if (keyword == "vt") {
                glm::vec2 tex_coord;
                ss >> tex_coord.x >> tex_coord.y;
                tex_coord_tmp.push_back(tex_coord);
            } else if (keyword == "vn") {
                glm::vec3 normal;
                ss >> normal.x >> normal.y >> normal.z;
                normal_tmp.push_back(normal);
            } else if (keyword == "usemtl") {
                if (!vertices.empty()) {
                    flushMeshImport(current_mesh_name, vertices, indices, material_map[current_material_name], global_vertices, global_indices);
                }
                ss >> current_material_name;
            } else if (keyword == "o" || keyword == "g") {
                if (!vertices.empty()) {
                    flushMeshImport(current_mesh_name, vertices, indices, material_map[current_material_name], global_vertices, global_indices);
                }
                ss >> current_mesh_name;
            } else if (keyword == "f") {
                std::string vertex_substr;
                for (int i = 0; i < 3; ++i) {
                    ss >> vertex_substr;
                    size_t slash_pos_1 = vertex_substr.find("/");
                    size_t slash_pos_2 = vertex_substr.find("/", slash_pos_1 + 1);
                    
                    vIndex v_index = std::stoi(vertex_substr.substr(0, slash_pos_1)) - 1;
                    vIndex vt_index = std::stoi(vertex_substr.substr(slash_pos_1 + 1, slash_pos_2 - slash_pos_1 - 1)) - 1;
                    vIndex vn_index = std::stoi(vertex_substr.substr(slash_pos_2 + 1)) - 1;

                    Vertex vertex = {
                        position_tmp[v_index],
                        tex_coord_tmp[vt_index],
                        normal_tmp[vn_index]
                    };

                    vertices.push_back(vertex);
                    indices.push_back(vertices.size() - 1);
                }
            }
        }

        if (!vertices.empty()) {
            flushMeshImport(current_mesh_name, vertices, indices, material_map[current_material_name], global_vertices, global_indices);
        }

        if (alloc_gpu_) {
            allocGPU(global_vertices, global_indices);
        }

        return true;
    }

    void flushMeshImport(std::string name, std::vector<Vertex>& vertices, std::vector<vIndex>& indices, unsigned int material_index, std::vector<Vertex>& global_vertices, std::vector<vIndex>& global_indices) {
        unsigned int vertex_offset = (unsigned int)global_vertices.size();
        size_t index_offset = global_indices.size();

        global_vertices.insert(global_vertices.end(), vertices.begin(), vertices.end());
        for (auto index : indices) {
            global_indices.push_back(index + vertex_offset);
        }

        Mesh mesh;
        mesh.initMesh(name, vertices, indices, index_offset, material_index);
        insertMesh(std::move(mesh));

        vertices.clear();
        indices.clear();
    }

    void importMtlFile(const std::string& filepath, std::unordered_map<std::string, unsigned int>& material_map) {
        auto texture_manager = ServiceLocator<TextureManager>::get();
        if (!texture_manager) {
            std::cerr << "[import] Texture manager not initialized" << std::endl;
            return;
        }

        std::filesystem::path mtl_path(filepath);
        std::ifstream mtl(mtl_path);
        if (!mtl.is_open()) {
            std::cerr << "[import] Error opening file for reading: " << filepath << std::endl;
            return;
        }

        std::string line;
        std::shared_ptr<Material> current_material;
        std::string current_material_name;

        auto load_texture_constant = [&current_material](const std::string& slot, unsigned int count, std::stringstream& ss, unsigned int padding = 0, float padding_value = 0.0f) {
            glm::vec4 value { padding_value };
            int num_input = count - padding;

            if (num_input >= 1) { ss >> value.x; }
            if (num_input >= 2) { ss >> value.y; }
            if (num_input >= 3) { ss >> value.z; }
            if (num_input >= 4) { ss >> value.w; }

            if (count == 1) {
                if (current_material) {
                    current_material->setConstant(slot, value.x);
                }
            } else if (count == 3) {
                if (current_material) {
                    current_material->setConstant(slot, glm::vec3(value.x, value.y, value.z));
                }
            } else if (count == 4) {
                if (current_material) {
                    current_material->setConstant(slot, value);
                }
            }
        };

        auto load_texture = [&](const std::string& slot, std::stringstream& ss) {
            std::string texture_filename;
            ss >> texture_filename;
            std::string texture_path = (mtl_path.parent_path() / texture_filename).string();
            std::cout << "[import] Loading texture: " << texture_path << std::endl;
            auto texture = texture_manager->getTexture(texture_path);

            if (current_material) {
                current_material->setTexture(slot, texture);
            }
        };

        auto shader = ServiceLocator<ShaderManager>::get()->getShader("model");
        while (std::getline(mtl, line)) {
            std::stringstream ss(line);
            std::string keyword;
            ss >> keyword;

            if (keyword == "newmtl") {
                ss >> current_material_name;
                current_material = std::make_shared<Material>(shader);
                unsigned int material_index = insertMaterial(current_material);
                material_map[current_material_name] = material_index;
            } else if (keyword == "Kd") {
                load_texture_constant("Diffuse", 4, ss, 1, 1.0f);
            } else if (keyword == "map_Kd") {
                load_texture("Diffuse", ss);
            } else if (keyword == "Ks") {
                load_texture_constant("Specular", 3, ss);
            } else if (keyword == "map_Ks") {
                load_texture("Specular", ss);
            } else if (keyword == "Pm") {
                load_texture_constant("Metallic", 1, ss);
            } else if (keyword == "Pr") {
                load_texture_constant("Roughness", 1, ss);
            }
        }

    }

    bool importObjModelTree(const std::filesystem::path& filepath) {
        std::ifstream obj(filepath);
        if (!obj.is_open()) {
            std::cerr << "[import] Error opening file for reading: " << filepath << std::endl;
            return false;
        }
        
        bool has_model_tree = false;

        std::string line;
        while (std::getline(obj, line)) {
            if (line.find("# BEGIN_MODEL_TREE") != std::string::npos) {
                has_model_tree = true;
                break;
            }
        }

        if (!has_model_tree) {
            std::cout << "[import] Model " << filepath << " does not have a model tree, using default flat structure" << std::endl;
            root_node_ = ModelNode();
            root_node_.name = "root";
            root_node_.local_transform = glm::mat4(1.0f);
            for (size_t i = 0; i < meshes_.size(); ++i) {
                root_node_.meshes.push_back(i);
            }

            obj.close();
            return true;
        }

        std::string keyword;
        std::string root_name;
        std::getline(obj, line);
        if (line.size() > 2) {
            line = line.substr(2);
        }
        
        std::stringstream ss(line);
        ss >> keyword >> root_name;
        
        if (keyword == "MODELNODE") {
            root_node_ = ModelNode();
            root_node_.deserialize(obj, root_name, "# ");
        }

        obj.close();

        std::cout << "[import] Model tree of " << filepath << " loaded." << std::endl;
        return true;
    }

private:
    static constexpr unsigned int UV_CHANNEL_DIFFUSE = 0;

    bool alloc_gpu_ { true };

    GLuint VAO_ {INVALID_VAO};
    GLuint VBO_ {INVALID_VBO};
    GLuint EBO_ {INVALID_EBO};
    std::vector<Mesh> meshes_;
    std::vector<std::shared_ptr<Material>> default_materials_;

    std::unordered_map<std::string, const Mesh*> mesh_map_;
    ModelNode root_node_;
};
