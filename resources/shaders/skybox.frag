#version 410 core
in vec3 TexCoord0;
out vec4 FragColor;
uniform samplerCube gCubemapTexture;
uniform vec3 gTintColor;
void main()
{
    vec4 BaseColor = texture(gCubemapTexture, TexCoord0);
    FragColor = vec4(gTintColor, 1.0) * BaseColor;
}