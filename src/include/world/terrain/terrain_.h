#pragma once

#include "resource/texture.h"
#include "replacer.h"
#include "world/terrain/terrain.h"
#include <glm/glm.hpp>

class TexturePoolInterface {
public:
    virtual ~TexturePoolInterface() = default;
    virtual void insert(int world_x, int world_z, int scale, int &texture_index) = 0;
    virtual void remove(int texture_index) = 0;
};

template<unsigned int PoolSize = 1024, typename ReplacerType = LRUReplacer>
class TexturePool : public TexturePoolInterface {
public:
    TexturePool(TerrainGenerator& generator): generator_(generator) {
        if (!std::is_base_of_v<Replacer, ReplacerType>) {
            static_assert(false, "ReplacerType must be a subclass of Replacer");
        }

        replacer_ = new ReplacerType(PoolSize);
        height_map_ = new Texture(GL_TEXTURE_2D_ARRAY, "HeightMap");
        normal_map_ = new Texture(GL_TEXTURE_2D_ARRAY, "NormalMap");

        height_map_->Load<float>(nullptr, Chunk::width, Chunk::length, 1, PoolSize);
        normal_map_->Load<float>(nullptr, Chunk::width, Chunk::length, 3, PoolSize);
    }

    ~TexturePool() {
        delete replacer_;
        delete height_map_;
        delete normal_map_;
    }

    void insert(int world_x, int world_z, int scale, int& texture_index) override {
        int victim_index;
        if (replacer_->victim(victim_index)) {
            std::vector<float> height_data = generator_.getHeightChunk({world_x - scale / 2, world_z - scale / 2}, scale, 4);

            height_map_->updateTextureLayer(height_data.data(), victim_index);

            texture_index = victim_index;
            replacer_->pin(texture_index);
        }
    }

    void remove(int texture_index) override {
        replacer_->unpin(texture_index);
    }

private:
    Replacer* replacer_;
    Texture* height_map_;
    Texture* normal_map_;
    TerrainGenerator& generator_;
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

    QuadTree(int c_x, int c_z, int max_level, TexturePoolInterface* texture_pool): center_x_(c_x), center_z_(c_z), max_level_(max_level), texture_pool_(texture_pool) {
        int num_nodes = ((1 << (max_level << 1)) - 1) / 3;
        int index = 0;

        nodes_.resize(num_nodes);
        build_quad_tree(center_x_, center_z_, 0, index);
        assert(index == num_nodes);
    }

    void build_quad_tree(int current_x, int current_z, int depth, int &index) {
        nodes_[index].texture_index = -1;
        nodes_[index].world_x = current_x;
        nodes_[index].world_z = current_z;
        nodes_[index].scale = 1 << (max_level_ - depth);
        nodes_[index].is_leaf = index == 0;
        index++;

        if (depth != max_level_) {
            int child_scale = nodes_[index].scale >> 1;
            build_quad_tree(current_x - child_scale, current_z - child_scale, depth + 1, index);
            build_quad_tree(current_x + child_scale, current_z - child_scale, depth + 1, index);
            build_quad_tree(current_x - child_scale, current_z + child_scale, depth + 1, index);
            build_quad_tree(current_x + child_scale, current_z + child_scale, depth + 1, index);
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

private:
    int get_skip_index(int scale) {
        return (4 * scale * scale - 1) / 3;
    }

    int center_x_;
    int center_z_;
    int max_level_;
    std::vector<Node> nodes_;
    TexturePoolInterface* texture_pool_ { nullptr };
};

class TerrainModel {

};




class Terrain_ {
public:

private:
    static constexpr unsigned int kPoolSize = 1024;

    QuadTree quad_tree_;
    TexturePool<kPoolSize> texture_pool_;
    std::vector<TerrainModel> seed_model_;
    TerrainGenerator& generator_;
};
