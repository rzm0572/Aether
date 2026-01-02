#version 410

// 每个顶点代表 billboard 的一个角点
layout (location = 0) in float inParticleID;      // 所属粒子 ID（用于随机方向）
layout (location = 1) in vec2 inCorner;         // 局部空间角点：(-1,-1), (1,-1), (-1,1), (1,1)
layout (location = 2) in vec3 inEmitOffset;     // 发射偏移（通常为 0）
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
uniform float scale;   // 爆炸范围
uniform float life_time;    // 粒子总寿命（秒）
uniform float Max_size; // 粒子的最大大小
uniform vec3 uCenterVelocity; // 中心速度（用于控制粒子群的速度）
uniform float Button_size; // 子弹大小

void main() {
    // 计算当前粒子已存在的时间（秒）
    // 整个生命周期都应该有发射，这条计算发射时间
    float startTime = life_time * startTimeFrac;
    float age = uTime - uExplosionTime - startTime;
    if (age > 0.0) {
        float t = mod(age , life_time) / life_time;  // 归一化生命 [0,1]

        vec3 dir = normalize(-uCenterVelocity); // 子弹中心速度作为方向
        vec3 center = uExplosionPos + inEmitOffset  + dir * t * scale + inOffset * (1.0-t) * Button_size;
        if(inParticleID < 0.5){
            center = uExplosionPos;
        }
        // === Billboard 构建 ===   目的是由于一个粒子是一个片，所以需要面向相机
        vec3 toCamera = normalize(uCameraPos - center);
        vec3 worldUp = vec3(0, 1, 0);
        vec3 right = normalize(cross(toCamera, worldUp)); // 右方向
        vec3 up = cross(right, toCamera);                 // 上方向（正交化）

        // 控制粒子大小，首先粒子本身不能太大，其次粒子大小也会随着时间变化，比如爆炸后时间越久粒子越远，而且大小变小
        float size = Max_size * (1.0 - t * t + 0.1 * t);
        if(inParticleID < 0.5){
            size = Button_size;
        }

        // 将局部角点映射到世界空间
        vec3 worldPos = center + right * inCorner.x * size + up * inCorner.y * size;

        // 输出裁剪空间位置
        gl_Position = uProjection * uView * vec4(worldPos, 1.0);

        // 纹理坐标：将 [-1,1] 映射到 [0,1] 
        TexCoord = inCorner * 0.5 + 0.5;

        // 粒子颜色：根据粒子生命周期变化，从红到黄到白色
        vec3 basecolor;
        float alpha = 0.0; // 更柔和的 fade-out
        if(inParticleID < 0.5){
            basecolor = vec3(1.0, 1.0, 1.0);
        }
        else{
            if (t < 0.9) {
                basecolor = vec3(0.9,0.9,0.9);
            } else {
                // 最后阶段加入一点青蓝色辉光（模拟高温余烬）
                basecolor = mix(vec3(0.9, 0.9, 0.9), vec3(0.8, 1.0, 1.2), (t - 0.9) / 0.1);
            }
            // alpha =smoothstep(0.0, 0.1, 1.0 - t)* (1.0 - t * 0.7); // 整体衰减
        }


        Color = vec4(basecolor , 1.0 - alpha); // 高光增强：越年轻越亮！
        // Color = vec4(1.0,1.0,1.0,1.0); // 调试用，显示白色粒子
    }
    else{// 如果不需要渲染
        // 那么我们丢到一边去就行了
        gl_Position = uProjection * uView * vec4(-10000,-10000,-10000, 1.0);

        // 纹理坐标：将 [-1,1] 映射到 [0,1] 
        TexCoord = inCorner * 0.5 + 0.5;
        Color = vec4(0.0, 0.0,0.0,0.0);
    }
}