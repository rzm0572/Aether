#version 410

in vec2 TexCoord;
in vec4 Color;
out vec4 fragColor;

uniform sampler2D uSpriteTex; // RGBA 粒子贴图（带 alpha）

void main() {
    // 采样纹理并混合颜色
    vec4 texColor = texture(uSpriteTex, TexCoord);
    // fragColor = Color * texColor;
    // if(texColor.a > 250.0/256.0){
    //     fragColor = texColor;
    // }
    // else{
        fragColor = Color * texColor;
    // }
    // fragColor = vec4(0.0,1.0,0.0,1.0);
    // fragColor = Color; // 先测试，不用纹理
    // fragColor=texColor;
    // 透明度剔除（节省填充率）
    if (fragColor.a < 0.01) discard;
}