#pragma once

#include "common/game_object.h"
#include "common/shape.h"
#include "entity/collision.h"
#include "entity/collision_draw.h"
#include "world/terrain.h"
#include "system/bullet.h"
#include "service/service_locator.h"
#include "resource/shader.h"

struct CollisionInstance {
    CollisionComponent* cc;
    CollisionObjectType type;
    glm::mat4 global_transform;
    AABB aabb;

    CollisionLayer layer;
    uint8_t collisionable_mask;
};

class CollisionSystem {
public:
    CollisionSystem() = default;
    CollisionSystem(const TerrainGenerator* terrain_generator): terrain_generator_(terrain_generator) {}

    void init() {
        auto shader_manager = ServiceLocator<ShaderManager>::get();
        if (shader_manager) {
            shader_manager->registerShader("debug_line", getShaderPath("debug_line.vert"), getShaderPath("debug_line.frag"));
        } else {
            std::cerr << "CollisionSystem::init() Failed to get ShaderManager" << std::endl;
        }
    }

    void clear() {}

    void submit(CollisionableObject* obj) {
        auto& cc = obj->getCollisionComponent();
        auto& tc = obj->getTransformComponent();
        glm::mat4 global_transform = tc.getGlobalModelMatrix();

        CollisionLayer layer = cc.getLayer();
        uint8_t collisionable_mask = 0xFF ^ (1 << static_cast<uint8_t>(layer));

        instances_.emplace_back(
            &cc,
            cc.getType(),
            global_transform,
            cc.getAABB(global_transform),
            layer,
            collisionable_mask
        );
    }

    void registerBulletManager(BulletManager* bullet_manager) {
        bullet_managers_.push_back(bullet_manager);
    }

    void setTerrainGenerator(const TerrainGenerator* terrain_generator) {
        terrain_generator_ = terrain_generator;
    }

    void update(float now) {
        for (size_t i = 0; i < instances_.size(); ++i) {
            for (size_t j = i + 1; j < instances_.size(); ++j) {
                if (detect(instances_[i], instances_[j])) {
                    instances_[i].cc->submitEvent(CollisionEvent(instances_[j].type, instances_[j].layer, now));
                    instances_[j].cc->submitEvent(CollisionEvent(instances_[i].type, instances_[i].layer, now));
                    // std::cout << "Collision: " << static_cast<int>(instances_[i].type) << " and " << static_cast<int>(instances_[j].type) << std::endl;
                }
            }
        }

        for (auto& instance : instances_) {
            if (detectTerrainInstance(instance)) {
                instance.cc->submitEvent(CollisionEvent(CollisionObjectType::TERRAIN, CollisionLayer::LAYER_NEUTRAL, now));
                // std::cout << "Collision: " << static_cast<int>(instance.type) << " and TERRAIN" << std::endl;
            }
        }

        for (auto* bullet_manager : bullet_managers_) {
            detectTerrainBullet(bullet_manager);
            for (auto& instance : instances_) {
                float damage = 0.0f;
                bullet_manager->detectCollisions(instance, damage);
                if (damage > 0.0f) {
                    CollisionLayer bullet_layer = CollisionLayer::LAYER_NEUTRAL;
                    if (instance.layer == CollisionLayer::LAYER_PLAYER) {
                        bullet_layer = CollisionLayer::LAYER_ENEMY;
                    } else if (instance.layer == CollisionLayer::LAYER_ENEMY) {
                        bullet_layer = CollisionLayer::LAYER_PLAYER;
                    }
                    instance.cc->submitEvent(CollisionEvent(CollisionObjectType::BULLET, bullet_layer, now, damage));
                    // std::cout << "Collision: " << static_cast<int>(instance.type) << " and BULLET" << std::endl;
                }
            }
        }

        instances_.clear();
    }

    void debugOutput() {
        for (auto instance : instances_) {
            std::cout << "type: " << static_cast<int>(instance.cc->getType())
                      << ", layer: " << static_cast<int>(instance.cc->getLayer()) << std::endl;
            auto shapes = instance.cc->getShapes();
            ShapeToStringVisitor visitor_;
            for (auto shape: shapes) {
                std::cout << std::visit(visitor_, shape) << std::endl;
            }
            std::cout << "mask: " << static_cast<int>(instance.collisionable_mask) << std::endl << std::endl;
        }
    }

    void generateDebugLines(std::vector<DebugLine>& render_lines) {
        for (const auto& instance : instances_) {
            // 根据碰撞层级设置颜色，方便区分敌我
            glm::vec3 color(1.0f); // 默认白色
            if (instance.layer == CollisionLayer::LAYER_PLAYER) {
                color = glm::vec3(0.0f, 1.0f, 0.0f); // 玩家绿色
            } else if (instance.layer == CollisionLayer::LAYER_ENEMY) {
                color = glm::vec3(1.0f, 0.0f, 0.0f); // 敌人红色
            } else {
                color = glm::vec3(0.0f, 0.0f, 1.0f); // 其他蓝色
            }

            // 构造 Visitor
            CollisionDebugVisitor visitor { instance.global_transform, render_lines, color };

            // 遍历该实例的所有形状
            for (const auto& shape : instance.cc->getShapes()) {
                std::visit(visitor, shape);
            }
            
            // 可选：同时也画出 AABB 方便调试 Broadphase
            // drawAABB(instance.aabb, render_lines);
        }
    }

private:

    uint8_t makeLayerMask(CollisionLayer layer) {
        return 1 << static_cast<uint8_t>(layer);
    }

    bool detect(CollisionInstance& a, CollisionInstance& b) {
        bool collisionable = (makeLayerMask(a.layer) & b.collisionable_mask) && (makeLayerMask(b.layer) & a.collisionable_mask);
        if (!collisionable) {
            return false;
        }

        if (!AABB::isIntersectAABB(a.aabb, b.aabb)) {
            return false;
        }

        CollisionVisitor visitor(a.global_transform, b.global_transform);
        for (const auto& shape1 : a.cc->getShapes()) {
            for (const auto& shape2 : b.cc->getShapes()) {
                if (std::visit(visitor, shape1, shape2)) {
                    return true;
                }
            }
        }
        return false;
    }

    bool detectTerrainInstance(CollisionInstance& instance) {
        if (terrain_generator_ == nullptr) {
            return false;
        }
        
        CollisionTerrainVisitor visitor(terrain_generator_, instance.global_transform);
        for (auto shape: instance.cc->getShapes()) {
            if (std::visit(visitor, shape)) {
                return true;
            }
        }

        return false;
    }

    void detectTerrainBullet(BulletManager* bullet_manager) {
        if (terrain_generator_ == nullptr) {
            return;
        }
        bullet_manager->detectCollisionsTerrain(*terrain_generator_);
    }

public:
    std::vector<CollisionInstance> instances_;
    std::vector<BulletManager*> bullet_managers_;
    const TerrainGenerator* terrain_generator_;
};
