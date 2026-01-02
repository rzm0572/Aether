#pragma once

#include "terrain.h"

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
