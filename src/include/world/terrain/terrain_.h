#pragma once

#include "resource/material.h"
#include "resource/shadow_map.h"
#include "resource/texture.h"
#include "replacer.h"
#include "utils/macros.h"
#include "world/terrain/terrain.h"
#include <glm/glm.hpp>

class TexturePoolInterface {
public:
    virtual ~TexturePoolInterface() = default;
    virtual void insert(int world_x, int world_z, int scale, int &texture_index) = 0;
    virtual void remove(int texture_index) = 0;
};

template<unsigned int PoolSize = 1024, ReplacerType Replacer = LRUReplacer>
class TexturePool : public TexturePoolInterface {
public:
    TexturePool(TerrainGenerator& generator, int base_resolution = 8): height_map_(GL_TEXTURE_2D_ARRAY, "HeightMap"), normal_map_(GL_TEXTURE_2D_ARRAY, "NormalMap"), generator_(generator), base_resolution_(base_resolution) {
        replacer_ = new Replacer(PoolSize);

        int texture_size = base_resolution + 1;

        height_map_.Load<float>(nullptr, texture_size, texture_size, 1, PoolSize, false, GL_CLAMP_TO_EDGE);
        normal_map_.Load<float>(nullptr, texture_size, texture_size, 3, PoolSize, false, GL_CLAMP_TO_EDGE);
    }

    ~TexturePool() {
        delete replacer_;
    }

    void insert(int world_x, int world_z, int scale, int& texture_index) override {
        int victim_index;
        if (replacer_->victim(victim_index)) {
            std::vector<float> height_data;
            std::vector<glm::vec3> normal_data;

            generator_.getHeightNormalMap({world_x, world_z}, scale, base_resolution_, height_data, normal_data);

            // std::cout << "Generated texture: " << world_x << ", " << world_z << ", " << scale << std::endl;
            // std::cout << "Victim texture: " << victim_index << std::endl;
            // std::cout << "Height data size: " << height_data.size() << std::endl;
            // std::cout << "Height data: ";
            // for (int i = 0; i < 8; ++i) {
            //     std::cout << height_data[i] << " ";
            // }
            // std::cout << std::endl;
            // std::cout << "Normal data size: " << normal_data.size() << std::endl;

            height_map_.updateTextureLayer(height_data.data(), victim_index);
            normal_map_.updateTextureLayer(&normal_data[0].x, victim_index);

            // TODO: replace fBmGenerator with fBmGPUGenerator

            texture_index = victim_index;
            replacer_->pin(texture_index);
        }
    }

    void remove(int texture_index) override {
        replacer_->unpin(texture_index);
    }

    void bind(const Shader* shader) const {
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_2D_ARRAY, height_map_.getID());
        shader->setUniform("heightMap", 10);
        
        glActiveTexture(GL_TEXTURE11);
        glBindTexture(GL_TEXTURE_2D_ARRAY, normal_map_.getID());
        shader->setUniform("normalMap", 11);
    }

private:
    Replacer* replacer_;
    Texture height_map_;
    Texture normal_map_;
    TerrainGenerator& generator_;
    int base_resolution_ { 8 };
};

class QuadTree {
public:
    struct Node {
        int texture_index;
        int world_x;
        int world_z;
        int scale;
        bool is_leaf;
    };

    struct NodeRenderData {
        glm::mat4 transform;
        int texture_index;
    };

    QuadTree(int c_x, int c_z, int max_level, TexturePoolInterface* texture_pool): center_x_(c_x), center_z_(c_z), max_level_(max_level), texture_pool_(texture_pool) {
        int num_nodes = ((1 << ((max_level + 1) << 1)) - 1) / 3;
        int index = 0;

        nodes_.resize(num_nodes);
        build_quad_tree(center_x_, center_z_, 0, index);

        // std::cout << "[terrain] index = " << index << ", num_nodes = " << num_nodes << std::endl;
        assert(index == num_nodes);
    }

