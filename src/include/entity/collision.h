#pragma once

#include "common/game_object.h"
#include "common/shape.h"
#include "entity/collision_config.h"
#include "glm/fwd.hpp"
#include "world/terrain/terrain.h"
#include <glm/glm.hpp>
#include <glm/gtx/intersect.hpp>
#include <vector>

template <typename T>
concept DerivedFromShape = std::is_base_of_v<Shape, std::remove_cvref_t<T>>;

enum class CollisionObjectType {
    PLANE,
    MISSILE,
    BOMB,
    BULLET,
    TERRAIN,
    UNKNOWN,
};

enum class CollisionLayer {
    LAYER_NEUTRAL,
    LAYER_PLAYER,
    LAYER_ENEMY,
};

struct CollisionVisitor {
    static constexpr float kEpsilon = 1e-5f;

    const glm::mat4 global_transform_1;
    const glm::mat4 global_transform_2;

    CollisionVisitor(const glm::mat4& global_transform_1, const glm::mat4& global_transform_2): global_transform_1(global_transform_1), global_transform_2(global_transform_2) {}

    glm::vec3 get_global_position(const glm::vec3& local_position, const glm::mat4& global_transform) const {
        return glm::vec3(global_transform * glm::vec4(local_position, 1.0f));
    }

    float distSqrPointSegment(glm::vec3 C, glm::vec3 A, glm::vec3 B) const {
        glm::vec3 AB = B - A;
        glm::vec3 AC = C - A;
        float s = glm::clamp(glm::dot(AB, AC) / glm::dot(AB, AB), 0.0f, 1.0f);
        glm::vec3 H = A + AB * s;
        glm::vec3 CH = H - C;

        return glm::dot(CH, CH);
    }

    float distSqrSegmentSegment(glm::vec3 P1, glm::vec3 Q1, glm::vec3 P2, glm::vec3 Q2) const {
        glm::vec3 d1 = Q1 - P1;
        glm::vec3 d2 = Q2 - P2;
        glm::vec3 r = P2 - P1;

        float d1_dot_d1 = glm::dot(d1, d1);
        float d2_dot_d2 = glm::dot(d2, d2);

        if (d1_dot_d1 < kEpsilon && d2_dot_d2 < kEpsilon) {
            return glm::dot(r, r);
        }
        if (d1_dot_d1 < kEpsilon) {
            return distSqrPointSegment(P1, P2, Q2);
        }
        if (d2_dot_d2 < kEpsilon) {
            return distSqrPointSegment(P2, P1, Q1);
        }

        float d1_dot_r = glm::dot(d1, r);
        float d2_dot_r = glm::dot(d2, r);
        float d1_dot_d2 = glm::dot(d1, d2);

        // Assume P1Q1 and P2Q2 are lines, we want to calculate the distance between the two lines.
        // Let V1 and V2 be the closest points on the two lines.
        // V1 = P1 + d1 * s
        // V2 = P2 + d2 * t
        // We have V1V2 \cdot P1Q1 = 0 and V1V2 \cdot P2Q2 = 0.
        // Then we have
        // a) |P1Q1|^2 s - P1Q1 \cdot P2Q2 t = 0
        // b) P1Q1 \cdot P2Q2 s - |P2Q2|^2 t = 0
        // We can solve the two equations to get s.
        float p = d1_dot_d2 * d1_dot_d2 - d1_dot_d1 * d2_dot_d2;
        float s = 0.0;
        if (glm::abs(p) >= kEpsilon) {
            s = (d1_dot_d2 * d2_dot_r - d1_dot_r * d2_dot_d2) / p;
            s = glm::clamp(s, 0.0f, 1.0f);
        }

        // With s, we can calculate V1.
        // Let R1 and R2 be the closest points on the two line segments.
        // R1 = P1 + d1 * s', s' \in [0, 1]
        // R2 = P2 + d2 * t', t' \in [0, 1]
        // We can use V1 to calculate R2.
        glm::vec3 V1 = P1 + d1 * s;
        glm::vec3 d3 = V1 - P2;
        float d3_dot_d2 = glm::dot(d3, d2);

        float t = 0.0f;
        if (d3_dot_d2 > d2_dot_d2) {
            t = 1.0f;
        } else if (d3_dot_d2 > 0.0f) {
            t = d3_dot_d2 / d2_dot_d2;
        }

        glm::vec3 R2 = P2 + d2 * t;
        return distSqrPointSegment(R2, P1, Q1);
    }

