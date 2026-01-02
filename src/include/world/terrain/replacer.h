#pragma once

#include <cstddef>
#include <unordered_set>
#include <list>
#include <iostream>

/**
 * Replacer is an abstract class that tracks chunk usage.
 */
class Replacer {
public:
    Replacer() = default;

    virtual ~Replacer() = default;

    /**
    * Remove the victim texture as defined by the replacement policy.
    * @param[out] texture_id id of texture that was removed, -1 if no victim was found
    * @return true if a victim texture was found, false otherwise
    */
    virtual bool victim(int &texture_id) = 0;

    /**
    * Pins a texture, indicating that it should not be victimized until it is unpinned.
    * @param texture_id the id of the texture to pin
    */
    virtual void pin(int texture_id) = 0;

    /**
    * Unpins a texture, indicating that it can now be victimized.
    * @param texture_id the id of the texture to unpin
    */
    virtual void unpin(int texture_id) = 0;

    /** @return the number of elements in the replacer that can be victimized */
    virtual size_t size() = 0;
};


/**
 * LRUReplacer implements the Least Recently Used replacement policy.
 */
class LRUReplacer : public Replacer {
public:
    /**
    * Create a new LRUReplacer.
    * @param num_textures the maximum number of textures the LRUReplacer will be required to store
    */
    explicit LRUReplacer(size_t num_textures): num_textures_(num_textures) {
        for (int i = 0; i < num_textures_; ++i) {
            unpin(i);
        }
    }

    /**
    * Destroys the LRUReplacer.
    */
    ~LRUReplacer() override = default;

    bool victim(int &texture_id) override {
        if (lru_used_textures_.empty()) {
            std::cerr << "No frame available for eviction" << std::endl;
            texture_id = INVALID_TEXTURE_ID;
            return false;
        }

        int victim_texture = lru_list_.back();

        if (victim_texture == INVALID_TEXTURE_ID) {
            texture_id = INVALID_TEXTURE_ID;
            return false;
        }

        lru_list_.pop_back();
        lru_used_textures_.erase(victim_texture);
        texture_id = victim_texture;

        return true;
    }

    void pin(int texture_id) override {
        if (texture_id == INVALID_TEXTURE_ID) {
            std::cerr << "Invalid frame id: " << texture_id << std::endl;
            return;
        }

        auto it = lru_used_textures_.find(texture_id);
        if (it == lru_used_textures_.end()) {
            return;
        }

        lru_list_.remove(texture_id);
        lru_used_textures_.erase(it);
    }

    void unpin(int texture_id) override {
        if (texture_id == INVALID_TEXTURE_ID) {
            std::cerr << "Invalid frame id: " << texture_id << std::endl;
            return;
        }

        auto it = lru_used_textures_.find(texture_id);
        if (it != lru_used_textures_.end()) {
            return;
        }

        lru_list_.push_front(texture_id);
        lru_used_textures_.insert(texture_id);
    }

    size_t size() override {
        return lru_used_textures_.size();
    }

private:
    static constexpr int INVALID_TEXTURE_ID = -1;

    typedef std::unordered_set<int> lru_set_t;
    typedef std::list<int> lru_list_t;

    size_t num_textures_;
    lru_list_t lru_list_;
    lru_set_t lru_used_textures_;
};

template<typename T>
concept ReplacerType = std::is_base_of_v<Replacer, std::remove_cvref_t<T>>;
