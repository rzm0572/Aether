#version 410 core

// 对应 C++ 中 glVertexAttribPointer 的 index
layout (location = 0) in vec3 aPos;   // 位置
layout (location = 1) in vec3 aColor; // 颜色

// 输出到片段着色器
out vec3 LineColor;

// 变换矩阵
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // 直接进行 视图 -> 投影 变换
    // 因为 aPos 已经是世界坐标了
    gl_Position = projection * view * vec4(aPos, 1.0);
    
    // 将颜色传递给片段着色器
    LineColor = aColor;
}