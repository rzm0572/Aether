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

    Bullet(BulletType type, glm::vec3 position, glm::vec3 velocity, float shoot_time) : type(type), position(position), velocity(velocity), collided(false), shoot_time(shoot_time) {}
};

class BulletManager {
public:
    BulletManager() {
        bullets_.reserve(capacity_);
    }
    
    void fire(BulletType type, glm::vec3 position, glm::vec3 velocity, float now) {
        if (count_ >= MAX_BULLETS) {
            return;
        }

        while (count_ >= capacity_) {
            expand();
        }

        bullets_.emplace_back(
            type, position, velocity, now
        );
        count_++;
    }

    void cleanBullets(float now) {
        std::vector<Bullet> bullets_remaining;
        bullets_remaining.reserve(capacity_);
        for (int i = 0; i < count_; ++i) {
            if (now - bullets_[i].shoot_time > MAX_LIFE_TIME) {
                continue;
            }
            if (bullets_[i].collided) {
                continue;
            }
            bullets_remaining.emplace_back(bullets_[i]);
        }

        bullets_.swap(bullets_remaining);
    }

private:
    static constexpr int MAX_BULLETS = 16384;
    static constexpr float MAX_LIFE_TIME = 20.0f;

    void expand() {
        if (capacity_ * 2 > MAX_BULLETS) {
            return;
        }

        capacity_ = capacity_ * 2;
        bullets_.reserve(capacity_);
    }

    std::vector<Bullet> bullets_;
    int capacity_ { 128 };
    int count_ { 0 };
};
