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

public:
    Mesh() = default;

    Mesh(const std::string& name, const std::vector<Vertex>& vertices, const std::vector<vIndex>& indices) : name_(name), vertices_(vertices), indices_(indices) {
        allocGPU();
    }

    Mesh(const std::string& name, std::vector<Vertex>&& vertices, std::vector<vIndex>&& indices) : name_(name), vertices_(std::move(vertices)), indices_(std::move(indices)) {
        allocGPU();
    }

    ~Mesh() {
        releaseGPU();
    }

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(Mesh&& other) noexcept {
        VBO_ = other.VBO_;
        EBO_ = other.EBO_;
        material_index_ = other.material_index_;
        name_ = std::move(other.name_);
        vertices_ = std::move(other.vertices_);
        indices_ = std::move(other.indices_);

        other.VBO_ = INVALID_VBO;
        other.EBO_ = INVALID_EBO;
        other.material_index_ = INVALID_MATERIAL;
    }

    Mesh& operator=(Mesh&& other) noexcept {
        if (this != &other) {
            if (VBO_ != INVALID_VBO || EBO_ != INVALID_EBO) {
                releaseGPU();
            }

            VBO_ = other.VBO_;
            EBO_ = other.EBO_;
            name_ = std::move(other.name_);
            material_index_ = other.material_index_;
            vertices_ = std::move(other.vertices_);
            indices_ = std::move(other.indices_);

            other.VBO_ = INVALID_VBO;
            other.EBO_ = INVALID_EBO;
            other.material_index_ = INVALID_MATERIAL;
        }
        return *this;
    }

    // Initialize the mesh with vertex data and index data
    //! warning: this function will destroy the data stored in the input vectors
    bool initMesh(const std::string& name, std::vector<Vertex>& vertices, std::vector<vIndex>& indices) {
        name_ = name;
        vertices_.swap(vertices);
        indices_.swap(indices);
        return allocGPU();
    }

    // Allocate GPU resources for rendering
    bool allocGPU() {
        if (VBO_ != INVALID_VBO || EBO_ != INVALID_EBO) {
            std::cerr << "Warning: mesh already has GPU resources, releasing them first." << std::endl;
            releaseGPU();
        }

        glGenBuffers(1, &VBO_);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_);
        glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(Vertex), vertices_.data(), GL_STATIC_DRAW);

        glGenBuffers(1, &EBO_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_.size() * sizeof(vIndex), indices_.data(), GL_STATIC_DRAW);

        return true;
    }

    // Release GPU resources
    void releaseGPU() {
        if (glIsBuffer(VBO_)) {
            glDeleteBuffers(1, &VBO_);
        }
        if (glIsBuffer(EBO_)) {
            glDeleteBuffers(1, &EBO_);
        }
    }

    // Setters and getters
    size_t getNumIndices() const {
        return indices_.size();
    }

    // Debugging
    const std::string toString() const {
        return "Mesh(name: " + name_ + ", VBO: " + std::to_string(VBO_) + ", EBO: " + std::to_string(EBO_) + ", material_index: " + std::to_string(material_index_) + ", vertices: " + std::to_string(vertices_.size()) + ", indices: " + std::to_string(indices_.size()) + ")";
    }

private:
    // Identification
    std::string name_;

    // GPU resources
    GLuint VBO_ {INVALID_VBO};
    GLuint EBO_ {INVALID_EBO};
    unsigned int material_index_ {INVALID_MATERIAL};

    // Vertex data
    std::vector<Vertex> vertices_;
    std::vector<vIndex> indices_;
};