    void build_quad_tree(int current_x, int current_z, int depth, int &index) {
        int scale = 1 << (max_level_ - depth);

        nodes_[index].texture_index = -1;
        nodes_[index].world_x = current_x;
        nodes_[index].world_z = current_z;
        nodes_[index].scale = scale;
        nodes_[index].is_leaf = 0;
        index++;

        if (depth != max_level_) {
            int child_scale = scale >> 1;
            build_quad_tree(current_x, current_z, depth + 1, index);
            build_quad_tree(current_x + child_scale, current_z, depth + 1, index);
            build_quad_tree(current_x, current_z + child_scale, depth + 1, index);
            build_quad_tree(current_x + child_scale, current_z + child_scale, depth + 1, index);
        }
    }

    void initializeTree(int index, float camera_x, float camera_z) {
        if (index < 0 || index >= nodes_.size()) {
            return;
        }

        // std::cout << "index: " << index << ", node_pos: (" << nodes_[index].world_x << ", " << nodes_[index].world_z << "), scale: " << nodes_[index].scale << std::endl;

        Node& node = nodes_[index];
        if (node.scale == 1) {
            node.is_leaf = true;
            texture_pool_->insert(node.world_x, node.world_z, node.scale, node.texture_index);
            return;
        }

        // glm::vec2 center = glm::vec2(node.world_x, node.world_z) + glm::vec2(node.scale) / 2.0f;
        float dist_sqr = getDistSqrToCenter(node, camera_x, camera_z);

        // std::cout << "center: " << center.x << ", " << center.y << std::endl;
        // std::cout << "dist^2: " << getDistSqrToCenter(node, camera_x, camera_z) << std::endl;
        // std::cout << "split: " << (dist_sqr < node.scale * node.scale * kSplitThreshold * kSplitThreshold) << std::endl;
        // std::cout << std::endl;

        if (dist_sqr < node.scale * node.scale * kSplitThreshold * kSplitThreshold) {
            int first_child = index + 1;
            int skip_index = get_skip_index(nodes_[first_child].scale);
            initializeTree(first_child, camera_x, camera_z);
            initializeTree(first_child + skip_index, camera_x, camera_z);
            initializeTree(first_child + 2 * skip_index, camera_x, camera_z);
            initializeTree(first_child + 3 * skip_index, camera_x, camera_z);
        } else {
            node.is_leaf = true;
            texture_pool_->insert(node.world_x, node.world_z, node.scale, node.texture_index);
        }
    }

    void updateTree(int index, float camera_x, float camera_z) {
        if (index < 0 || index >= nodes_.size()) {
            return;
        }

        Node& node = nodes_[index];
        float dist_sqr = getDistSqrToCenter(node, camera_x, camera_z);
        int scale_sqr = node.scale * node.scale;

        if (node.is_leaf) {
            if (node.scale > 1 && dist_sqr < scale_sqr * kSplitThreshold * kSplitThreshold) {
                split(index);
            } else {
                return;
            }
        }

        int child_index = index + 1;
        int skip_index = get_skip_index(nodes_[child_index].scale);
        updateTree(child_index, camera_x, camera_z);
        updateTree(child_index + skip_index, camera_x, camera_z);
        updateTree(child_index + 2 * skip_index, camera_x, camera_z);
        updateTree(child_index + 3 * skip_index, camera_x, camera_z);

        if (dist_sqr > scale_sqr * kMergeThreshold * kMergeThreshold) {
            bool all_leaves = true;
            for (int i = 0; i < 4; ++i) {
                int child_index = index + 1 + i * skip_index;
                if (!nodes_[child_index].is_leaf) {
                    all_leaves = false;
                    break;
                }
            }
            if (all_leaves) {
                merge(index);
            }
        }
    }

    void split(int index) {
        if (index < 0 || index >= nodes_.size()) {
            return;
        }
        if (!nodes_[index].is_leaf || nodes_[index].scale == 1) {
            return;
        }

        nodes_[index].is_leaf = false;
        texture_pool_->remove(nodes_[index].texture_index);

        int child_index = index + 1;
        int skip_index = get_skip_index(nodes_[child_index].scale);
        for (int i = 0; i < 4; ++i) {
            nodes_[child_index].is_leaf = true;
            texture_pool_->insert(
                nodes_[child_index].world_x,
                nodes_[child_index].world_z,
                nodes_[child_index].scale,
                nodes_[child_index].texture_index
            );

            child_index += skip_index;
        }
    }

