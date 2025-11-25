#pragma once

#include "component/render.h"
#include "component/transform.h"

using UUID = unsigned long long;

class GameObject {
public:
    GameObject(UUID uuid = 0): uuid_(uuid) {}
    GameObject(UUID uuid, TransformComponent transform): uuid_(uuid), transform_(transform) {}

    UUID GetUUID() const { return uuid_; }
    TransformComponent& getTransformComponent() { return transform_; }

    RenderComponent& getRenderComponent() { return render_; }

private:
    UUID uuid_;
    TransformComponent transform_;
    RenderComponent render_;
};
