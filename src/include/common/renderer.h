#pragma once

#include "common/game_object.h"
#include "component/render.h"
#include "world/light.h"
#include <glm/glm.hpp>

class Renderer {
public:
    void render(glm::mat4 view, glm::mat4 projection, const Light& light) {
        std::sort(render_queue_.begin(), render_queue_.end());

        current_VAO_ = INVALID_VAO;
        current_shader_ID_ = 0;

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        for (const auto& entry : render_queue_) {
            if (!entry.rc->renderable_) {
                continue;
            }

            if (current_VAO_ != entry.rc->VAO_) {
                // std::cout << "VAO changed from " << current_VAO_ << " to " << entry.rc->VAO_ << std::endl;
                glBindVertexArray(entry.rc->VAO_);
                current_VAO_ = entry.rc->VAO_;
            }

            const auto* shader = entry.rc->material_->getShader();
            if (current_shader_ID_ != shader->getShaderID()) {
                shader->useShader();
                current_shader_ID_ = shader->getShaderID();

                shader->setUniform("view", view);
                shader->setUniform("projection", projection);
                light.use(shader);
            }

            entry.rc->material_->apply();

            shader->setUniform("model", entry.global_transform);
            shader->setUniform("ourTexture", 0);

            void* offset = (void*)(entry.rc->mesh_->getIndexOffset() * sizeof(unsigned int));
            glDrawElements(GL_TRIANGLES, entry.rc->mesh_->getNumIndices(), GL_UNSIGNED_INT, offset);
        }

        glBindVertexArray(INVALID_VAO);
        current_VAO_ = INVALID_VAO;

        render_queue_.clear();
    }

    void submit(GameObject* obj) {
        const auto* rc = &obj->getRenderComponent();
        if (!rc->renderable_) {
            return;
        }

        unsigned int key = (rc->material_->getShader()->getShaderID() << 16) | rc->VAO_;
        glm::mat4 global_transform = obj->getTransformComponent().getGlobalModelMatrix();
        render_queue_.push_back({ key, rc, global_transform });
    }

    void submit_recursive(GameObject* obj) {
        submit(obj);
        for (auto* child : obj->getChildren()) {
            submit_recursive(child);
        }
    }

private:
    struct RenderQueueEntry {
        unsigned int key;
        const RenderComponent* rc;
        glm::mat4 global_transform;

        bool operator<(const RenderQueueEntry& other) const {
            return key < other.key;
        }
    };

    std::vector<RenderQueueEntry> render_queue_;

    GLuint current_VAO_ {INVALID_VAO};
    unsigned int current_shader_ID_ {0};
};
