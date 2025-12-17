#pragma once

#include "terrain.h"
#include <chrono>
#include <random>

class PerlinGenerator : public TerrainGenerator {
    friend class fBmGenerator;

public:
    PerlinGenerator(float y_base = 0.0f, float y_scale = 1.0f, int perm_period = 16, unsigned long seed = 0) : period_(perm_period), y_base_(y_base), y_scale_(y_scale) {
        if (seed) {
            seed_ = seed;
        } else {
            seed_ = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        }
        
        generateGradients();
    }

    float getHeight(ChunkCoord chunk_coord, float x, float z) const override {
        float global_x = chunk_coord.x + x / Chunk::length;
        float global_z = chunk_coord.z + z / Chunk::width;
        return calculateHeight(global_x, global_z);
    }

    Vertex getVertex(ChunkCoord chunk_coord, float x, float z) const override {
        float global_x = chunk_coord.x + x / Chunk::length;
        float global_z = chunk_coord.z + z / Chunk::width;
        TerrainData data = calculateTerrainData(global_x, global_z);
        glm::vec3 normal = glm::normalize(glm::vec3(-data.dx, 1.0f, -data.dz));
        
        return {
            glm::vec3(x, data.height, z),
            glm::vec2(x, z) * 0.125f,
            normal
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
        // glm::vec3 normal { 0.0f };
        float dx { 0.0f };
        float dz { 0.0f };
    };

    void generatePermutation(int length) {
        perm_.resize(length * 2);
        std::iota(perm_.begin(), perm_.begin() + length, 0);

        std::mt19937 rng(seed_);
        std::shuffle(perm_.begin(), perm_.begin() + length, rng);
        std::copy(perm_.begin(), perm_.begin() + length, perm_.begin() + length);
    }

    void generateGradients() {
        const unsigned int perm_length = period_ * period_;
        generatePermutation(perm_length);
    }

    /*
     * Calculate height for a given position on the terrain.
     * @param x range [0, period_) chunk
     * @param z range [0, period_) chunk
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
     * @param x range [0, period_) chunk
     * @param z range [0, period_) chunk
     * @return TerrainData { height, normal }
     */
    TerrainData calculateTerrainData(float x, float z) const {
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

        // glm::vec3 normal = glm::normalize(glm::vec3(-partial_x, 1.0f, -partial_z));
        
        return { height, partial_x, partial_z };
    }

    int hash(int x, int z) const {
        return perm_[perm_[module(x, period_)] + module(z, period_)] & 7;
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
    int period_ { 16 };            // fundemental period of Perlin noise [chunk(s)]
    float y_base_ { 0.0f };
    float y_scale_ { 1.0f };
    std::vector<int> perm_;
};


class fBmGenerator : public TerrainGenerator {
public:
    fBmGenerator(
        float y_base = 0.0f,
        float y_scale = 64.0f,
        int octaves = 6,
        int lacunarity = 2,
        float persistence = 0.5f,
        int fundamental_freq_log = 0,
        int perm_period = 16,
        unsigned long seed = 0
    ): octaves_(octaves), lacunarity_(lacunarity), persistence_(persistence), fundamental_freq_log_(fundamental_freq_log), y_base_(y_base), y_scale_(y_scale), perm_period_(perm_period), seed_(seed) {
        if (seed_ == 0) {
            seed_ = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        }

        createPerlinLayers(octaves_);
    }

    float getHeight(ChunkCoord chunk_coord, float x, float z) const override {
        float height = y_base_;
        float global_x = chunk_coord.x + x / Chunk::length;
        float global_z = chunk_coord.z + z / Chunk::width;
        float freq = glm::pow(lacunarity_, fundamental_freq_log_);

        for (const auto& layer : perlin_layers_) {
            float x_perl = global_x * freq;
            float z_perl = global_z * freq;
            height += layer.calculateHeight(x_perl, z_perl);

            freq *= lacunarity_;
        }
        return height;
    }

    Vertex getVertex(ChunkCoord chunk_coord, float x, float z) const override {
        float height = y_base_;
        float dx = 0.0f;
        float dz = 0.0f;
        
        float freq = glm::pow(lacunarity_, fundamental_freq_log_);

        float global_x = chunk_coord.x + x / Chunk::length;
        float global_z = chunk_coord.z + z / Chunk::width;

        for (const auto& layer : perlin_layers_) {
            float x_perl = global_x * freq;
            float z_perl = global_z * freq;

            auto data = layer.calculateTerrainData(x_perl, z_perl);
            height += data.height;
            dx += freq * data.dx;
            dz += freq * data.dz;
            
            freq *= lacunarity_;
        }

        glm::vec3 normal = glm::normalize(glm::vec3(-dx, 1.0f, -dz));
        
        return {
            glm::vec3(x, height, z),
            glm::vec2(x, z) * 0.125f,
            normal
        };
    }


private:
    void createPerlinLayers(int num_seeds) {
        std::mt19937 rng(seed_);
        std::uniform_int_distribution<unsigned long> dist(1, std::numeric_limits<unsigned long>::max());

        float layer_scale = y_scale_;
        for (int i = 0; i < num_seeds; ++i) {
            unsigned int layer_seed = dist(rng);
            perlin_layers_.emplace_back(
                0.0f,
                layer_scale,
                perm_period_,
                layer_seed
            );
            layer_scale *= persistence_;
        }
    }

    int module(int x, int m) const {
        int p = x % m;
        return p < 0 ? p + m : p;
    }

    static constexpr float EPS = 1.0e-5f;

    int octaves_ { 6 };
    int lacunarity_ { 2 };
    float persistence_ { 0.5f };
    int fundamental_freq_log_ { 0 };
    float y_base_ { 0.0f };
    float y_scale_ { 64.0f };
    int perm_period_ { 16 };
    unsigned long seed_ { 0 };
    std::vector<PerlinGenerator> perlin_layers_;
};
