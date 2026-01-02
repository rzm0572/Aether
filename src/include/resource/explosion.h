#pragma once

#include "entity/collision_draw.h"
#include "glm/fwd.hpp"
#include "resource/mesh.h"
#include "resource/model.h"
#include "resource/material.h"
#include "utils/macros.h"

class GameObject;

struct ExplosionCenter {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 axis;
};

struct ExplodedVertex: public Vertex {
    int fragment_index;
    glm::vec3 fragment_center;
    glm::vec3 velocity;
    glm::vec3 axis;

    ExplodedVertex(const Vertex& vertex, int frag_index, ExplosionCenter center): Vertex(vertex), fragment_index(frag_index), fragment_center(center.position), velocity(center.velocity), axis(center.axis) {}
    ExplodedVertex(const glm::vec3& pos, const glm::vec2& tex, const glm::vec3& normal, int frag_index, const glm::vec3& frag_center, glm::vec3 velocity, glm::vec3 axis): Vertex(pos, tex, normal), fragment_index(frag_index), fragment_center(frag_center), velocity(velocity), axis(axis) {}
};


struct ExplodedMesh {
    friend class ExplodedModel;

    ExplodedMesh() = default;

    ExplodedMesh(unsigned int material_index): material_index_(material_index) {}

    ~ExplodedMesh() = default;

    // Debugging
    const std::string toString() const {
        return "ExplodedMesh(material_index: " + std::to_string(material_index_) + ")";
    }

    unsigned int material_index_ { INVALID_MATERIAL };
    int vertex_offset_ { 0 };
    int vertex_count_ { 0 };
};

class ExplodedModel {
public:
    ExplodedModel(int fragment_count = 1): fragment_count_(fragment_count) {
        glGenVertexArrays(1, &VAO_);
    }

    ExplodedModel(const Model& model, int fragment_count = 1): fragment_count_(fragment_count) {
        glGenVertexArrays(1, &VAO_);
        reconstructModel(model);
    }

    ~ExplodedModel() {
        if (glIsVertexArray(VAO_)) {
            glDeleteVertexArrays(1, &VAO_);
        }
    }

    void reconstructModel(const Model& model);

    // GameObject* createGameObject(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale) {
    //     GameObject* go = new GameObject();
    //     auto& transform = go->getTransformComponent();
    //     transform.setPosition(position);
    //     transform.setRotation(rotation);
    //     transform.setScale(scale);
    // }

    GameObject* createExplosion(GameObject* origin) const;

    std::vector<ExplodedMesh> getMeshes() const {
        return meshes_;
    }

    std::vector<std::shared_ptr<Material>> getMaterials() const {
        return materials_;
    }

    void output() {
        for (const auto& center : explosion_centers_) {
            std::cout << "ExplosionCenter(position: " << center.position << ", velocity: " << center.velocity << ", axis: " << center.axis << ")" << std::endl;
        }
    }

private:
    void allocGPU(const std::vector<ExplodedVertex>& vertices);

    void releaseGPU();

    const std::vector<glm::mat4> collectMeshFromSource(const Model& model);

    void collectMeshFromModelNode(const Model& model, const ModelNode& node, const glm::mat4& parent_transform, std::vector<glm::mat4>& mesh_global_transforms);

    void generateExplosionCenter(const std::vector<Mesh>& meshes, const std::vector<glm::mat4>& mesh_global_transforms);

    int getNearestExplosionCenter(const glm::vec3& position) const;

    glm::vec3 getGlobalPosition(const glm::vec3& local_position, const glm::mat4& global_transform) {
        return glm::vec3(global_transform * glm::vec4(local_position, 1.0f));
    }

    glm::vec3 getGlobalNormal(const glm::vec3& local_normal, const glm::mat3& normal_matrix) {
        return glm::normalize(normal_matrix * local_normal);
    }

    void destroyModel() {
        releaseGPU();
        meshes_.clear();
        materials_.clear();
    }

private:
    GLuint VAO_ { INVALID_VAO };
    GLuint VBO_ { INVALID_VBO };

    int fragment_count_ { 1 };
    std::vector<ExplosionCenter> explosion_centers_;

    std::vector<ExplodedMesh> meshes_;
    std::vector<std::shared_ptr<Material>> materials_;
};
