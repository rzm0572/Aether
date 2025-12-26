#pragma once

#include "resource/mesh.h"
#include "resource/material.h"
#include "resource/explosion.h"
#include <glad/glad.h>
#include <memory>

enum class RenderType {
    MESH,
    EXPLODED_MODEL,
};

class RenderComponent {
public:
    RenderComponent() : renderable_(false), mesh_(nullptr), material_(nullptr) {};
    RenderComponent(GLuint VAO, const Mesh* mesh, const std::shared_ptr<Material>& material) : 
        renderable_(true), type_(RenderType::MESH), VAO_(VAO), mesh_(mesh), material_(material) {}
    

public:
    bool renderable_;
    RenderType type_;
    GLuint VAO_;
    union {
        const Mesh* mesh_;
        const ExplodedModel* exploded_model_;
    };
    std::shared_ptr<Material> material_;
};
