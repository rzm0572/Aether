#pragma once

#include "resource/mesh.h"
#include "resource/model.h"
#include "resource/shader.h"
#include "resource/texture.h"
#include "service/service_locator.h"
#include <chrono>
#include <random>

struct ChunkCoord {
    int x;
    int z;
};

class TerrainGenerator {
public:
    virtual ~TerrainGenerator() = default;
    virtual float getHeight(ChunkCoord chunk_coord, float x, float z) const = 0;
    virtual Vertex getVertex(ChunkCoord chunk_coord, float x, float z) const = 0;
};

class Chunk {
public:
    static constexpr int length = 32;
    static constexpr int width = 32;

    static glm::vec3 getChunkOrigin(ChunkCoord chunk_coord) {
        return glm::vec3(chunk_coord.x * length, 0.0f, chunk_coord.z * width);
    }

};


class PlainGenerator : public TerrainGenerator {
public:
    PlainGenerator(float y = 0.0f) : y_(y) {}

    float getHeight(ChunkCoord chunk_coord, float x, float z) const override {
        return y_;
    }

    Vertex getVertex(ChunkCoord chunk_coord, float x, float z) const override {
        return {
            glm::vec3(x, y_, z),
            glm::vec2(x, z) * 0.125f,
            glm::vec3(0.0f, 1.0f, 0.0f)
        };
    }

private:
    float y_ { 0.0f };
};


class PerlinGenerator : public TerrainGenerator {
public:
    PerlinGenerator(float y_base = 0.0f, float y_scale = 1.0f, int freq = 16, unsigned long seed = 0) : freq_(freq), y_base_(y_base), y_scale_(y_scale) {
        if (seed) {
            seed_ = seed;
        } else {
            seed_ = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        }
        
        generateGradients();
    }

    float getHeight(ChunkCoord chunk_coord, float x, float z) const override {
        float x_perl = module(chunk_coord.x, freq_) + x / Chunk::length;
        float z_perl = module(chunk_coord.z, freq_) + z / Chunk::width;
        return calculateHeight(x_perl, z_perl);
    }

    Vertex getVertex(ChunkCoord chunk_coord, float x, float z) const override {
        float x_perl = module(chunk_coord.x, freq_) + x / Chunk::length;
        float z_perl = module(chunk_coord.z, freq_) + z / Chunk::width;
        TerrainData data = calculateVertex(x_perl, z_perl);
        
        return {
            glm::vec3(x, data.height, z),
            glm::vec2(x, z) * 0.125f,
            data.normal
        };
    }

    void printChunk(ChunkCoord chunk_coord) const {
        for (int x = 0; x < Chunk::length; ++x) {
            for (int z = 0; z < Chunk::width; ++z) {
                std::cout << getHeight(chunk_coord, x, z) << " ";
            }
            std::cout << std::endl;
        }
    }
    
private:
    struct TerrainData {
        float height { 0.0f };
        glm::vec3 normal { 0.0f };
    };

    void generatePermutation(int length) {
        perm_.resize(length * 2);
        std::iota(perm_.begin(), perm_.begin() + length, 0);

        std::mt19937 rng(seed_);
        std::shuffle(perm_.begin(), perm_.begin() + length, rng);
        std::copy(perm_.begin(), perm_.begin() + length, perm_.begin() + length);
    }

    void generateGradients() {
        const unsigned int perm_length = freq_ * freq_;
        generatePermutation(perm_length);
    }

    /*
     * Calculate height for a given position on the terrain.
     * @param x range [0, freq_) chunk
     * @param z range [0, freq_) chunk
     * @return range [-1, 1]
     */
    float calculateHeight(float x, float z) const {
        int x_int = std::floor(x);
        int z_int = std::floor(z);
        
        float x_frac = x - x_int;
        float z_frac = z - z_int;

        glm::vec2 offset[4] = {
            glm::vec2(x_frac, z_frac),
            glm::vec2(x_frac - 1, z_frac),
            glm::vec2(x_frac, z_frac - 1),
            glm::vec2(x_frac - 1, z_frac - 1)
        };

        glm::vec2 g[4] = {
            kGradient[hash(x_int, z_int)],
            kGradient[hash(x_int + 1, z_int)],
            kGradient[hash(x_int, z_int + 1)],
            kGradient[hash(x_int + 1, z_int + 1)]
        };

        float dot[4] = {
            glm::dot(offset[0], g[0]),
            glm::dot(offset[1], g[1]),
            glm::dot(offset[2], g[2]),
            glm::dot(offset[3], g[3])
        };

        float u = fade(x_frac);
        float v = fade(z_frac);

        float height = lerp(
            lerp(dot[0], dot[1], u),
            lerp(dot[2], dot[3], u),
            v
        );

        return height * y_scale_ + y_base_;
    }

