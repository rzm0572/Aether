#pragma once

#include "common/shape.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct CollisionInstance;
struct TerrainGenerator;
enum class CollisionLayer;

enum class BulletType {
    COMMON,
    FireBall,
    Autocannon,
    kCount,
};

struct Bullet {
    BulletType type;
    glm::vec3 position;
    glm::vec3 prev_position;
    glm::vec3 velocity;
    Capsule collider;
    CollisionLayer layer;

    bool collided { false };
    float shoot_time { 0.0f };
};

class BulletManager {
public:
    BulletManager() {
        bullets_.reserve(MAX_BULLETS);
    }
    
    void fire(BulletType type, glm::vec3 position, glm::vec3 velocity, float now, CollisionLayer layer);

    void update(float dt) {
        for (auto& bullet : bullets_) {
            bullet.prev_position = bullet.position;
            bullet.position = bullet.position + bullet.velocity * dt;
            bullet.velocity = (bullet.velocity + gravity * dt) * velocity_damping;
            
            bullet.collider.point_1 = bullet.prev_position;
            bullet.collider.point_2 = bullet.position;
            bullet.collider.radius = 0.05f;
        }
    }

    std::vector<Bullet>& getBullets() {
        return bullets_;
    }

    void detectCollisions(const CollisionInstance& instance, float& damage);

    void detectCollisionsTerrain(const TerrainGenerator& tg);

    std::vector<glm::vec3> cleanBullets(float now) {
        std::vector<glm::vec3> explode_poses;
        size_t alive_bullets = 0;
        for (size_t i = 0; i < bullets_.size(); ++i) {
            if (now - bullets_[i].shoot_time > MAX_LIFE_TIME) {
                explode_poses.push_back(bullets_[i].position);
                continue;
            }
            if (bullets_[i].collided) {
                explode_poses.push_back(bullets_[i].position);
                continue;
            }

            bullets_[alive_bullets] = bullets_[i];
            alive_bullets++;
        }

        bullets_.resize(alive_bullets);
        return explode_poses;
    }

private:
    static constexpr size_t MAX_BULLETS = 2048;
    static constexpr float MAX_LIFE_TIME = 2.0f;
    static constexpr glm::vec3 gravity = glm::vec3(0.0f, -9.8f, 0.0f);
    static constexpr float velocity_damping = 0.9999f;

    std::vector<Bullet> bullets_;
};
