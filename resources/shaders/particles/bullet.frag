#version 410

in vec2 TexCoord;
in vec4 Color;
out vec4 fragColor;

uniform sampler2D uSpriteTex; // RGBA 粒子贴图（带 alpha）

void main() {
    // 采样纹理并混合颜色
    vec4 texColor = texture(uSpriteTex, TexCoord);
    fragColor = Color * texColor;
    // fragColor = tex; // 先测试，不用纹理
    // 透明度剔除（节省填充率）
    if (fragColor.a < 0.01) discard;
}