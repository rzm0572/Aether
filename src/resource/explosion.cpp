#include "resource/explosion.h"
#include "common/game_object.h"
#include "resource/material.h"
#include "service/service_locator.h"
#include <random>


void ExplodedModel::reconstructModel(const Model& model) {
    destroyModel();    // If model is already loaded, destroy it first

    const auto& source_meshes = model.meshes_;
    const auto mesh_global_transforms = collectMeshFromSource(model);

    generateExplosionCenter(source_meshes, mesh_global_transforms);

    std::vector<ExplodedVertex> global_vertices;
    meshes_.resize(source_meshes.size());

    // std::vector<int> explosion_center_count(explosion_centers_.size(), 0);

    for (size_t i = 0; i < source_meshes.size(); ++i) {
        const auto& source_mesh = source_meshes[i];
        const auto& mesh_global_transform = mesh_global_transforms[i];
        const auto& indices = source_mesh.indices_;
        const auto& vertices = source_mesh.vertices_;

        glm::mat3 normal_matrix = glm::mat3(glm::transpose(glm::inverse(mesh_global_transform)));

        meshes_[i].material_index_ = source_mesh.material_index_;
        meshes_[i].vertex_offset_ = global_vertices.size();

        for (size_t j = 0; j < source_mesh.getNumIndices(); j += 3) {
            Vertex v1 = vertices[indices[j]];
            Vertex v2 = vertices[indices[j+1]];
            Vertex v3 = vertices[indices[j+2]];

            v1.Position = getGlobalPosition(v1.Position, mesh_global_transform);
            v2.Position = getGlobalPosition(v2.Position, mesh_global_transform);
            v3.Position = getGlobalPosition(v3.Position, mesh_global_transform);

            v1.Normal = getGlobalNormal(v1.Normal, normal_matrix);
            v2.Normal = getGlobalNormal(v2.Normal, normal_matrix);
            v3.Normal = getGlobalNormal(v3.Normal, normal_matrix);
            
            glm::vec3 triangle_center = (v1.Position + v2.Position + v3.Position) / 3.0f;
            int fragment_index = getNearestExplosionCenter(triangle_center);
            ExplosionCenter fragment_center = explosion_centers_[fragment_index];
            // explosion_center_count[fragment_index]++;

            global_vertices.emplace_back(
                v1,
                fragment_index,
                fragment_center
            );
            global_vertices.emplace_back(
                v2,
                fragment_index,
                fragment_center
            );
            global_vertices.emplace_back(
                v3,
                fragment_index,
                fragment_center
            );
        }

        meshes_[i].vertex_count_ = global_vertices.size() - meshes_[i].vertex_offset_;
    }

    // for (size_t i = 0; i < explosion_center_count.size(); ++i) {
    //     std::cout << "Fragment " << i << " has " << explosion_center_count[i] << " triangles" << std::endl;
    // }

    auto* shader_manager = ServiceLocator<ShaderManager>::get();
    const auto* exploded_shader = shader_manager->getShader("explosion");
    for (auto source_material : model.default_materials_) {
        auto material = std::make_shared<Material>(*source_material);
        material->setShader(exploded_shader);
        material->multConstant("Diffuse", 0.4);
        materials_.push_back(material);
    }

    allocGPU(global_vertices);
}

GameObject* ExplodedModel::createExplosion(GameObject* origin) const {
    GameObject* explosion = new GameObject();
    explosion->getTransformComponent().copyLocalTransform(origin->getTransformComponent());

    auto& rc = explosion->getRenderComponent();
    rc.renderable_ = true;
    rc.type_ = RenderType::EXPLODED_MODEL;
    rc.VAO_ = VAO_;
    rc.exploded_model_ = this;
    rc.material_ = ServiceLocator<MaterialManager>::get()->getMaterial("explosion");

    return explosion;
}

void ExplodedModel::allocGPU(const std::vector<ExplodedVertex>& vertices) {
    releaseGPU();

    glBindVertexArray(VAO_);
    
    glGenBuffers(1, &VBO_);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ExplodedVertex), vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    glEnableVertexAttribArray(5);
    glEnableVertexAttribArray(6);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ExplodedVertex), (const GLvoid*)offsetof(ExplodedVertex, Position));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ExplodedVertex), (const GLvoid*)offsetof(ExplodedVertex, TexCoords));
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ExplodedVertex), (const GLvoid*)offsetof(ExplodedVertex, Normal));
    glVertexAttribIPointer(3, 1, GL_INT, sizeof(ExplodedVertex), (const GLvoid*)offsetof(ExplodedVertex, fragment_index));
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(ExplodedVertex), (const GLvoid*)offsetof(ExplodedVertex, fragment_center));
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(ExplodedVertex), (const GLvoid*)offsetof(ExplodedVertex, velocity));
    glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, sizeof(ExplodedVertex), (const GLvoid*)offsetof(ExplodedVertex, axis));

    glBindVertexArray(INVALID_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, INVALID_VBO);
}

