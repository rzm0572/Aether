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

struct PointLight {
    // TODO
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

private:
    const Camera* camera_ {nullptr};
    glm::vec3 ambient_color_;
    ParallelLight parallel_;
    std::vector<PointLight> point_lights_;
};
