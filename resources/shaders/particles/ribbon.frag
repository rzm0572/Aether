#version 410

in vec2 TexCoord;
in vec4 Color;
out vec4 fragColor;

uniform sampler2D uSpriteTex;

void main() {
    vec4 texColor = texture(uSpriteTex, TexCoord);
    // fragColor = Color * texColor;
    fragColor = vec4(0.0,0.0,0.0,1.0);
    if (fragColor.a < 0.01) discard;
}