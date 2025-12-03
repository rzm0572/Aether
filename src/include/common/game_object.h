#pragma once

#include "component/render.h"
#include "component/transform.h"
#include "resource/model.h"
#include "service/service_locator.h"

using UUID_t = unsigned long long;

/**
 * @brief GameObject class
 * 
 * Represents an entity in the game world with transform and render components.
 * Supports hierarchical relationships through parent-child transforms and
 * provides factory methods for creating GameObjects from model data.
 */
class GameObject {
public:
    /**
     * @brief Construct a new GameObject with self-incrementing UUID
     * 
     * @param transform The transform component (defaults to identity transform)
     * @param render The render component (defaults to non-renderable)
     */
    GameObject(TransformComponent transform = TransformComponent(), RenderComponent render = RenderComponent()): uuid_(next_uuid_++), transform_(transform), render_(render) {}
    
    /**
     * @brief Construct a new GameObject with a specific UUID
     * 
     * @param uuid The unique identifier for this GameObject
     * @param transform The transform component (defaults to identity transform)
     * @param render The render component (defaults to non-renderable)
     */
    GameObject(UUID_t uuid, TransformComponent transform = TransformComponent(), RenderComponent render = RenderComponent()): uuid_(uuid), transform_(transform), render_(render) {}

    UUID_t GetUUID() const { return uuid_; }

    TransformComponent& getTransformComponent() { return transform_; }

    RenderComponent& getRenderComponent() { return render_; }

    /**
     * @brief Create a GameObject hierarchy from a model node tree
     * 
     * Recursively creates GameObjects for each node in the model tree.
     * If a node has a single mesh, it becomes renderable. If it has multiple
     * meshes, child GameObjects are created for each mesh.
     * 
     * @param node The model node to create the GameObject from
     * @param model The model containing mesh and material data
     * @param parent The parent GameObject (or nullptr for root objects)
     * @return GameObject* Pointer to the created GameObject
     */
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

    /**
     * @brief Create a GameObject from a single mesh
     * 
     * Creates a renderable GameObject with the specified mesh and its material.
     * 
     * @param meshIdx The index of the mesh in the model's mesh array
     * @param model The model containing mesh and material data
     * @param parent The parent GameObject (or nullptr for root objects)
     * @return GameObject* Pointer to the created GameObject
     */
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

    /**
     * @brief Create a GameObject hierarchy from a complete model
     * 
     * Creates a wrapper GameObject as the root, then builds the entire
     * GameObject hierarchy from the model's node tree.
     * 
     * @tparam GameObjectDerived The type of the root wrapper GameObject (default: GameObject)
     * @param model The model to create GameObjects from
     * @return GameObjectDerived* Pointer to the root wrapper GameObject
     */
    template<typename GameObjectDerived = GameObject>
    static GameObjectDerived* createFromModel(const Model& model) {
        // Check that GameObjectDerived is derived from GameObject
        if constexpr (!std::is_base_of_v<GameObject, GameObjectDerived>) {
            static_assert(false, "GameObjectDerived must be derived from GameObject");
        }

        GameObjectDerived* wrapper = new GameObjectDerived();
        wrapper->transform_.setParent(nullptr, wrapper);
        wrapper->render_.renderable_ = false;
        createFromModel(wrapper, model);
        return wrapper;
    }

    template<typename GameObjectDerived = GameObject>
    static void createFromModel(GameObjectDerived* wrapper, const Model& model) {
        // Check that GameObjectDerived is derived from GameObject
        if constexpr (!std::is_base_of_v<GameObject, GameObjectDerived>) {
            static_assert(false, "GameObjectDerived must be derived from GameObject");
        }

        auto* go = createFromModelTree(model.root_node_, model, nullptr);
        go->getTransformComponent().setParent(wrapper, go);
    }

    const std::vector<GameObject*>& getChildren() const {
        return transform_.getChildren();
    }

private:
    UUID_t uuid_;
    TransformComponent transform_;
    RenderComponent render_;

    inline static UUID_t next_uuid_ = 0;
};
