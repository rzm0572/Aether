#version 410 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in int aFragmentID;
layout (location = 4) in vec3 aFragmentCenter;
layout (location = 5) in vec3 aFragmentVelocity;
layout (location = 6) in vec3 aFragmentAxis;

uniform float uTime;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoord;
out vec3 FragPos;
out vec3 Normal;

mat4 axisAngleRotation(vec3 axis, float angle) {
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    
    return mat4(
        oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
        oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
        oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
        0.0,                                0.0,                                0.0,                                1.0
    );
}

void main() {
    float rotationSpeed = 2.0;

    int id = aFragmentID;

    vec3 displacement = aFragmentVelocity * uTime;
    vec3 axis = aFragmentAxis;

    mat4 rotationMatrix = axisAngleRotation(axis, uTime * rotationSpeed);

    vec3 localPos = aPos - aFragmentCenter;
    vec3 rotatedPos = (rotationMatrix * vec4(localPos, 1.0)).xyz;
    vec3 finalPos = rotatedPos + aFragmentCenter + displacement;

    FragPos = vec3(model * vec4(finalPos, 1.0));
    gl_Position = projection * view * vec4(FragPos, 1.0);

    TexCoord = aTexCoord;
    Normal = mat3(model) * mat3(rotationMatrix) * aNormal;
}
