#version 410

// 每个顶点代表 billboard 的一个角点
layout (location = 0) in vec2 inCorner;         // 局部空间角点：(-1,-1), (1,-1), (-1,1), (1,1)
out vec4 Color;

// 全局 uniform
uniform vec3 uExplosionPos;     // 爆炸世界坐标中心
uniform mat4 uView;             // 相机视图矩阵
uniform mat4 uProjection;             // 投影矩阵
uniform vec3 uCameraPos;        // 相机世界位置
uniform float scale;   // 长度
uniform float height;  // 高度
uniform float health;  // 生命值


void main() {

    vec3 center = uExplosionPos;


    // === Billboard 构建 ===   目的是由于一个粒子是一个片，所以需要面向相机
    vec3 toCamera = normalize(uCameraPos - center);
    vec3 worldUp = vec3(0, 1, 0);
    vec3 right = normalize(cross(toCamera, worldUp)); // 右方向
    vec3 up = cross(right, toCamera);                 // 上方向（正交化）



    // 将局部角点映射到世界空间
    vec3 worldPos = center + right * inCorner.x * scale + up * inCorner.y * height;
    if(inCorner.x > 0){
        worldPos -= right * inCorner.x * scale * (1.0-health)*2.0;
    }

    // 输出裁剪空间位置
    gl_Position = uProjection * uView * vec4(worldPos, 1.0);

    if(health > 0.3){
        Color = vec4(0.0,0.8,0.4 , 1.0);
    }
    else{
        Color = vec4(0.8,0.2,0.2, 1.0);
    }
    
}