#version 330 core

in vec3 LineColor;
out vec4 FragColor;

void main()
{
    // 输出不透明的颜色
    FragColor = vec4(LineColor, 1.0);
}