    bool insideTriangle(glm::vec3 P, glm::vec3 A, glm::vec3 B, glm::vec3 C) const {
        glm::vec3 AB = B - A;
        glm::vec3 AC = C - A;
        glm::vec3 BC = C - B;
        glm::vec3 AP = P - A;
        glm::vec3 BP = P - B;

        glm::vec3 n = glm::cross(AB, AC);

        if (glm::dot(glm::cross(AB, AP), n) < 0.0f) {
            return false;
        }
        if (glm::dot(glm::cross(AP, AC), n) < 0.0f) {
            return false;
        }
        if (glm::dot(glm::cross(BC, BP), n) < 0.0f) {
            return false;
        }

        return true;
    }

    float distSqrPointTriangle(glm::vec3 P, glm::vec3 A, glm::vec3 B, glm::vec3 C) const {
        glm::vec3 AB = B - A;
        glm::vec3 AC = C - A;
        glm::vec3 AP = P - A;
        glm::vec3 n = glm::normalize(glm::cross(AB, AC));
        float d = glm::dot(n, AP);
        glm::vec3 H = P - n * d;

        bool inside = insideTriangle(H, A, B, C);

        if (inside) {
            return d * d;
        } else {
            float d1 = distSqrPointSegment(P, A, B);
            float d2 = distSqrPointSegment(P, B, C);
            float d3 = distSqrPointSegment(P, C, A);

            return std::min(d1, std::min(d2, d3));
        }
    }

    bool isIntersectCapsuleTriangle(glm::vec3 P, glm::vec3 Q, glm::vec3 A, glm::vec3 B, glm::vec3 C, float r) const {
        if (glm::length(Q - P) < kEpsilon) {
            return distSqrPointTriangle(P, A, B, C) <= r * r;
        }

        if (distSqrPointTriangle(P, A, B, C) <= r * r || distSqrPointTriangle(Q, A, B, C) <= r * r) {
            return true;
        }

        float t = 0.0f;
        glm::vec2 u = glm::vec2(0.0f);

        if (glm::intersectRayTriangle(P, Q - P, A, B, C, u, t) && t >= 0.0f && t <= 1.0f) {
            return true;
        }

        // For enough thin capsules, we just regard them as line segments.
        if (r < 0.1f) {
            return false;
        }

        if (distSqrSegmentSegment(P, Q, A, B) <= r * r || distSqrSegmentSegment(P, Q, B, C) <= r * r || distSqrSegmentSegment(P, Q, C, A) <= r * r) {
            return true;
        }

        return false;
    }

    bool operator()(const Sphere& s1, const Sphere& s2) const {
        glm::vec3 center_1 = get_global_position(s1.center, global_transform_1);
        glm::vec3 center_2 = get_global_position(s2.center, global_transform_2);

        glm::vec3 diff = center_1 - center_2;

        return glm::dot(diff, diff) <= (s1.radius + s2.radius) * (s1.radius + s2.radius);
    }

    bool operator()(const Sphere& s, const Capsule& c) const {
        glm::vec3 C = get_global_position(s.center, global_transform_1);
        glm::vec3 A = get_global_position(c.point_1, global_transform_2);
        glm::vec3 B = get_global_position(c.point_2, global_transform_2);

        return distSqrPointSegment(C, A, B) <= (s.radius + c.radius) * (s.radius + c.radius);
    }

    bool operator()(const Capsule& c, const Sphere& s) const {
        glm::vec3 C = get_global_position(s.center, global_transform_2);
        glm::vec3 A = get_global_position(c.point_1, global_transform_1);
        glm::vec3 B = get_global_position(c.point_2, global_transform_1);

        return distSqrPointSegment(C, A, B) <= (s.radius + c.radius) * (s.radius + c.radius);
    }

