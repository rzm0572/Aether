#pragma once

#include "utils/macros.h"
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

// Vertex layout
struct Vertex {
    glm::vec3 Position;
    glm::vec2 TexCoords;
    glm::vec3 Normal;

    Vertex(const glm::vec3& pos, const glm::vec2& tex, const glm::vec3& normal)
        : Position(pos), TexCoords(tex), Normal(normal) {}
};

using vIndex = unsigned int;

// Mesh class
// This class stores vertex data and index data of a 3D model in memory
// It also holds GPU resources for rendering, such as VBO and EBO
class Mesh {
    friend class Model;
    friend class ExplodedModel;

public:
    Mesh() = default;

    Mesh(const std::string& name, const std::vector<Vertex>& vertices, const std::vector<vIndex>& indices, size_t index_offset = 0, unsigned int material_index = INVALID_MATERIAL) : name_(name), vertices_(vertices), indices_(indices), index_offset_(index_offset), material_index_(material_index) {}

    Mesh(const std::string& name, std::vector<Vertex>&& vertices, std::vector<vIndex>&& indices, size_t index_offset = 0, unsigned int material_index = INVALID_MATERIAL) : name_(name), vertices_(std::move(vertices)), indices_(std::move(indices)), index_offset_(index_offset), material_index_(material_index) {}

    ~Mesh() = default;

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(Mesh&& other) noexcept {
        name_ = std::move(other.name_);
        vertices_ = std::move(other.vertices_);
        indices_ = std::move(other.indices_);
        index_offset_ = other.index_offset_;
        material_index_ = other.material_index_;

        other.material_index_ = INVALID_MATERIAL;
    }

    Mesh& operator=(Mesh&& other) noexcept {
        if (this != &other) {
            name_ = std::move(other.name_);
            vertices_ = std::move(other.vertices_);
            indices_ = std::move(other.indices_);
            index_offset_ = other.index_offset_;
            material_index_ = other.material_index_;

            other.material_index_ = INVALID_MATERIAL;
        }
        return *this;
    }

    // Initialize the mesh with vertex data and index data
    //! warning: this function will destroy the data stored in the input vectors
    void initMesh(const std::string& name, std::vector<Vertex>& vertices, std::vector<vIndex>& indices, size_t index_offset = 0, unsigned int material_index = INVALID_MATERIAL) {
        name_ = name;
        vertices_.swap(vertices);
        indices_.swap(indices);
        index_offset_ = index_offset;
        material_index_ = material_index;
    }

    // Setters and getters
    void setMaterialIndex(unsigned int material_index) {
        material_index_ = material_index;
    }

    size_t getNumVertices() const {
        return vertices_.size();
    }

    size_t getNumIndices() const {
        return indices_.size();
    }

    unsigned int getMaterialIndex() const {
        return material_index_;
    }

    size_t getIndexOffset() const {
        return index_offset_;
    }

    // Debugging
    const std::string toString() const {
        return "Mesh(name: " + name_ + ", material_index: " + std::to_string(material_index_) + ", vertices: " + std::to_string(vertices_.size()) + ", indices: " + std::to_string(indices_.size()) + ", index_offset: " + std::to_string(index_offset_) + ")";
    }

private:
    // Identification
    std::string name_;

    // Vertex data
    std::vector<Vertex> vertices_;
    std::vector<vIndex> indices_;

    size_t index_offset_ {0};
    unsigned int material_index_ {INVALID_MATERIAL};
};