    void merge(int index) {
        if (index < 0 || index >= nodes_.size()) {
            return;
        }
        if (nodes_[index].is_leaf || nodes_[index].scale == 1) {
            return;
        }

        int child_index = index + 1;
        int skip_index = get_skip_index(nodes_[child_index].scale);
        for (int i = 0; i < 4; ++i) {
            nodes_[child_index].is_leaf = false;
            texture_pool_->remove(nodes_[child_index].texture_index);

            child_index += skip_index;
        }

        nodes_[index].is_leaf = true;
        texture_pool_->insert(
            nodes_[index].world_x,
            nodes_[index].world_z,
            nodes_[index].scale,
            nodes_[index].texture_index
        );
    }

    void update(float camera_x, float camera_z) {
        if (!nodes_.empty()) {
            updateTree(0, camera_x, camera_z);
        }
    }

    std::vector<NodeRenderData> getLeaves() const {
        std::vector<NodeRenderData> leaves;
        for (const auto& node : nodes_) {
            if (node.is_leaf) {
                leaves.emplace_back(getTransform(node), node.texture_index);
            }
        }

        return leaves;
    }

    void outputLeaves() const {
        for (const auto& node : nodes_) {
            if (node.is_leaf) {
                std::cout << "leaf: " << node.world_x << ", " << node.world_z << ", " << node.scale << ", " << node.texture_index << std::endl;
            }
        }
    }

private:

    static constexpr float kSplitThreshold = 2.0f;
    static constexpr float kMergeThreshold = 2.5f;

    int get_skip_index(int scale) {
        return (4 * scale * scale - 1) / 3;
    }

    glm::mat4 getTransform(const Node& node) const {
        glm::mat4 transform = glm::mat4(1.0f);
        transform = glm::translate(transform, glm::vec3(node.world_x * Chunk::X_LENGTH, 0, node.world_z * Chunk::Z_LENGTH));
        transform = glm::scale(transform, glm::vec3(node.scale, 1, node.scale));
        return transform;
    }

    float getDistSqrToCenter(Node& node, float x, float z) {
        glm::vec2 center = glm::vec2(node.world_x, node.world_z) + glm::vec2(node.scale) / 2.0f;
        glm::vec2 vec = glm::vec2(x / Chunk::X_LENGTH, z / Chunk::Z_LENGTH) - center;
        return glm::dot(vec, vec);
    }

    int center_x_;
    int center_z_;
    int max_level_;
    std::vector<Node> nodes_;
    TexturePoolInterface* texture_pool_ { nullptr };
};


class TerrainModel {
    friend class Terrain_;

public:
    struct __attribute__((packed)) TerrainVertex {
        glm::vec2 position;
        glm::vec2 tex_coord;
        int is_skirt;
    };

    TerrainModel(int base_resolution = 8) : base_resolution_(base_resolution) {
        generateMesh();
        generateIndexBuffer(base_resolution_);
    }

    ~TerrainModel() {
        if (VAO_ != INVALID_VAO) {
            glDeleteVertexArrays(1, &VAO_);
        }
        if (VBO_ != INVALID_VBO) {
            glDeleteBuffers(1, &VBO_);
        }
        for (auto [resolution, EBO] : EBOs_) {
            if (EBO != INVALID_EBO) {
                glDeleteBuffers(1, &EBO);
            }
        }
    }

    TerrainModel(const TerrainModel&) = delete;
    TerrainModel& operator=(const TerrainModel&) = delete;

    void bind(int resolution = -1) {
        if (resolution == -1) {
            resolution = base_resolution_;
        }

        glBindVertexArray(VAO_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOs_[resolution]);
    }