void ExplodedModel::releaseGPU() {
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(3);
    glDisableVertexAttribArray(4);
    glDisableVertexAttribArray(5);
    glDisableVertexAttribArray(6);
    if (glIsBuffer(VBO_)) {
        glDeleteBuffers(1, &VBO_);
        VBO_ = INVALID_VBO;
    }
}

const std::vector<glm::mat4> ExplodedModel::collectMeshFromSource(const Model& model) {
    std::vector<glm::mat4> mesh_global_transforms(model.meshes_.size());
    collectMeshFromModelNode(model, model.root_node_, glm::mat4(1.0f), mesh_global_transforms);
    return mesh_global_transforms;
}

void ExplodedModel::collectMeshFromModelNode(const Model& model, const ModelNode& node, const glm::mat4& parent_transform, std::vector<glm::mat4>& mesh_global_transforms) {
    const glm::mat4 global_transform = parent_transform * node.local_transform;
    for (const auto& mesh_index : node.meshes) {
        mesh_global_transforms[mesh_index] = global_transform;
    }

    for (const auto& child : node.children) {
        collectMeshFromModelNode(model, child, global_transform, mesh_global_transforms);
    }
}

void ExplodedModel::generateExplosionCenter(const std::vector<Mesh>& meshes, const std::vector<glm::mat4>& mesh_global_transforms) {
    if (meshes.empty()) {
        std::cerr << "Error: meshes is empty" << std::endl;
        return;
    }
    if (mesh_global_transforms.size() != meshes.size()) {
        std::cerr << "Error: meshes and mesh_global_transforms must have the same size" << std::endl;
        return;
    }
    explosion_centers_.resize(fragment_count_);

    size_t total_vertices = 0;
    for (const auto& mesh : meshes) {
        total_vertices += mesh.getNumVertices();
    }

    std::mt19937 rng;
    std::uniform_int_distribution<int> dist(0, total_vertices - 1);
    std::uniform_real_distribution<float> axis_dist(-1.0f, 1.0f);
    
    std::vector<size_t> candidate_indices;
    for (int i = 0; i < fragment_count_; ++i) {
        candidate_indices.push_back(dist(rng));
    }

    std::sort(candidate_indices.begin(), candidate_indices.end());

    int current_mesh_index = 0;
    size_t current_mesh_lowerbound = 0;
    size_t current_mesh_upperbound = meshes[current_mesh_index].getNumVertices();
    for (int i = 0; i < fragment_count_; ++i) {
        while (current_mesh_index < meshes.size() && candidate_indices[i] >= current_mesh_upperbound) {
            current_mesh_index++;
            current_mesh_lowerbound = current_mesh_upperbound;
            current_mesh_upperbound += meshes[current_mesh_index].getNumVertices();
        }
        
        const auto& mesh = meshes[current_mesh_index];
        size_t vertex_index = candidate_indices[i] - current_mesh_lowerbound;
        glm::vec3 center_local_position = mesh.vertices_[vertex_index].Position;
        glm::mat4 center_global_transform = mesh_global_transforms[current_mesh_index];
        explosion_centers_[i].position = getGlobalPosition(center_local_position, center_global_transform);
        explosion_centers_[i].velocity = glm::normalize(explosion_centers_[i].position) * 10.0f;
        explosion_centers_[i].axis = glm::normalize(glm::vec3(
            axis_dist(rng),
            axis_dist(rng),
            axis_dist(rng)
        ));
    }
}

int ExplodedModel::getNearestExplosionCenter(const glm::vec3& position) const {
    size_t nearest_index = 0;
    float nearest_distance_sq = glm::length(position - explosion_centers_[0].position);

    for (size_t i = 1; i < explosion_centers_.size(); ++i) {
        float distance_sq = glm::length(position - explosion_centers_[i].position);
        if (distance_sq < nearest_distance_sq) {
            nearest_distance_sq = distance_sq;
            nearest_index = i;
        }
    }

    return static_cast<int>(nearest_index);
}