    bool operator()(const Capsule& c1, const Capsule& c2) const {
        glm::vec3 P1 = get_global_position(c1.point_1, global_transform_1);
        glm::vec3 Q1 = get_global_position(c1.point_2, global_transform_1);
        glm::vec3 P2 = get_global_position(c2.point_1, global_transform_2);
        glm::vec3 Q2 = get_global_position(c2.point_2, global_transform_2);

        bool c1_is_sphere = glm::dot(P1 - Q1, P1 - Q1) <= kEpsilon;
        bool c2_is_sphere = glm::dot(P2 - Q2, P2 - Q2) <= kEpsilon;
        float dist_sqr = (c1.radius + c2.radius) * (c1.radius + c2.radius);

        if (c1_is_sphere && c2_is_sphere) {
            return glm::dot(P1 - P2, P1 - P2) <= dist_sqr;
        }

        if (c1_is_sphere) {
            return distSqrPointSegment(P1, P2, Q2) <= dist_sqr;
        }

        if (c2_is_sphere) {
            std::cout << distSqrPointSegment(P2, P1, Q1) << std::endl;
            std::cout << P1 << " " << Q1 << " " << P2 << " " << Q2 << std::endl;
            return distSqrPointSegment(P2, P1, Q1) <= dist_sqr;
        }

        return distSqrSegmentSegment(P1, Q1, P2, Q2) <= dist_sqr;
    }

    bool operator()(const Sphere& s, const Triangle& t) const {
        glm::vec3 P = get_global_position(s.center, global_transform_1);
        glm::vec3 A = get_global_position(t.point_1, global_transform_2);
        glm::vec3 B = get_global_position(t.point_2, global_transform_2);
        glm::vec3 C = get_global_position(t.point_3, global_transform_2);

        return distSqrPointTriangle(P, A, B, C) <= s.radius * s.radius;
    }

    bool operator()(const Triangle& t, const Sphere& s) const {
        glm::vec3 P = get_global_position(s.center, global_transform_2);
        glm::vec3 A = get_global_position(t.point_1, global_transform_1);
        glm::vec3 B = get_global_position(t.point_2, global_transform_1);
        glm::vec3 C = get_global_position(t.point_3, global_transform_1);

        return distSqrPointTriangle(P, A, B, C) <= s.radius * s.radius;
    }

    bool operator()(const Capsule& c, const Triangle& t) const {
        AABBVisitor aabb_visitor_1(global_transform_1);
        AABBVisitor aabb_visitor_2(global_transform_2);

        AABB aabb_1 = aabb_visitor_1(c);
        AABB aabb_2 = aabb_visitor_2(t);

        if (!AABB::isIntersectAABB(aabb_1, aabb_2)) {
            return false;
        }

        glm::vec3 P = get_global_position(c.point_1, global_transform_1);
        glm::vec3 Q = get_global_position(c.point_2, global_transform_1);
        glm::vec3 A = get_global_position(t.point_1, global_transform_2);
        glm::vec3 B = get_global_position(t.point_2, global_transform_2);
        glm::vec3 C = get_global_position(t.point_3, global_transform_2);

        return isIntersectCapsuleTriangle(P, Q, A, B, C, c.radius);
    }

    bool operator()(const Triangle& t, const Capsule& c) const {
        AABBVisitor aabb_visitor_1(global_transform_1);
        AABBVisitor aabb_visitor_2(global_transform_2);

        AABB aabb_1 = aabb_visitor_1(t);
        AABB aabb_2 = aabb_visitor_2(c);

        if (!AABB::isIntersectAABB(aabb_1, aabb_2)) {
            return false;
        }

        glm::vec3 P = get_global_position(c.point_1, global_transform_2);
        glm::vec3 Q = get_global_position(c.point_2, global_transform_2);
        glm::vec3 A = get_global_position(t.point_1, global_transform_1);
        glm::vec3 B = get_global_position(t.point_2, global_transform_1);
        glm::vec3 C = get_global_position(t.point_3, global_transform_1);

        return isIntersectCapsuleTriangle(P, Q, A, B, C, c.radius);
    }

    template<DerivedFromShape T, DerivedFromShape U>
    bool operator()(const T& shape1, const U& shape2) const {
        return false;
    }
};

struct CollisionTerrainVisitor {
    const TerrainGenerator* terrain_generator;
    glm::mat4 global_transform;

    CollisionTerrainVisitor(const TerrainGenerator* terrain_generator, const glm::mat4& global_transform) : terrain_generator(terrain_generator), global_transform(global_transform) {}

    glm::vec3 get_global_position(const glm::vec3& local_position, const glm::mat4& global_transform) const {
        return glm::vec3(global_transform * glm::vec4(local_position, 1.0f));
    }

