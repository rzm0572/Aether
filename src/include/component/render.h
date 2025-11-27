#pragma once

#include "resource/mesh.h"
#include "resource/material.h"
#include <glad/glad.h>
#include <memory>

class RenderComponent {
public:
    RenderComponent() : renderable_(false), mesh_(nullptr), material_(nullptr) {};
    RenderComponent(GLuint VAO, const Mesh* mesh, const std::shared_ptr<Material>& material) : 
        renderable_(true), VAO_(VAO), mesh_(mesh), material_(material) {}

public:
    bool renderable_;
    GLuint VAO_;
    const Mesh* mesh_;
    std::shared_ptr<Material> material_;
};
