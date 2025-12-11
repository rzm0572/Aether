#pragma once

#include <glm/glm.hpp>
#include <resource/shader.h>
#include <interaction/camera.h>

struct ParallelLight {
    glm::vec3 light_dir;
    glm::vec3 light_color;

    void use(const Shader* shader) const {
        shader->setUniform("lightDir", light_dir);
        shader->setUniform("lightColor", light_color);
    }
};


class Light {
public:
    Light() = default;
    Light(const Camera* camera, const glm::vec3& ambient_color, const ParallelLight& parallel)
        : camera_(camera), ambient_color_(ambient_color), parallel_(parallel) {}

    void use(const Shader* shader) const {
        shader->setUniform("camPos", camera_->getPosition());
        shader->setUniform("ambientLight", ambient_color_);
        parallel_.use(shader);
    }
        // 实时计算 Light Space Matrix（用于 Shadow Mapping）
    glm::mat4 getLightSpaceMatrix() const {
        if (!camera_) {
            return glm::mat4(1.0f);
        }

        // 1. 获取相机参数
        float fov = glm::radians(60.0f);
        float aspect = (float)800 / (float)600;
        float near_plane = 0.1f;
        float far_plane = 100.0f;

        // 2. 计算视锥体8个角点（世界空间）
        float nh = near_plane * tan(fov / 2.0f); // 近平面高度的一半
        float nw = nh * aspect;                  // 近平面宽度的一半
        float fh = far_plane * tan(fov / 2.0f);  // 远平面高度的一半
        float fw = fh * aspect;                  // 远平面宽度的一半

        glm::vec3 camPos = camera_->getPosition();
        glm::vec3 camFront = glm::normalize(camera_->getFrontVec());
        glm::vec3 camRight = glm::normalize(camera_->getRightVec());
        glm::vec3 camUp = glm::normalize(camera_->getUpVec());

        // 近平面四个点
        glm::vec3 nearCenter = camPos + camFront * near_plane;
        glm::vec3 ntl = nearCenter + camUp * nh - camRight * nw;
        glm::vec3 ntr = nearCenter + camUp * nh + camRight * nw;
        glm::vec3 nbl = nearCenter - camUp * nh - camRight * nw;
        glm::vec3 nbr = nearCenter - camUp * nh + camRight * nw;

        // 远平面四个点
        glm::vec3 farCenter = camPos + camFront * far_plane;
        glm::vec3 ftl = farCenter + camUp * fh - camRight * fw;
        glm::vec3 ftr = farCenter + camUp * fh + camRight * fw;
        glm::vec3 fbl = farCenter - camUp * fh - camRight * fw;
        glm::vec3 fbr = farCenter - camUp * fh + camRight * fw;

        std::array<glm::vec3, 8> frustumCorners = {
            ntl, ntr, nbl, nbr,
            ftl, ftr, fbl, fbr
        };

        // 3. 光源方向（注意：平行光方向指向光源，但 lookAt 需要“从哪看”）
        glm::vec3 lightDir = glm::normalize(parallel_.light_dir);
        glm::vec3 up = glm::abs(lightDir.y) > 0.99f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);

        // 光源位置：可以设为任意点，只要 view 矩阵正确即可（这里用原点后退）
        // 实际上我们只关心方向，所以 view 矩阵由 lookAt(任意点沿 -lightDir, 任意点, up) 决定
        glm::vec3 lightPos = camPos - lightDir * (far_plane + near_plane); // 足够远
        glm::mat4 lightView = glm::lookAt(lightPos, camPos, up);

        // 4. 将视锥体角点变换到光源空间（即 lightView * point）
        glm::vec4 minBound(FLT_MAX);
        glm::vec4 maxBound(-FLT_MAX);

        for (const auto& corner : frustumCorners) {
            glm::vec4 lightSpaceCorner = lightView * glm::vec4(corner, 1.0f);
            minBound = glm::min(minBound, lightSpaceCorner);
            maxBound = glm::max(maxBound, lightSpaceCorner);
        }

        // 可选：扩大一点边界防止走样（例如加 1~2 单位 padding）
        float padding = 2.0f;
        minBound.x -= padding; minBound.y -= padding; minBound.z -= padding;
        maxBound.x += padding; maxBound.y += padding; maxBound.z += padding;

        // 5. 构建正交投影矩阵（注意 Z 范围：OpenGL 是 -1 到 1，但深度通常用 near/far）
        // 注意：Z 轴方向需与 lightView 一致（通常取 [min.z, max.z]）
        glm::mat4 lightProjection = glm::ortho(
            minBound.x, maxBound.x,
            minBound.y, maxBound.y,
            minBound.z, maxBound.z
        );

        return lightProjection * lightView;
    }

private:
    const Camera* camera_ {nullptr};
    glm::vec3 ambient_color_;
    ParallelLight parallel_;
};