    void draw(int resolution = -1) {
        if (resolution == -1) {
            resolution = base_resolution_;
        }
        glDrawElements(GL_TRIANGLES, 3 * (2 * resolution + 8) * resolution, GL_UNSIGNED_INT, nullptr);
    }

private:
    void generateMesh() {
        int length = base_resolution_ + 1;
        int stride_x = Chunk::X_LENGTH / base_resolution_;
        int stride_z = Chunk::Z_LENGTH / base_resolution_;
        std::vector<TerrainVertex> vertices(length * length + base_resolution_ * 4);

        for (int i = 0; i <= base_resolution_; ++i) {
            for (int j = 0; j <= base_resolution_; ++j) {
                vertices[i + j * length].position = glm::vec2(i * stride_x, j * stride_z);
                vertices[i + j * length].tex_coord = glm::vec2(i / static_cast<float>(base_resolution_), j / static_cast<float>(base_resolution_));
                vertices[i + j * length].is_skirt = 0;
            }
        }

        int cur = length * length;
        skirt_map_.resize(4 * base_resolution_);

        for (int i = 0; i < base_resolution_; ++i) {
            vertices[cur].position = glm::vec2(i * stride_x, 0);
            vertices[cur].tex_coord = glm::vec2(i / static_cast<float>(base_resolution_), 0);
            vertices[cur].is_skirt = 1;
            skirt_map_[cur - length * length] = i;
            cur++;
        }

        for (int j = 0; j < base_resolution_; ++j) {
            vertices[cur].position = glm::vec2(Chunk::X_LENGTH, j * stride_z);
            vertices[cur].tex_coord = glm::vec2(1, j / static_cast<float>(base_resolution_));
            vertices[cur].is_skirt = 1;
            skirt_map_[cur - length * length] = base_resolution_ + length * j;
            cur++;
        }

        for (int i = base_resolution_; i >= 1; --i) {
            vertices[cur].position = glm::vec2(i * stride_x, Chunk::Z_LENGTH);
            vertices[cur].tex_coord = glm::vec2(i / static_cast<float>(base_resolution_), 1);
            vertices[cur].is_skirt = 1;
            skirt_map_[cur - length * length] = i + base_resolution_ * length;
            cur++;
        }

        for (int j = base_resolution_; j >= 1; --j) {
            vertices[cur].position = glm::vec2(0, j * stride_z);
            vertices[cur].tex_coord = glm::vec2(0, j / static_cast<float>(base_resolution_));
            vertices[cur].is_skirt = 1;
            skirt_map_[cur - length * length] = length * j;
            cur++;
        }

        glGenVertexArrays(1, &VAO_);
        glGenBuffers(1, &VBO_);

        glBindVertexArray(VAO_);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(TerrainVertex), vertices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, position));
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, tex_coord));
        glVertexAttribIPointer(2, 1, GL_INT, sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, is_skirt));

        glBindVertexArray(INVALID_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, INVALID_VBO);
    }

    void generateIndexBuffer(int resolution) {
        int index_stride = base_resolution_ / resolution;
        std::vector<GLuint> indices(3 * (2 * resolution + 8) * resolution);

        // std::cout << "plain size: " << resolution * resolution << std::endl;

        int length = base_resolution_ + 1;
        int cur = 0;
        for (int i = 0; i < resolution; ++i) {
            for (int j = 0; j < resolution; ++j) {
                int is = i * index_stride;
                int js = j * index_stride;
                int left_down = is + js * length;
                int right_down = left_down + index_stride;

                int left_up = left_down + length * index_stride;
                int right_up = right_down + length * index_stride;

                indices[cur++] = left_down;
                indices[cur++] = right_down;
                indices[cur++] = left_up;
                indices[cur++] = right_down;
                indices[cur++] = right_up;
                indices[cur++] = left_up;
                // std::cout << left_down << ", " << right_down << ", " << left_up << ", " << right_up << std::endl;
            }
        }

        // std::cout << "skirt_map size: " << skirt_map_.size() << std::endl;

        int skirt_size = skirt_map_.size();
        for (size_t i = 0; i < skirt_map_.size(); i += index_stride) {
            int left_down = i + length * length;
            int right_down = (i + index_stride) % skirt_size + length * length;
            int left_up = skirt_map_[i];
            int right_up = skirt_map_[(i + index_stride) % skirt_size];

            // std::cout << left_down << ", " << right_down << ", " << left_up << ", " << right_up << std::endl;

            indices[cur++] = left_down;
            indices[cur++] = right_down;
            indices[cur++] = left_up;
            indices[cur++] = right_down;
            indices[cur++] = right_up;
            indices[cur++] = left_up;
        }

        GLuint EBO;
        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, INVALID_EBO);
        
        EBOs_[resolution] = EBO;
    }

    GLuint VAO_ { INVALID_VAO };
    GLuint VBO_ { INVALID_VAO };

    std::unordered_map<int, GLuint> EBOs_;

    std::vector<int> skirt_map_;

    int base_resolution_ { 8 };
};



