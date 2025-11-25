#pragma once

#include "resource/mesh.h"
#include "resource/material.h"
#include <memory>

class RenderComponent {
public:
    RenderComponent() : renderable_(false), mesh_(nullptr), material_(nullptr) {};
    RenderComponent(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Material>& material) : 
        renderable_(true), mesh_(mesh), material_(material) {}

public:
    bool renderable_;
    std::shared_ptr<Mesh> mesh_;
    std::shared_ptr<Material> material_;
};
