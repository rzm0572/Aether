#pragma once

#include "common/game_object.h"
#include "common/window.h"
#include "component/render.h"
#include "world/light.h"
#include "utils/profiler.h"
#include <glm/glm.hpp>
#include "resource/shadow_map.h"
/**
 * @brief Renderer class
 * 
 * Handles the rendering of GameObjects with optimization through render queue sorting.
 * Minimizes state changes by batching draw calls with the same VAO and shader.
 */
class Renderer {
public:
    void finishAllSubmissions() {
        std::sort(render_queue_.begin(), render_queue_.end());
    }

    void finishAllRender() {
        render_queue_.clear();
    }

    void renderShadowMap(const Light& light, Window& window) {
        beginShadowPass(light);
        if (depth_shader_ == nullptr) {
            std::cerr << "Depth shader not found!" << std::endl;
            return;
        }

        current_VAO_ = INVALID_VAO;
        current_shader_ID_ = 0;

        depth_shader_->useShader();
        depth_shader_->setUniform("lightSpaceMatrix", light.getLightSpaceMatrix());
        for (const auto& entry : render_queue_) {
            if (!entry.rc->renderable_) {
                continue;
            }

            if (entry.rc->type_ != RenderType::MESH) {
                continue;
            }

            if (current_VAO_ != entry.rc->VAO_) {
                glBindVertexArray(entry.rc->VAO_);
                current_VAO_ = entry.rc->VAO_;
            }

            glm::mat4 model = entry.global_transform;
            const Mesh* mesh = entry.rc->mesh_;
            depth_shader_->setUniform("model", model);
            glDrawElements(GL_TRIANGLES, mesh->getNumIndices(), GL_UNSIGNED_INT,
                        (void*)(mesh->getIndexOffset() * sizeof(unsigned int)));
        }

        glBindVertexArray(INVALID_VAO);
        current_VAO_ = INVALID_VAO;

        endShadowPass(window);
    }

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
    void render(glm::mat4 view, glm::mat4 projection, const Light& light) {
        // std::sort(render_queue_.begin(), render_queue_.end());

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

                if (depth_shader_ != nullptr) {
                    // Bind shadow map texture
                    glActiveTexture(GL_TEXTURE15);
                    glBindTexture(GL_TEXTURE_2D, shadow_map_.getDepthMap());
                    shader->setUniform("shadowMap", 15);
                    shader->setUniform("lightSpaceMatrix", light_view_matrix_);
                }

                light.use(shader);
            }

            if (entry.rc->type_ == RenderType::MESH) {
                // Use the material and update OpenGL state
                entry.rc->material_->apply();

                // Update the model matrix by the global transform of the object
                shader->setUniform("model", entry.global_transform);

                // Render the object
                void* offset = (void*)(entry.rc->mesh_->getIndexOffset() * sizeof(unsigned int));
                glDrawElements(GL_TRIANGLES, entry.rc->mesh_->getNumIndices(), GL_UNSIGNED_INT, offset);
            } else if (entry.rc->type_ == RenderType::EXPLODED_MODEL) {
                const auto* model = entry.rc->exploded_model_;
                for (auto& mesh : model->getMeshes()) {
                    auto material = model->getMaterials()[mesh.material_index_];

                    material->apply();
                    shader->setUniform("model", entry.global_transform);
                    shader->setUniform("uTime", entry.custom_data_f32);
                    glDrawArrays(GL_TRIANGLES, mesh.vertex_offset_, mesh.vertex_count_);
                }
            }
        }

        // Unbind VAO
        glBindVertexArray(INVALID_VAO);
        current_VAO_ = INVALID_VAO;

        // Clear the render queue for the next frame
        // render_queue_.clear();
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
        render_queue_.push_back({ key, rc, global_transform, .custom_data_i32 = 0 });
    }

    template<typename T>
    void submit(GameObject* obj, T custom_data) {
        const auto* rc = &obj->getRenderComponent();
        if (!rc->renderable_) {
            return;
        }

        unsigned int key = (rc->material_->getShader()->getShaderID() << 16) | rc->VAO_;
        glm::mat4 global_transform = obj->getTransformComponent().getGlobalModelMatrix();
        // std::cout << "Add (key = " << key << ", type = " << (size_t)rc->type_ << ", global_transform = " << global_transform << ", custom_data = " << custom_data << ")" << std::endl;
        render_queue_.push_back({ key, rc, global_transform, custom_data });
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
    void beginShadowPass(const Light& light) {
        light_view_matrix_ = light.getLightSpaceMatrix();
        glBindFramebuffer(GL_FRAMEBUFFER, shadow_map_.getFBO());
        glViewport(0, 0, ShadowMap::SHADOW_WIDTH, ShadowMap::SHADOW_HEIGHT);
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);

        if (!depth_shader_) {
            auto shader_manager = ServiceLocator<ShaderManager>::get();
            depth_shader_ = shader_manager->getShader("depth");
        }
    }

    void endShadowPass(Window& window) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        int window_width = 0, window_height = 0;
        window.getFramebufferSize(window_width, window_height);
        glViewport(0, 0, window_width, window_height);
    }

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

        union {
            float custom_data_f32;
            int32_t custom_data_i32;
            uint32_t custom_data_u32;
        };

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
    glm::mat4 light_view_matrix_= glm::mat4(1.0f);
};
