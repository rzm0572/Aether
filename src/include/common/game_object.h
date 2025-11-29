#pragma once

#include "component/render.h"
#include "component/transform.h"
#include "resource/model.h"
#include "service/service_locator.h"

using UUID = unsigned long long;

class GameObject {
public:
    GameObject(TransformComponent transform = TransformComponent(), RenderComponent render = RenderComponent()): uuid_(next_uuid_++), transform_(transform), render_(render) {}
    GameObject(UUID uuid, TransformComponent transform = TransformComponent(), RenderComponent render = RenderComponent()): uuid_(uuid), transform_(transform), render_(render) {}

    UUID GetUUID() const { return uuid_; }
    TransformComponent& getTransformComponent() { return transform_; }

    RenderComponent& getRenderComponent() { return render_; }

    static GameObject* createFromModelTree(const ModelNode& node, const Model& model, GameObject* parent = nullptr) {
        GameObject* go = new GameObject();

        auto& transform = go->transform_;
        transform.setlocalModelMatrix(node.local_transform);
        transform.setParent(parent, go);

        auto& render = go->render_;
        if (!node.meshes.empty()) {
            if (node.meshes.size() == 1) {
                render.renderable_ = true;
                render.VAO_ = model.VAO_;
                render.mesh_ = &model.meshes_[node.meshes[0]];
                render.material_ = model.default_materials_[render.mesh_->getMaterialIndex()];
            } else {
                render.renderable_ = false;
                for (auto meshIdx : node.meshes) {
                    createFromMesh(meshIdx, model, go);
                }
            }
        } else {
            render.renderable_ = false;
        }

        for (auto& child : node.children) {
            createFromModelTree(child, model, go);
        }

        return go;
    }

    static GameObject* createFromMesh(int meshIdx, const Model& model, GameObject* parent = nullptr) {
        GameObject* go = new GameObject();
        go->transform_.setParent(parent, go);

        auto& render = go->render_;
        render.renderable_ = true;
        render.VAO_ = model.VAO_;
        render.mesh_ = &model.meshes_[meshIdx];
        render.material_ = model.default_materials_[render.mesh_->getMaterialIndex()];

        return go;
    }

    static GameObject* createFromModel(const Model& model) {
        GameObject* wrapper = new GameObject();
        wrapper->transform_.setParent(nullptr, wrapper);
        wrapper->render_.renderable_ = false;

        auto* go = createFromModelTree(model.root_node_, model, nullptr);
        go->getTransformComponent().setParent(wrapper, go);

        return wrapper;
    }

    const std::vector<GameObject*>& getChildren() const {
        return transform_.getChildren();
    }

private:
    UUID uuid_;
    TransformComponent transform_;
    RenderComponent render_;

    inline static UUID next_uuid_ = 0;
};
