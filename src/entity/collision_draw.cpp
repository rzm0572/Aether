#include "entity/collision_draw.h"
#include "service/service_locator.h"
#include "resource/shader.h"

void drawLines(const std::vector<DebugLine>& lines, const glm::mat4& view, const glm::mat4& proj) {
    if (lines.empty()) return;

    static GLuint vao = 0, vbo = 0;
    if (vao == 0) {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3) * 2, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3) * 2, (void*)sizeof(glm::vec3));
    }
    
    struct Vertex { glm::vec3 pos; glm::vec3 col; };
    std::vector<Vertex> vertices;
    vertices.reserve(lines.size() * 2);
    for(const auto& line : lines) {
        vertices.push_back({line.start, line.color});
        vertices.push_back({line.end, line.color});
    }

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STREAM_DRAW);

    glDrawArrays(GL_LINES, 0, vertices.size());
}

void renderCollisionBox(const std::vector<DebugLine>& lines, 
                      const glm::mat4& view, 
                      const glm::mat4& projection) {
    if (lines.empty()) return;

    auto debugShader = ServiceLocator<ShaderManager>::get()->useShader("debug_line");
    
    debugShader->setUniform("view", view);
    debugShader->setUniform("projection", projection);

    GLfloat originalLineWidth;
    glGetFloatv(GL_LINE_WIDTH, &originalLineWidth);
    
    glLineWidth(4.0f);

    drawLines(lines, view, projection);

    glLineWidth(originalLineWidth);
}

