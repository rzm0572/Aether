#include <entity/collision_config.h>
#include <entity/collision.h>

CollisionConfig::CollisionConfig(CollisionObjectType type, const std::vector<VoxelShape>& shape) : type_(type), shape_(shape) {}

CollisionConfig::CollisionConfig(std::string filename) {
    // TODO
}

const CollisionConfig CollisionConfig::inCollisionableConfig = CollisionConfig(CollisionObjectType::UNKNOWN);

void CollisionConfigRegistry::initialize() {
    registry["j-10"] = {
        CollisionObjectType::PLANE,
        {
            Capsule(1.0f, glm::vec3(0.7f, 0.0f, 0.0f), glm::vec3(12.0f, 0.0f, 0.0f)),
            Triangle(glm::vec3(1.9f, -0.4f, 4.6f), glm::vec3(8.0f, -0.4f, 0.0f), glm::vec3(1.9f, -0.4f, -4.6f)),
            Triangle(glm::vec3(0.3f, 0.0f, 0.0f), glm::vec3(-0.9f, 3.6f, 0.0f), glm::vec3(4.5f, 0.0f, 0.0f)),
        }
    };
    registry["missile"] = {
        CollisionObjectType::MISSILE,
        {
            // Capsule(0.5f, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(5.0f, 0.0f, 0.0f)),
            Sphere(5.0f, glm::vec3(2.7f, 0.0f, 0.0f)),
        }
    };
    registry["bomb"] = {
        CollisionObjectType::BOMB,
        {
            Sphere(5.0f, glm::vec3(2.7f, 0.0f, 0.0f)),
        }
    };
}