class Terrain_ {
public:
    Terrain_(
        TerrainGenerator& generator,
        int c_x,
        int c_z,
        int max_level,
        int base_resolution = 8
    ) : texture_pool_(generator, base_resolution), quad_tree_(c_x, c_z, max_level, &texture_pool_), seed_model_(base_resolution), generator_(generator) {
        initMaterial();
        quad_tree_.initializeTree(0, 0.0f, 0.0f);
    }

    void update(const Camera& camera) {
        glm::vec3 camera_pos = camera.getPosition();
        quad_tree_.update(camera_pos.x, camera_pos.z);
        // quad_tree_.outputLeaves();
    }

    void setShadowMap(const ShadowMap* shadow_map, const Shader* depth_shader) {
        shadow_map_ = shadow_map;
        depth_shader_ = depth_shader;
    }

    void render(const glm::mat4& view, const glm::mat4& projection, const Light& light) {
        auto shader_manager = ServiceLocator<ShaderManager>::get();
        auto shader = shader_manager->getShader("terrain");

        shader->useShader();
        shader->setUniform("shadowMap", 15);

        if (depth_shader_ != nullptr && shadow_map_ != nullptr) {
            // Bind shadow map texture
            glActiveTexture(GL_TEXTURE15);
            glBindTexture(GL_TEXTURE_2D, shadow_map_->getDepthMap());
            shader->setUniform("lightSpaceMatrix", light.getLightSpaceMatrix());
        }

        light.use(shader);
        material_->apply();

        texture_pool_.bind(shader);

        shader->setUniform("view", view);
        shader->setUniform("projection", projection);
        // TODO: bind other parameters
        shader->setUniform("uvTiling", 32.0f);
        shader->setUniform("skirtDepth", 10.0f);
        
        seed_model_.bind();
        auto leaves = quad_tree_.getLeaves();
        // std::cout << "leaves size: " << leaves.size() << std::endl;
        for (const auto& leaf : leaves) {
            shader->setUniform("model", leaf.transform);
            shader->setUniform("layerIndex", leaf.texture_index);

            seed_model_.draw();
        }
    }

    std::shared_ptr<Material> getMaterial() const {
        return material_;
    }

private:
    void initMaterial() {
        auto shader_manager = ServiceLocator<ShaderManager>::get();
        auto shader = shader_manager->getShader("terrain");
        auto material = std::make_shared<Material>(shader);

        auto texture_manager = ServiceLocator<TextureManager>::get();

        std::cout << "[terrain] initializing material" << std::endl;
        auto diffuse = texture_manager->getTexture("terrain", std::vector<std::string>{
            getAssetPath("textures/GroundSand005/GroundSand005_COL_2K.jpg"),                         // Sand
            getAssetPath("textures/grass_2k/Poliigon_GrassPatchyGround_4585_BaseColor.jpg"),         // Grass
            getAssetPath("textures/GroundDirtRocky020/GroundDirtRocky020_COL_2K.jpg"),               // Rock
            getAssetPath("textures/Poliigon_PlasterPainted_7664/2K/Poliigon_PlasterPainted_7664_BaseColor.jpg") // Snow
        }, GL_TEXTURE_2D_ARRAY);

        std::cout << "[terrain] initializing material done" << std::endl;

        material->setTexture("Diffuse", diffuse);
        material->setConstant("Diffuse", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        material->setConstant("Specular", glm::vec3(0.35f, 0.35f, 0.35f));
        material->setConstant("Metallic", 0.0f);
        material->setConstant("Roughness", 0.8f);

        material_ = material;
    }


    static constexpr unsigned int kPoolSize = 1024;

    TexturePool<kPoolSize> texture_pool_;
    QuadTree quad_tree_;
    TerrainModel seed_model_;
    TerrainGenerator& generator_;

    const Shader* depth_shader_ = nullptr;
    const ShadowMap* shadow_map_ = nullptr;

    std::shared_ptr<Material> material_;
};