    bool operator()(const Sphere& s) {
        glm::vec3 center = get_global_position(s.center, global_transform);
        float terrain_height = terrain_generator->getHeight(center.x, center.z);
        return center.y - s.radius <= terrain_height;
    }

    bool operator()(const Capsule& c) {
        glm::vec3 P1 = get_global_position(c.point_1, global_transform);
        glm::vec3 P2 = get_global_position(c.point_2, global_transform);

        float length = glm::length(P2 - P1);
        int sample_steps = std::max(1, static_cast<int>(length / c.radius));
        for (int i = 0; i <= sample_steps; ++i) {
            glm::vec3 sample_point = glm::mix(P1, P2, static_cast<float>(i) / sample_steps);

            float terrain_height = terrain_generator->getHeight(sample_point.x, sample_point.z);
            if (sample_point.y - c.radius <= terrain_height) {
                return true;
            }
        }

        return false;
    }

    bool operator()(const Triangle& t) {
        glm::vec3 sample_points[7];
        sample_points[0] = get_global_position(t.point_1, global_transform);
        sample_points[1] = get_global_position(t.point_2, global_transform);
        sample_points[2] = get_global_position(t.point_3, global_transform);
        sample_points[3] = get_global_position((t.point_1 + t.point_2) * 0.5f, global_transform);
        sample_points[4] = get_global_position((t.point_2 + t.point_3) * 0.5f, global_transform);
        sample_points[5] = get_global_position((t.point_1 + t.point_3) * 0.5f, global_transform);
        sample_points[6] = get_global_position((t.point_1 + t.point_2 + t.point_3) * 0.333333f, global_transform);

        for (int i = 0; i < 7; ++i) {
            float terrain_height = terrain_generator->getHeight(sample_points[i].x, sample_points[i].z);
            if (sample_points[i].y <= terrain_height) {
                return true;
            }
        }
        return false;
    }

    template<DerivedFromShape T>
    bool operator()(const T& shape) {
        return false;
    }
};

struct CollisionEvent {
    CollisionObjectType type;
    CollisionLayer layer;
    float time;
    float custom_data;

    CollisionEvent(CollisionObjectType type, CollisionLayer layer, float time, float custom_data = 0.0f): type(type), layer(layer), time(time), custom_data(custom_data) {}
};


class CollisionComponent {
public:
    CollisionComponent(): type_(CollisionConfig::inCollisionableConfig.type_), layer_(CollisionLayer::LAYER_NEUTRAL), shape_(CollisionConfig::inCollisionableConfig.shape_) {}

    CollisionComponent(const CollisionConfig& config, CollisionLayer layer = CollisionLayer::LAYER_NEUTRAL) : type_(config.type_), layer_(layer), shape_(config.shape_) {}

    void submitEvent(const CollisionEvent& event) {
        collision_events_.emplace_back(event);
    }

    void submitEvent(CollisionEvent&& event) {
        collision_events_.emplace_back(std::move(event));
    }

    void clearEvents() {
        collision_events_.clear();
    }

    const std::vector<VoxelShape>& getShapes() const {
        return shape_;
    }

    const std::vector<CollisionEvent>& getCollisionEvents() const {
        return collision_events_;
    }

    CollisionObjectType getType() const {
        return type_;
    }

    CollisionLayer getLayer() const {
        return layer_;
    }

    void setType(CollisionObjectType type) {
        type_ = type;
    }

    void setLayer(CollisionLayer layer) {
        layer_ = layer;
    }

    AABB getAABB(glm::mat4 transform) const {
        AABBVisitor visitor(transform);
        AABB aabb;
        for (const auto& shape : shape_) {
            aabb = AABB::getUnion(aabb, std::visit(visitor, shape));
        }
        return aabb;
    }

private:
    CollisionObjectType type_ { CollisionObjectType::UNKNOWN };
    CollisionLayer layer_ { CollisionLayer::LAYER_NEUTRAL };
    const std::vector<VoxelShape>& shape_;
    std::vector<CollisionEvent> collision_events_;
};


class CollisionableObject : public GameObject {
public:
    CollisionableObject(const CollisionConfig& config): collision_component_(config) {}

    virtual ~CollisionableObject() = default;

    virtual void handleCollision() = 0;

    CollisionComponent& getCollisionComponent() {
        return collision_component_;
    }

protected:
    CollisionComponent collision_component_;
};
