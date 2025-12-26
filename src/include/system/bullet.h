#pragma once

#include <glm/glm.hpp>

enum class BulletType {
    COMMON,
    MAGIC,
};

struct Bullet {
    BulletType type;
    glm::vec3 position;
    glm::vec3 velocity;

    bool collided { false };
    float shoot_time { 0.0f };
};

class BulletManager {
public:
    BulletManager() {
        bullets_.reserve(MAX_BULLETS);
    }
    
    void fire(BulletType type, glm::vec3 position, glm::vec3 velocity, float now) {
        if (bullets_.size() >= MAX_BULLETS) {
            return;
        }

        bullets_.emplace_back(
            type, position, velocity, false, now
        );
    }

    std::vector<Bullet>& getBullets() {
        return bullets_;
    }

    void cleanBullets(float now) {
        size_t alive_bullets = 0;
        for (int i = 0; i < bullets_.size(); ++i) {
            if (now - bullets_[i].shoot_time > MAX_LIFE_TIME) {
                continue;
            }
            if (bullets_[i].collided) {
                continue;
            }

            bullets_[alive_bullets] = bullets_[i];
            alive_bullets++;
        }

        bullets_.resize(alive_bullets);
    }

private:
    static constexpr size_t MAX_BULLETS = 16384;
    static constexpr float MAX_LIFE_TIME = 20.0f;

    std::vector<Bullet> bullets_;
};
