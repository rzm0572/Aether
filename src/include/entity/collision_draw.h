#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glad/glad.h>
#include "common/shape.h"

// 简单的线条结构，用于提交给你的渲染器
struct DebugLine {
    glm::vec3 start;
    glm::vec3 end;
    glm::vec3 color;
};

// 调试绘制访问器
struct CollisionDebugVisitor {
    const glm::mat4& global_transform; // 这一帧物体的模型矩阵
    std::vector<DebugLine>& out_lines; // 输出线条的容器
    glm::vec3 color;                   // 线条颜色

    // 辅助：获取变换后的世界坐标
    glm::vec3 transformPoint(const glm::vec3& local_pos) const {
        return glm::vec3(global_transform * glm::vec4(local_pos, 1.0f));
    }

    // 辅助：获取变换后的方向（忽略位移）
    glm::vec3 transformDir(const glm::vec3& local_dir) const {
        return glm::mat3(global_transform) * local_dir;
    }

    // 1. 处理三角形 (Triangle)
    void operator()(const Triangle& t) {
        glm::vec3 p1 = transformPoint(t.point_1);
        glm::vec3 p2 = transformPoint(t.point_2);
        glm::vec3 p3 = transformPoint(t.point_3);

        out_lines.push_back({p1, p2, color});
        out_lines.push_back({p2, p3, color});
        out_lines.push_back({p3, p1, color});
    }

    // 2. 处理球体 (Sphere)
    // 画三个正交的圆环来表示球体
    void operator()(const Sphere& s) {
        glm::vec3 center = transformPoint(s.center);
        // 如果物体有缩放，需要考虑缩放对半径的影响 (取最大缩放分量)
        float scale = glm::length(transformDir(glm::vec3(1.0f, 0.0f, 0.0f))); 
        float r = s.radius * scale;

        drawCircle(center, r, glm::vec3(1, 0, 0), glm::vec3(0, 1, 0)); // XY 平面
        drawCircle(center, r, glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)); // YZ 平面
        drawCircle(center, r, glm::vec3(0, 0, 1), glm::vec3(1, 0, 0)); // XZ 平面
    }

    // 3. 处理胶囊体 (Capsule)
    void operator()(const Capsule& c) {
        glm::vec3 p1 = transformPoint(c.point_1);
        glm::vec3 p2 = transformPoint(c.point_2);
        
        // 计算半径缩放
        float scale = glm::length(transformDir(glm::vec3(1.0f, 0.0f, 0.0f)));
        float r = c.radius * scale;

        // 胶囊体轴向量
        glm::vec3 axis = p2 - p1;
        float length = glm::length(axis);

        // 如果两点重合，退化为球体
        if (length < 1e-5f) {
            Sphere s(c.radius, c.point_1);
            (*this)(s);
            return;
        }

        glm::vec3 axisNorm = axis / length;

        // 构建局部坐标系 (Orthonormal Basis)
        glm::vec3 up = glm::abs(axisNorm.y) < 0.99f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
        glm::vec3 right = glm::normalize(glm::cross(up, axisNorm));
        glm::vec3 front = glm::normalize(glm::cross(axisNorm, right));

        // 绘制两端的半球（这里简化为画两个全圆环 + 端点）
        drawCircle(p1, r, right, front);
        drawCircle(p2, r, right, front);

        // 绘制连接两端的 4 条线
        out_lines.push_back({p1 + right * r, p2 + right * r, color});
        out_lines.push_back({p1 - right * r, p2 - right * r, color});
        out_lines.push_back({p1 + front * r, p2 + front * r, color});
        out_lines.push_back({p1 - front * r, p2 - front * r, color});

        // 可选：画出中心轴线
        // out_lines.push_back({p1, p2, color}); 
        
        // 为了视觉完整，在端点处画简单的十字交叉模拟球体感
        drawSemiArc(p1, r, axisNorm, right); // 顶部圆弧
        drawSemiArc(p1, r, axisNorm, front);
        drawSemiArc(p2, r, -axisNorm, right); // 底部圆弧
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
            // 从 -90 度到 90 度，或者是 0 到 180，取决于你想画哪个半球
            // 这里简单画一个远离轴心的半圆
            float angle1 = i * step + base_angle;
            float angle2 = (i+1) * step + base_angle;
            
            // 这里的数学逻辑是为了画出从端点向外延伸的弧
            glm::vec3 p1 = center - axisUp * radius * glm::cos(angle1) + axisRight * radius * glm::sin(angle1);
            glm::vec3 p2 = center - axisUp * radius * glm::cos(angle2) + axisRight * radius * glm::sin(angle2);
            
            // 实际上调试用的 Capsule 画两端的全圆和侧面的线通常就足够看清了，
            // 上面的 drawCircle 已经提供了主要的轮廓。
            out_lines.push_back({p1, p2, color});
        }
    }
};

void renderCollisionBox(const std::vector<DebugLine>& lines, const glm::mat4& view, const glm::mat4& projection);
