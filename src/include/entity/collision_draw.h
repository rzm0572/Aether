#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glad/glad.h>
#include "common/shape.h"

struct DebugLine {
    glm::vec3 start;
    glm::vec3 end;
    glm::vec3 color;
};

struct CollisionDebugVisitor {
    const glm::mat4& global_transform;
    std::vector<DebugLine>& out_lines;
    glm::vec3 color;

    glm::vec3 transformPoint(const glm::vec3& local_pos) const {
        return glm::vec3(global_transform * glm::vec4(local_pos, 1.0f));
    }

    glm::vec3 transformDir(const glm::vec3& local_dir) const {
        return glm::mat3(global_transform) * local_dir;
    }

    void operator()(const Triangle& t) {
        glm::vec3 p1 = transformPoint(t.point_1);
        glm::vec3 p2 = transformPoint(t.point_2);
        glm::vec3 p3 = transformPoint(t.point_3);

        out_lines.push_back({p1, p2, color});
        out_lines.push_back({p2, p3, color});
        out_lines.push_back({p3, p1, color});
    }

    void operator()(const Sphere& s) {
        glm::vec3 center = transformPoint(s.center);
        float scale = glm::length(transformDir(glm::vec3(1.0f, 0.0f, 0.0f))); 
        float r = s.radius * scale;

        drawCircle(center, r, glm::vec3(1, 0, 0), glm::vec3(0, 1, 0));
        drawCircle(center, r, glm::vec3(0, 1, 0), glm::vec3(0, 0, 1));
        drawCircle(center, r, glm::vec3(0, 0, 1), glm::vec3(1, 0, 0));
    }

    // 3. 处理胶囊体 (Capsule)
    void operator()(const Capsule& c) {
        glm::vec3 p1 = transformPoint(c.point_1);
        glm::vec3 p2 = transformPoint(c.point_2);
        
        float scale = glm::length(transformDir(glm::vec3(1.0f, 0.0f, 0.0f)));
        float r = c.radius * scale;

        glm::vec3 axis = p2 - p1;
        float length = glm::length(axis);

        if (length < 1e-5f) {
            Sphere s(c.radius, c.point_1);
            (*this)(s);
            return;
        }

        glm::vec3 axisNorm = axis / length;

        glm::vec3 up = glm::abs(axisNorm.y) < 0.99f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
        glm::vec3 right = glm::normalize(glm::cross(up, axisNorm));
        glm::vec3 front = glm::normalize(glm::cross(axisNorm, right));

        // circles
        drawCircle(p1, r, right, front);
        drawCircle(p2, r, right, front);

        // busbars
        out_lines.push_back({p1 + right * r, p2 + right * r, color});
        out_lines.push_back({p1 - right * r, p2 - right * r, color});
        out_lines.push_back({p1 + front * r, p2 + front * r, color});
        out_lines.push_back({p1 - front * r, p2 - front * r, color});

        // arcs
        drawSemiArc(p1, r, axisNorm, right);
        drawSemiArc(p1, r, axisNorm, front);
        drawSemiArc(p2, r, -axisNorm, right);
        drawSemiArc(p2, r, -axisNorm, front);
    }

private:
    void drawCircle(glm::vec3 center, float radius, glm::vec3 axisX, glm::vec3 axisY) {
        const int segments = 16;
        float step = glm::two_pi<float>() / segments;
        
        for (int i = 0; i < segments; ++i) {
            float angle1 = i * step;
            float angle2 = (i + 1) * step;

            glm::vec3 p1 = center + (axisX * glm::cos(angle1) + axisY * glm::sin(angle1)) * radius;
            glm::vec3 p2 = center + (axisX * glm::cos(angle2) + axisY * glm::sin(angle2)) * radius;

            out_lines.push_back({p1, p2, color});
        }
    }

    void drawSemiArc(glm::vec3 center, float radius, glm::vec3 axisUp, glm::vec3 axisRight) {
        const int segments = 8;
        float step = glm::half_pi<float>() * 2.0f / segments; // 180度
        float base_angle = -glm::half_pi<float>();

        for(int i = 0; i < segments; ++i) {
            float angle1 = i * step + base_angle;
            float angle2 = (i+1) * step + base_angle;
            
            glm::vec3 p1 = center - axisUp * radius * glm::cos(angle1) + axisRight * radius * glm::sin(angle1);
            glm::vec3 p2 = center - axisUp * radius * glm::cos(angle2) + axisRight * radius * glm::sin(angle2);
            
            out_lines.push_back({p1, p2, color});
        }
    }
};

void renderCollisionBox(const std::vector<DebugLine>& lines, const glm::mat4& view, const glm::mat4& projection);
