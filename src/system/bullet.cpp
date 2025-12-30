#include "system/bullet.h"
#include "entity/collision.h"
#include "system/collision.h"
#include "world/terrain/terrain.h"

struct BulletProperties {
    float damage { 1.0f };
} kBulletProperties[(size_t)BulletType::kCount] = {
    { 1.0f },
    { 3.0f },
    { 5.0f },
    { 3.0f },
    { 3.0f }
};

void BulletManager::fire(BulletType type, glm::vec3 position, glm::vec3 velocity, float now, CollisionLayer layer) {
    if (bullets_.size() >= MAX_BULLETS) {
        return;
    }

    bullets_.emplace_back(
        type, position, position, velocity, Capsule(), layer, false, now
    );
}

void BulletManager::detectCollisions(const CollisionInstance& instance, float& damage) {
    CollisionVisitor visitor(instance.global_transform, glm::mat4(1.0f));
    AABBVisitor aabb_visitor_bullet(glm::mat4(1.0f));

    AABB aabb_instance = instance.cc->getAABB(instance.global_transform);
    for (auto& bullet : bullets_) {
        if (bullet.collided) {
            continue;
        }

        uint8_t layer_mask = 1 << static_cast<uint8_t>(bullet.layer);
        if (!(instance.collisionable_mask & layer_mask)) {
            continue;
        }

        AABB aabb_bullet = aabb_visitor_bullet(bullet.collider);
        if (!AABB::isIntersectAABB(aabb_instance, aabb_bullet)) {
            continue;
        }

        for (auto& shape : instance.cc->getShapes()) {
            VoxelShape bullet_shape = bullet.collider;
            if (std::visit(visitor, shape, bullet_shape)) {
                bullet.collided = true;
                damage += kBulletProperties[(size_t)bullet.type].damage;
                break;
            }
        }
    }
}

void BulletManager::detectCollisionsTerrain(const TerrainGenerator& tg) {
    for (auto& bullet: bullets_) {
        if (bullet.collided) {
            continue;
        }

        glm::vec3 bullet_pos = bullet.position;
        float terrain_height = tg.getHeight(bullet_pos.x, bullet_pos.z);
        
        if (terrain_height > bullet_pos.y) {
            bullet.collided = true;
        }
    }
}
