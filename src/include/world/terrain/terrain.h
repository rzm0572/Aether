#pragma once

#include "common/shape.h"
#include "resource/mesh.h"
#include "resource/model.h"
#include "resource/shader.h"
#include "resource/texture.h"
#include "service/service_locator.h"

struct ChunkCoord {
    int x;
    int z;
};


class Chunk {
public:
    static constexpr int X_LENGTH = 32;
    static constexpr int Z_LENGTH = 32;

    static glm::vec3 getChunkOrigin(ChunkCoord chunk_coord) {
        return glm::vec3(chunk_coord.x * X_LENGTH, 0.0f, chunk_coord.z * Z_LENGTH);
    }

    static ChunkCoord getChunkCoord(float x, float z) {
        return {
            mod(static_cast<int>(x), X_LENGTH),
            mod(static_cast<int>(z), Z_LENGTH)
        };
    }

private:
    static int mod(int x, int m) {
        int p = x % m;
        return p < 0 ? p + m : p;
    }
};


class TerrainGenerator {
public:
    virtual ~TerrainGenerator() = default;
    virtual float getHeight(ChunkCoord chunk_coord, float x, float z) const = 0;
    virtual Vertex getVertex(ChunkCoord chunk_coord, float x, float z) const = 0;

    float getHeight(float x, float z) const {
        ChunkCoord c = Chunk::getChunkCoord(x, z);
        return getHeight(c, x - c.x * Chunk::X_LENGTH, z - c.z * Chunk::Z_LENGTH);
    }

    Vertex getVertex(float x, float z) const {
        ChunkCoord c = Chunk::getChunkCoord(x, z);
        return getVertex(c, x - c.x * Chunk::X_LENGTH, z - c.z * Chunk::Z_LENGTH);
    }

    virtual AABB getChunkAABB(ChunkCoord chunk_coord) const {
        float min_h, max_h;
        min_h = max_h = getHeight(chunk_coord, 0, 0);
        for (int i = 0; i < Chunk::X_LENGTH; ++i) {
            for (int j = 0; j < Chunk::Z_LENGTH; ++j) {
                float height = getHeight(chunk_coord, i, j);
                min_h = std::min(min_h, height);
                max_h = std::max(max_h, height);
            }
        }
        glm::vec3 min_vec = Chunk::getChunkOrigin(chunk_coord) + glm::vec3(0.0f, min_h, 0.0f);
        glm::vec3 max_vec = Chunk::getChunkOrigin({chunk_coord.x + 1, chunk_coord.z + 1}) + glm::vec3(0.0f, max_h, 0.0f);
        return AABB(min_vec, max_vec);
    }
};


// TODO: 四叉树动态加载
// TODO: 多层纹理混合
class Terrain : public Model {
public:
    Terrain(TerrainGenerator& generator) : Model(), generator_(generator) {}

    bool loadModel(const std::string& filepath) = delete;

    unsigned int createMaterial(const std::string& texture_path) {
        auto shader = ServiceLocator<ShaderManager>::get()->getShader("model");
        auto material_manager = ServiceLocator<MaterialManager>::get();
        if (material_manager == nullptr) {
            return 0xffffffff;
        }

        auto terrain_material = std::make_shared<Material>(shader);
        material_manager->registerMaterial("terrain", terrain_material);

        if (!texture_path.empty()) {
            auto texture = ServiceLocator<TextureManager>::get()->getTexture(texture_path);
            if (texture != nullptr) {
                terrain_material->setTexture("Diffuse", texture);
            }
        }
        terrain_material->setConstant("Diffuse", glm::vec4(1.0f));
        terrain_material->setConstant("Specular", glm::vec4(0.0f));
        terrain_material->setConstant("Metallic", 0.0f);
        terrain_material->setConstant("Roughness", 0.5f);

        return insertMaterial(terrain_material);
    }

    void createChunks(int x_min, int x_max, int z_min, int z_max, float stride, unsigned int material_index) {
        std::vector<Vertex> global_vertices;
        std::vector<vIndex> global_indices;

        auto& root_node = getRootNode();

        for (int z = z_min; z <= z_max; ++z) {
            for (int x = x_min; x <= x_max; ++x) {
                ChunkCoord chunk_coord { x, z };

                Mesh mesh;
                initChunkMesh(mesh, chunk_coord, stride, global_vertices, global_indices);
                mesh.setMaterialIndex(material_index);
                unsigned int mesh_index = insertMesh(std::move(mesh));

                ModelNode chunk_node;
                chunk_node.name = "chunk_" + std::to_string(x) + "_" + std::to_string(z);
                chunk_node.local_transform = glm::translate(glm::mat4(1.0f), Chunk::getChunkOrigin(chunk_coord));
                chunk_node.meshes.push_back(mesh_index);
                root_node.children.push_back(chunk_node);
            }
        }

        root_node.name = "Terrain";
        root_node.local_transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
        root_node.meshes.clear();

        allocGPU(global_vertices, global_indices);
    }

private:

    void initChunkMesh(Mesh& mesh, ChunkCoord chunk_coord, float stride, std::vector<Vertex>& global_vertices, std::vector<vIndex>& global_indices) {
        unsigned int vertex_offset = global_vertices.size();
        size_t index_offset = global_indices.size();
        std::string mesh_name = "chunk_" + std::to_string(chunk_coord.x) + "_" + std::to_string(chunk_coord.z);
        std::vector<Vertex> vertices;
        std::vector<vIndex> indices;

        createChunkData(chunk_coord, stride, vertices, indices);

        global_vertices.insert(global_vertices.end(), vertices.begin(), vertices.end());
        for (const auto& index : indices) {
            global_indices.push_back(index + vertex_offset);
        }

        mesh.initMesh(mesh_name, vertices, indices, index_offset);
    }

    void createChunkData(ChunkCoord chunk_coord, float stride, std::vector<Vertex>& vertices, std::vector<vIndex>& indices) {
        int x_count = int((Chunk::X_LENGTH + stride + EPS) / stride);
        int z_count = int((Chunk::Z_LENGTH + stride + EPS) / stride);

        for (int z = 0; z < z_count; ++z) {
            for (int x = 0; x < x_count; ++x) {
                vertices.emplace_back(generator_.getVertex(chunk_coord, x * stride, z * stride));
            }
        }

        for (int z_idx = 0; z_idx < z_count - 1; ++z_idx) {
            for (int x_idx = 0; x_idx < x_count - 1; ++x_idx) {
                int base = z_idx * x_count + x_idx;

                indices.push_back(base);
                indices.push_back(base + 1);
                indices.push_back(base + x_count + 1);

                indices.push_back(base);
                indices.push_back(base + x_count);
                indices.push_back(base + x_count + 1);
            }
        }
    }
    
private:
    static constexpr float EPS = 1e-5;

    TerrainGenerator& generator_;
};

