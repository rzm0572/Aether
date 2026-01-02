#pragma once

#include "common/shape.h"
#include <string>
#include <vector>

enum class CollisionObjectType;

class CollisionConfig {
    friend class CollisionComponent;

public:
    CollisionConfig() = default;
    
    CollisionConfig(CollisionObjectType type, const std::vector<VoxelShape>& shape = {});

    CollisionConfig(std::string filename);

    static const CollisionConfig inCollisionableConfig;

    std::string toString() const {
        std::stringstream ss;
        ShapeToStringVisitor visitor;
        ss << "type: " << static_cast<int>(type_) << ", shape: " << std::endl;
        for (auto& shape : shape_) {
            ss << std::visit(visitor, shape) << std::endl;
        }
        return ss.str();
    }

private:
    CollisionObjectType type_;
    std::vector<VoxelShape> shape_;
};

struct CollisionConfigRegistry {
    std::unordered_map<std::string, CollisionConfig> registry;

    void load(std::string name, const std::string& filename) {
        registry[name] = CollisionConfig(filename);
    }

    void add(std::string name, CollisionConfig config) {
        registry[name] = config;
    }

    void initialize();

    void output() {
        for (auto& [name, config] : registry) {
            std::cout << "CollisionConfig " << name << ":\n" << config.toString() << std::endl;
        }
    }

    static CollisionConfigRegistry& getInstance() {
        static CollisionConfigRegistry registry;
        return registry;
    }
};
