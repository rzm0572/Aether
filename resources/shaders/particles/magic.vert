#version 410

// 每个顶点代表 billboard 的一个角点
layout (location = 0) in float inParticleID;      // 所属粒子 ID（用于随机方向）
layout (location = 1) in vec2 inCorner;         // 局部空间角点：(-1,-1), (1,-1), (-1,1), (1,1)
layout (location = 2) in vec3 inEmitOffset;     // 发射偏移，z 为转轴方向
layout (location = 3) in float startTimeFrac;      // 粒子发射时间可以有先后顺序
layout (location = 4) in vec3 inOffset;       // 粒子偏移位置
out vec2 TexCoord;
out vec4 Color;

// 全局 uniform
uniform float uTime;            // 当前全局时间（秒）
uniform float uExplosionTime;   // 本次爆炸触发时间
uniform vec3 uExplosionPos;     // 爆炸世界坐标中心
uniform mat4 uView;             // 相机视图矩阵
uniform mat4 uProjection;             // 投影矩阵
uniform vec3 uCameraPos;        // 相机世界位置
uniform float scale;   // 魔法长度缩放
uniform float life_time;    // 粒子总寿命（秒）
uniform float Max_size; // 粒子的最大大小
uniform vec3 uCenterVelocity; // 转轴方向
uniform float Button_size; // 魔法宽度
uniform float Rotate_speed; // 角速度
uniform vec3 Color_in;


void main() {
    // 计算当前粒子已存在的时间（秒）
    // 整个生命周期都应该有发射，这条计算发射时间
    float startTime = life_time * startTimeFrac;
    float age = uTime - uExplosionTime - startTime;
    // if (age > 0.0) {
    float t = mod(age , life_time) / life_time;  // 归一化生命 [0,1]

    vec3 Rotete_axis = normalize(uCenterVelocity); // 转轴方向
    vec3 Normal_Axis;
    if(dot(Rotete_axis,vec3(0,1,0))>0.99){
        Normal_Axis = cross(Rotete_axis,vec3(0,0,1));
    }
    else{
        Normal_Axis = cross(Rotete_axis,vec3(0,1,0));
    }
    Normal_Axis = normalize(Normal_Axis); // 法线方向
    vec3 Radial_Axis=normalize(cross(Normal_Axis,Rotete_axis)); // 径向轴
    // 最终偏移量
    float angle = t * Rotate_speed; // 角度
    // vec3 rotatedOffset = vec3(0,0,1) * cos(angle) +
    //             cross(vec3(0,0,1), inEmitOffset) * sin(angle) +
    //             vec3(0,0,1) * dot(vec3(0,0,1), inEmitOffset) * (1.0 - cos(angle));
    vec3 rotatedOffset = inEmitOffset ;
    rotatedOffset.x = inEmitOffset.x * cos(angle) - inEmitOffset.y * sin(angle);
    rotatedOffset.y = inEmitOffset.x * sin(angle) + inEmitOffset.y * cos(angle);
    rotatedOffset.x *= inEmitOffset.z + 1.0;
    rotatedOffset.y *= inEmitOffset.z + 1.0;
    // 粒子中心位置
    vec3 center = uExplosionPos + Rotete_axis * rotatedOffset.z * scale + Normal_Axis * rotatedOffset.y * Button_size + Radial_Axis *  rotatedOffset.x * Button_size;


    // === Billboard 构建 ===   目的是由于一个粒子是一个片，所以需要面向相机
    vec3 toCamera = normalize(uCameraPos - center);
    vec3 worldUp = vec3(0, 1, 0);
    vec3 right = normalize(cross(toCamera, worldUp)); // 右方向
    vec3 up = cross(right, toCamera);                 // 上方向（正交化）



    // 将局部角点映射到世界空间
    vec3 worldPos = center + right * inCorner.x * Max_size + up * inCorner.y * Max_size;

    // 输出裁剪空间位置
    gl_Position = uProjection * uView * vec4(worldPos, 1.0);

    // 纹理坐标：将 [-1,1] 映射到 [0,1] 
    TexCoord = inCorner * 0.5 + 0.5;

    Color = vec4(Color_in , 1.0); // 高光增强：越年轻越亮！
}