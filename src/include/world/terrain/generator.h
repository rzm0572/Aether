#pragma once

#include "common/shape.h"
#include "resource/mesh.h"

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

    virtual void getHeightNormalMap(ChunkCoord chunk_coord, int scale, int resolution, std::vector<float>& height_map, std::vector<glm::vec3>& normal_map) {
        int stride = scale * Chunk::X_LENGTH / resolution;
        int global_x_min = chunk_coord.x * Chunk::X_LENGTH;
        int global_z_min = chunk_coord.z * Chunk::Z_LENGTH;

        int length = resolution + 1;
        int map_length = length * length;

        height_map.resize(map_length);
        normal_map.resize(map_length);
        height_map.shrink_to_fit();
        normal_map.shrink_to_fit();

        for (int i = 0; i <= resolution; ++i) {
            for (int j = 0; j <= resolution; ++j) {
                Vertex v = getVertex(global_x_min + i * stride, global_z_min + j * stride);
                height_map[i + j * length] = v.Position.y;
                normal_map[i + j * length] = v.Normal;
            }
        }
    }

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

