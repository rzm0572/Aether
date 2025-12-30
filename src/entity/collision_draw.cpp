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
        
        // 假设 Shader layout: location 0 = pos, location 1 = color
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3) * 2, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3) * 2, (void*)sizeof(glm::vec3));
    }

    // 将 DebugLine 结构平铺为 float 数组
    // DebugLine 内存布局通常是: float x1,y1,z1, r1,g1,b1,  x2,y2,z2, r2,g2,b2 ...
    // 但我们的 struct DebugLine { vec3 start, end, color } 不直接符合这个布局
    // 所以需要转换一下数据格式，或者修改 DebugLine 结构
    
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
    
    // 2. 设置 Uniforms
    debugShader->setUniform("view", view);
    debugShader->setUniform("projection", projection);

    // 3. 设置 OpenGL 状态（可选）
    // 保存旧的线宽
    GLfloat originalLineWidth;
    glGetFloatv(GL_LINE_WIDTH, &originalLineWidth);
    
    glLineWidth(4.0f); // 设置线条稍微粗一点，方便观察
    
    // 如果你想让碰撞箱“透视”显示（总是画在最上层），解开下面这行
    // glDisable(GL_DEPTH_TEST); 

    // 4. 调用之前的绘制函数 (假设你已经实现了上一条回答中的 drawLines)
    drawLines(lines, view, projection);

    // 5. 恢复状态
    glLineWidth(originalLineWidth);
    // glEnable(GL_DEPTH_TEST); // 如果上面禁用了，这里要恢复
}

