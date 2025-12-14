#pragma once

#include "utils/config.h"
#include "service/service_locator.h"
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

        // 获取相机参数
        auto config = ServiceLocator<Config>::get();
        float fov = glm::radians(45.0f);
        float aspect = (float)config->scr_width / (float)config->scr_height;
        float near_plane = config->z_near;
        float far_plane = config->z_far;

        // 计算视锥体8个角点（世界空间） 
        // TODO: 不行，这里需要考虑摄像机外面的物体
        float nh = near_plane * tan(fov / 2.0f); // 近平面高度的一半
        float nw = nh * aspect;                  // 近平面宽度的一半
        float fh = far_plane * tan(fov / 2.0f);  // 远平面高度的一半
        float fw = fh * aspect;                  // 远平面宽度的一半

        glm::vec3 camPos = camera_->getPosition();
        // std::cout<<"camPos: "<<camPos.x<<" "<<camPos.y<<" "<<camPos.z<<std::endl;
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


        glm::vec3 lightDir = -glm::normalize(parallel_.light_dir); // 光的传播方向
        glm::vec3 up = glm::abs(lightDir.y) > 0.99f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);

        // 光源位置：可以设为任意点，只要 view 矩阵正确即可（这里用原点后退）
        // 实际上我们只关心方向，所以 view 矩阵由 lookAt(任意点沿 -lightDir, 任意点, up) 决定
        glm::vec3 lightPos = camPos - lightDir * 3.0f*(far_plane + near_plane); // 足够远
        glm::mat4 lightView = glm::lookAt(lightPos,  camPos,up); // 这个矩阵将世界坐标转换为以光源为原点观察、光线传播方向为 -z 的坐标系

        // 4. 将视锥体角点变换到光源空间（即 lightView * point）
        glm::vec4 minBound(FLT_MAX);
        glm::vec4 maxBound(-FLT_MAX);

        for (const auto& corner : frustumCorners) {
            glm::vec4 lightSpaceCorner = lightView * glm::vec4(corner, 1.0f);
            minBound = glm::min(minBound, lightSpaceCorner);
            maxBound = glm::max(maxBound, lightSpaceCorner);
        }

        // 扩大一点边界防止走样
        float padding = 2.0f;
        minBound.x -= padding; minBound.y -= padding; minBound.z -= padding;
        maxBound.x += padding; maxBound.y += padding; maxBound.z += padding;

        glm::mat4 lightProjection = glm::ortho(
            minBound.x, maxBound.x,
            minBound.y, maxBound.y,
            -maxBound.z  ,-minBound.z//因为ortho需要两个正数的z，所以这里取负值
        );
        // // 输出包围盒调试
        // std::cout << "minBound: " << minBound.x << " " << minBound.y << " " << minBound.z << std::endl;
        // std::cout << "maxBound: " << maxBound.x << " " << maxBound.y << " " << maxBound.z << std::endl;
        // // 输出光线方向调试
        // std::cout << "lightDir: " << lightDir.x << " " << lightDir.y << " " << lightDir.z << std::endl;
        // // 输出飞机位置的light space坐标
        // glm::vec4 lightSpacePos = lightView * glm::vec4(camPos, 1.0f);
        // std::cout << "lightSpacePos: " << lightSpacePos.x << " " << lightSpacePos.y << " " << lightSpacePos.z << std::endl;
        // lightSpacePos = lightProjection * lightSpacePos;
        // std::cout << "lightSpacePos: " << lightSpacePos.x / lightSpacePos.w << " " << lightSpacePos.y / lightSpacePos.w << " " << lightSpacePos.z / lightSpacePos.w << std::endl;
        return lightProjection * lightView;
    }

private:
    const Camera* camera_ {nullptr};
    glm::vec3 ambient_color_;
    ParallelLight parallel_;
};
