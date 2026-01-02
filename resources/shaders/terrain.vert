#version 410 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in int aIsSkirt;

out vec2 TexCoord;
out vec3 FragPos;
out float Height;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform sampler2DArray heightMap;
uniform int layerIndex;

uniform float skirtDepth;

void main() {
    float height = texture(heightMap, vec3(aTexCoords, layerIndex)).r;
    // float height = 0.0;
    
    if (aIsSkirt != 0) {
        height -= skirtDepth;
    }

    vec3 localPos = vec3(aPos.x, height, aPos.y);
    FragPos = vec3(model * vec4(localPos, 1.0));
    TexCoord = aTexCoords;
    Height = height;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
