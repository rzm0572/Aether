#version 410

layout (location = 0) in vec3 inPosition;
layout (location = 1) in float inTime;
layout (location = 2) in vec2 inTexCoord;

out vec2 TexCoord;
out vec4 Color;

uniform mat4 uView;
uniform mat4 uProjection;

void main() {
    if (inTime > 1.0 || inTime < 0.0) {
        gl_Position = vec4(-10000, -10000, -10000, 1.0);
        Color = vec4(0.0);
        return;
    }

    // 颜色：从亮黄到淡白
    vec3 startColor = vec3(0.95, 0.95, 0.98); // 微冷白（带一点点蓝）
    vec3 endColor   = vec3(0.85, 0.85, 0.90); // 更灰的白

    vec3 baseColor = mix(startColor, endColor, inTime);

    // 烟雾 alpha 应该随时间平滑衰减，且整体较低（更透明）
    float alpha = (1.0 - inTime) * 0.6;

    // 颜色
    Color = vec4(baseColor, alpha);

    // 纹理坐标
    TexCoord = inTexCoord;

    gl_Position = uProjection * uView * vec4(inPosition, 1.0);
}