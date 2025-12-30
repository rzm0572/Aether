#pragma once

#include <glm/glm.hpp>
#include "utils/profiler.h"

struct Shape {};

struct Sphere : public Shape {
    float radius { 0.0f };
    glm::vec3 center { 0.0f };

    Sphere() = default;
    Sphere(float radius, const glm::vec3& center) : radius(radius), center(center) {}

    std::string toString() const {
        std::stringstream ss;
        ss << "Sphere(radius=" << radius << ", center=" << center << ")";
        return ss.str();
    }
};

struct Capsule : public Shape {
    float radius { 0.0f };
    glm::vec3 point_1 { 0.0f };
    glm::vec3 point_2 { 0.0f };

    Capsule() = default;
    Capsule(float radius, const glm::vec3& point_1, const glm::vec3& point_2) : radius(radius), point_1(point_1), point_2(point_2) {}

    std::string toString() const {
        std::stringstream ss;
        ss << "Capsule(radius=" << radius << ", point_1=" << point_1 << ", point_2=" << point_2 << ")";
        return ss.str();
    }
};

struct Triangle : public Shape {
    glm::vec3 point_1 { 0.0f };
    glm::vec3 point_2 { 0.0f };
    glm::vec3 point_3 { 0.0f };

    Triangle() = default;
    Triangle(const glm::vec3& point_1, const glm::vec3& point_2, const glm::vec3& point_3) : point_1(point_1), point_2(point_2), point_3(point_3) {}

    std::string toString() const {
        std::stringstream ss;
        ss << "Triangle(point_1=" << point_1 << ", point_2=" << point_2 << ", point_3=" << point_3 << ")";
        return ss.str();
    }
};

struct AABB {
    glm::vec3 min_point { 0.0f };
    glm::vec3 max_point { 0.0f };

    AABB() = default;
    AABB(const glm::vec3& min_point, const glm::vec3& max_point) : min_point(min_point), max_point(max_point) {}
    AABB(float x1, float y1, float z1, float x2, float y2, float z2) : min_point(x1, y1, z1), max_point(x2, y2, z2) {}

    static AABB getUnion(const AABB& a, const AABB& b) {
        return AABB(glm::min(a.min_point, b.min_point), glm::max(a.max_point, b.max_point));
    }

    static bool isIntersectAABB(const AABB& a, const AABB& b) {
        bool x_intersect = !(a.max_point.x < b.min_point.x || a.min_point.x > b.max_point.x);
        bool y_intersect = !(a.max_point.y < b.min_point.y || a.min_point.y > b.max_point.y);
        bool z_intersect = !(a.max_point.z < b.min_point.z || a.min_point.z > b.max_point.z);

        return x_intersect && y_intersect && z_intersect;
    }

    std::string toString() const {
        std::stringstream ss;
        ss << "AABB(min_point=" << min_point << ", max_point=" << max_point << ")";
        return ss.str();
    }
};

using VoxelShape = std::variant<Sphere, Capsule, Triangle>;

struct AABBVisitor {
    const glm::mat4 global_transform;

    glm::vec3 get_global_position(const glm::vec3& local_position, const glm::mat4& global_transform) const {
        return glm::vec3(global_transform * glm::vec4(local_position, 1.0f));
    }

    AABB operator()(const Sphere& s) const {
        glm::vec3 center = get_global_position(s.center, global_transform);
        return AABB(center - glm::vec3(s.radius), center + glm::vec3(s.radius));
    }

    AABB operator()(const Capsule& c) const {
        glm::vec3 P = get_global_position(c.point_1, global_transform);
        glm::vec3 Q = get_global_position(c.point_2, global_transform);
        
        glm::vec3 min_coord = glm::min(P, Q);
        glm::vec3 max_coord = glm::max(P, Q);

        return AABB(min_coord - glm::vec3(c.radius), max_coord + glm::vec3(c.radius));
    }

    AABB operator()(const Triangle& t) const {
        glm::vec3 A = get_global_position(t.point_1, global_transform);
        glm::vec3 B = get_global_position(t.point_2, global_transform);
        glm::vec3 C = get_global_position(t.point_3, global_transform);

        glm::vec3 min_coord = glm::min(A, glm::min(B, C));
        glm::vec3 max_coord = glm::max(A, glm::max(B, C));

        return AABB(min_coord, max_coord);
    }
};

struct ShapeToStringVisitor {
    std::string operator()(const Sphere& s) {
        return s.toString();
    }

    std::string operator()(const Capsule& c) {
        return c.toString();
    }

    std::string operator()(const Triangle& t) {
        return t.toString();
    }
};
