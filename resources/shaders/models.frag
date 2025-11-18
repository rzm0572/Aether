#version 410 core
in vec2 TexCoord;
out vec4 FragColor;

uniform vec4 baseColorFactor;   //  传入颜色
uniform bool hasTexture;        //  是否使用纹理
uniform sampler2D ourTexture;   //  可能未使用

void main() {
    if (hasTexture) {
        FragColor = baseColorFactor * texture(ourTexture, TexCoord);
    } else {
        FragColor = baseColorFactor; // 如果没有使用纹理则直接使用纯色
    }
}