    /*
     * Calculate vertex data for a given position on the terrain.
     * @param x range [0, freq_) chunk
     * @param z range [0, freq_) chunk
     * @return TerrainData { height, normal }
     */
    TerrainData calculateVertex(float x, float z) const {
        int x_int = std::floor(x);
        int z_int = std::floor(z);
        
        float x_frac = x - x_int;
        float z_frac = z - z_int;

        glm::vec2 offset[4] = {
            glm::vec2(x_frac, z_frac),
            glm::vec2(x_frac - 1, z_frac),
            glm::vec2(x_frac, z_frac - 1),
            glm::vec2(x_frac - 1, z_frac - 1)
        };

        glm::vec2 g[4] = {
            kGradient[hash(x_int, z_int)],
            kGradient[hash(x_int + 1, z_int)],
            kGradient[hash(x_int, z_int + 1)],
            kGradient[hash(x_int + 1, z_int + 1)]
        };

        float dot[4] = {
            glm::dot(offset[0], g[0]),
            glm::dot(offset[1], g[1]),
            glm::dot(offset[2], g[2]),
            glm::dot(offset[3], g[3])
        };

        float u = fade(x_frac);
        float v = fade(z_frac);
        float du = fade_derivative(x_frac);
        float dv = fade_derivative(z_frac);

        float height = lerp(
            lerp(dot[0], dot[1], u),
            lerp(dot[2], dot[3], u),
            v
        );

        float partial_x = y_scale_ * lerp(
            g[0].x + (g[1].x - g[0].x) * u + (dot[1] - dot[0]) * du,
            g[2].x + (g[3].x - g[2].x) * u + (dot[3] - dot[2]) * du,
            v
        );

        float partial_z = y_scale_ * lerp(
            g[0].y + (g[2].y - g[0].y) * v + (dot[2] - dot[0]) * dv,
            g[1].y + (g[3].y - g[1].y) * v + (dot[3] - dot[1]) * dv,
            u
        );

        float approx_partial_x = (calculateHeight(x + 0.001f, z) - calculateHeight(x - 0.001f, z)) / 0.002f;
        float approx_partial_z = (calculateHeight(x, z + 0.001f) - calculateHeight(x, z - 0.001f)) / 0.002f;

        // std::cout << "X: " << x << " Z: " << z << std::endl;
        // std::cout << "Height: " << height << std::endl;
        // std::cout << "Partial x: " << partial_x << " Approx: " << approx_partial_x << std::endl;
        // std::cout << "Partial z: " << partial_z << " Approx: " << approx_partial_z << std::endl;

        height = height * y_scale_ + y_base_;

        glm::vec3 normal = glm::normalize(glm::vec3(-partial_x, 1.0f, -partial_z));
        
        return { height, normal };
    }

    int hash(int x, int z) const {
        return perm_[perm_[module(x, freq_)] + module(z, freq_)] & 7;
    }

    float fade(float t) const {
        return (10 + t * (-15 + 6 * t)) * t * t * t;
    }

    float fade_derivative(float t) const {
        return 30 * t * t * (t - 1) * (t - 1);
    }

    float lerp(float a, float b, float t) const {
        return a + (b - a) * t;
    }

    int module(int x, int m) const {
        int p = x % m;
        return p < 0 ? p + m : p;
    }

    static constexpr glm::vec2 kGradient[8] = {
        {1, 0},
        {1, 1},
        {0, 1},
        {-1, 1},
        {-1, 0},
        {1, -1},
        {0, -1},
        {-1, -1}
    };

    unsigned long seed_ { 0 };
    int freq_ { 16 };            // chunk(s)
    float y_base_ { 0.0f };
    float y_scale_ { 1.0f };
    std::vector<int> perm_;
};


class Terrain : public Model {
public:
    Terrain(TerrainGenerator& generator) : generator_(generator), Model() {}

    bool loadModel(const std::string& filepath) = delete;

    void createChunks(int x_min, int x_max, int z_min, int z_max, float stride, const std::string& texture_path = std::string()) {
        auto shader = ServiceLocator<ShaderManager>::get()->getShader("model");

        auto terrain_material = std::make_shared<Material>(shader);

        if (!texture_path.empty()) {
            auto texture = ServiceLocator<TextureManager>::get()->getTexture(texture_path);
            if (texture != nullptr) {
                terrain_material->setTexture(TextureType::kDIFFUSE, texture);
            }
        }
        terrain_material->setBaseColor(glm::vec4(1.0f));
        unsigned int material_index = insertMaterial(terrain_material);

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
        int x_count = int((Chunk::length + stride + EPS) / stride);
        int z_count = int((Chunk::width + stride + EPS) / stride);

        unsigned int vertex_offset = vertices.size();
        unsigned int index_offset = indices.size();

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

