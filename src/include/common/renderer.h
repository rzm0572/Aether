#pragma once

#include "common/game_object.h"
#include "component/render.h"
#include "world/light.h"
#include "utils/profiler.h"
#include <glm/glm.hpp>
#include "resource/shadow_map.h"
#include "utils/config.h"
/**
 * @brief Renderer class
 * 
 * Handles the rendering of GameObjects with optimization through render queue sorting.
 * Minimizes state changes by batching draw calls with the same VAO and shader.
 */
class Renderer {
public:
    /**
     * @brief Render all submitted GameObjects
     * 
     * Processes the render queue in sorted order to minimize state changes.
     * Applies view, projection, and lighting transformations, then draws each
     * renderable object with its material and mesh data.
     * 
     * @param view The view matrix (camera transform)
     * @param projection The projection matrix (perspective/orthographic)
     * @param light The light to apply to the scene
     */
    void renderShadow(const std::vector<GameObject*>& objects,const Light& light) {
        if (!depth_shader_) {
            auto shader_manager = ServiceLocator<ShaderManager>::get();
            depth_shader_ = shader_manager->getShader("depth");
        }

        glViewport(0, 0, ShadowMap::SHADOW_WIDTH, ShadowMap::SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, shadow_map_.getFBO());
        glClear(GL_DEPTH_BUFFER_BIT);

        depth_shader_->useShader();
        depth_shader_->setUniform("lightSpaceMatrix", light.getLightSpaceMatrix());

        for (auto* obj : objects) {
            submit_for_shadow(obj, depth_shader_);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // 恢复默认视口
        Config* config = ServiceLocator<Config>::get();
        glViewport(0, 0, config->scr_width, config->scr_height);
    }

    void submit_for_shadow(GameObject* obj,const Shader* shader) {
        const auto& rc = obj->getRenderComponent();
        if (!rc.renderable_) return;

        glBindVertexArray(rc.VAO_);
        glm::mat4 model = obj->getTransformComponent().getGlobalModelMatrix();
        shader->setUniform("model", model);
        glDrawElements(GL_TRIANGLES, rc.mesh_->getNumIndices(), GL_UNSIGNED_INT,
                    (void*)(rc.mesh_->getIndexOffset() * sizeof(unsigned int)));
        glBindVertexArray(0);
    }
    void render(glm::mat4 view, glm::mat4 projection, const Light& light) {
        std::sort(render_queue_.begin(), render_queue_.end());

        current_VAO_ = INVALID_VAO;
        current_shader_ID_ = 0;

        // Traverse the render queue and render each renderable object
        for (const auto& entry : render_queue_) {
            if (!entry.rc->renderable_) {
                continue;
            }

            // If the VAO changed, bind the new one
            if (current_VAO_ != entry.rc->VAO_) {
                // std::cout << "VAO changed from " << current_VAO_ << " to " << entry.rc->VAO_ << std::endl;
                glBindVertexArray(entry.rc->VAO_);
                current_VAO_ = entry.rc->VAO_;
            }

            // If the shader changed, bind the new one
            const auto* shader = entry.rc->material_->getShader();
            if (current_shader_ID_ != shader->getShaderID()) {
                shader->useShader();
                current_shader_ID_ = shader->getShaderID();

                shader->setUniform("view", view);
                shader->setUniform("projection", projection);
                shader->setUniform("lightSpaceMatrix", light.getLightSpaceMatrix());
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, shadow_map_.getDepthMap());
                shader->setUniform("shadowMap", 1);
                light.use(shader);
            }

            // Use the material and update OpenGL state
            entry.rc->material_->apply();

            // Update the model matrix by the global transform of the object
            shader->setUniform("model", entry.global_transform);
            shader->setUniform("ourTexture", 0);            
            shader->setUniform("lightSpaceMatrix", light.getLightSpaceMatrix());

            // Render the object
            void* offset = (void*)(entry.rc->mesh_->getIndexOffset() * sizeof(unsigned int));
            glDrawElements(GL_TRIANGLES, entry.rc->mesh_->getNumIndices(), GL_UNSIGNED_INT, offset);
        }

        // Unbind VAO
        glBindVertexArray(INVALID_VAO);
        current_VAO_ = INVALID_VAO;

        // Clear the render queue for the next frame
        render_queue_.clear();
    }

    /**
     * @brief Submit a GameObject for rendering
     * 
     * Adds the GameObject to the render queue if it has a renderable component.
     * The object will be rendered in the next render() call.
     * Generates a sorting key based on the shader and VAO of the material like:
     * *---------------------*---------------*
     * | shader ID (16 bits) | VAO (16 bits) |
     * *---------------------*---------------*
     * 
     * @param obj The GameObject to submit for rendering
     */
    void submit(GameObject* obj) {
        // debug_output(obj);
        const auto* rc = &obj->getRenderComponent();
        if (!rc->renderable_) {
            return;
        }

        unsigned int key = (rc->material_->getShader()->getShaderID() << 16) | rc->VAO_;
        glm::mat4 global_transform = obj->getTransformComponent().getGlobalModelMatrix();
        render_queue_.push_back({ key, rc, global_transform });
    }

    /**
     * @brief Submit a GameObject and all its children for rendering
     * 
     * Recursively traverses the GameObject hierarchy and submits each object
     * for rendering.
     * 
     * @param obj The root GameObject to submit (along with its children)
     */
    void submit_recursive(GameObject* obj) {
        submit(obj);
        for (auto* child : obj->getChildren()) {
            submit_recursive(child);
        }
    }

private:
    /**
     * @brief Output debug information for a GameObject
     * 
     * Prints mesh, material, position, and transformation matrix information
     * to the console for debugging purposes.
     * 
     * @param obj The GameObject to debug
     */
    void debug_output(GameObject* obj) {
        const auto* rc = &obj->getRenderComponent();
        std::cout << "Submitting " << obj->GetUUID() << std::endl;

        if (rc->renderable_) {
            std::cout << "Mesh: " << rc->mesh_->toString() << std::endl;
            std::cout << "Material: " << rc->material_->toString() << std::endl;
        }

        auto transform = obj->getTransformComponent();
        auto position = transform.getPosition();
        std::cout << "Position: " << position << std::endl;
        std::cout << "Local transform: " << transform.getLocalModelMatrix() << std::endl;
        std::cout << "Global transform: " << transform.getGlobalModelMatrix() << std::endl;
    }

    /**
     * @brief Render queue entry
     * 
     * Represents a single object in the render queue with a sorting key,
     * render component reference, and global transformation matrix.
     */
    struct RenderQueueEntry {
        unsigned int key;
        const RenderComponent* rc;
        glm::mat4 global_transform;

        bool operator<(const RenderQueueEntry& other) const {
            return key < other.key;
        }
    };

    std::vector<RenderQueueEntry> render_queue_;

    // Copy of OpenGL states
    GLuint current_VAO_ {INVALID_VAO};
    unsigned int current_shader_ID_ {0};

    // Shadow map 需要的着色器和深度纹理
    const Shader* depth_shader_ = nullptr;
    ShadowMap shadow_map_;
};
