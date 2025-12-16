#version 410

// 每个顶点代表 billboard 的一个角点
layout (location = 0) in float RandNum;      // 一个随机数
layout (location = 1) in vec2 inCorner;         // 局部空间角点：(-1,-1), (1,-1), (-1,1), (1,1)
layout (location = 2) in vec3 inEmitOffset;     // 发射偏移（通常为 0）
layout (location = 3) in float startTimeFrac;      // 粒子发射时间可以有先后顺序
layout (location = 4) in vec3 inVelocity;       // 粒子速度
layout (location = 5) in float ParticleKind;      // 粒子种类
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
uniform float raw_life_time;    // 粒子总寿命（秒）
uniform float Max_size; // 粒子的最大大小

void main() {
    // 计算当前粒子已存在的时间（秒）
    // 整个生命周期都应该有发射，这条计算发射时间
    float life_time = raw_life_time;
    if(ParticleKind >= 0.5){
        life_time *= 0.5;
    }
    float startTime = life_time * startTimeFrac;
    float age = uTime - uExplosionTime - startTime;
    if (age > 0.0 && age < life_time) {
        float t = age / life_time;  // 归一化生命 [0,1]

        vec3 dir = normalize(inVelocity); // 粒子速度作为方向

        // 计算粒子中心世界位置：
        // - 径向飞出（dir * t * speed）+ 炸完缩回
        vec3 center = uExplosionPos + inEmitOffset*scale ;//这是一个加上微重力的爆心
        float  displacement = 0;// 一个先快后慢的扩散之后收缩的过程

        if (ParticleKind >= 0.5){// 火星飞溅距离更远
            displacement = scale * 1.7 * (t * t - 2 * t);
            // 相对于粒子大小再引入一个随机的三维偏移量
            vec3 random_offset = vec3(
                sin(RandNum * 78.233 * t),
                sin(RandNum * 98.233 * t),
                sin(RandNum * 118.233 * t)
            ) * t; // 偏移量随时间增加
            center += random_offset * Max_size;
        }
        else{
            displacement = sin(t * 1.57) * scale;
            center += vec3(0, - t * 0.5 * scale, 0);
                        // 相对于粒子大小再引入一个随机的三维偏移量
            // vec3 random_offset = vec3(
            //     sin(RandNum * 78.233 * t),
            //     sin(RandNum * 98.233 * t),
            //     sin(RandNum * 118.233 * t)
            // ) * t; // 偏移量随时间增加
            // center += random_offset * Max_size * 2.0;
        }
        center += dir * displacement; // 径向飞出

        // === Billboard 构建 ===   目的是由于一个粒子是一个片，所以需要面向相机
        vec3 toCamera = normalize(uCameraPos - center);
        vec3 worldUp = vec3(0, 1, 0);
        vec3 right = normalize(cross(toCamera, worldUp)); // 右方向
        vec3 up = cross(right, toCamera);                 // 上方向（正交化）

        // 控制粒子大小，烟灰会变小
        float size = Max_size * (1.0 - t*t/2.0) * (RandNum * 0.25 + 0.75); 

        // 将局部角点映射到世界空间
        vec3 worldPos = center + right * inCorner.x * size + up * inCorner.y * size;

        // 输出裁剪空间位置
        gl_Position = uProjection * uView * vec4(worldPos, 1.0);

        // 纹理坐标：将 [-1,1] 映射到 [0,1] 
        TexCoord = inCorner * 0.5 + 0.5;

        // 爆炸的颜色变化是先亮黄，再变橙红，最后黑
        vec3 baseColor;
        if(ParticleKind >= 0.5){
            if (t < 0.3) {
                // 初始：明亮金黄（火星刚飞溅出）
                float fade = t / 0.3;
                baseColor = mix(vec3(1.0, 0.85, 0.2), vec3(1.0, 0.6, 0.1), fade);
            } else if (t < 0.6) {
                // 中期：橙红（快速冷却）
                float fade = (t - 0.3) / (0.6 - 0.3);
                baseColor = mix(vec3(1.0, 0.6, 0.1), vec3(0.7, 0.3, 0.05), fade);
            } else if (t < 0.98) {
                // 后期：暗红（即将熄灭）
                float fade = (t - 0.6) / (0.98 - 0.6);
                baseColor = mix(vec3(0.7, 0.3, 0.05), vec3(0.25, 0.12, 0.06), fade);
            } else {
                // 熄灭：接近灰黑（余烬）
                baseColor = vec3(0.15, 0.1, 0.08);
            }
        }
        else{
            if (t < 0.15) {
                // 极早期：耀眼白光
                float fade = t / 0.15;
                baseColor = mix(vec3(1.0, 1.0, 1.0), vec3(1.0, 0.95, 0.8), fade);
            } else if (t < 0.5) {
                // 早期：明亮黄白色-->金黄
                float fade = (t - 0.15) / (0.5 - 0.15);
                baseColor = mix(vec3(1.0, 0.95, 0.8), vec3(1.0, 0.7, 0.2), fade);
            } else if (t < 0.9) {
                // 中期：橙红火焰
                float fade = (t - 0.5) / (0.9 - 0.4);
                baseColor = mix(vec3(1.0, 0.7, 0.2), vec3(0.8, 0.3, 0.05), fade);
            } else {
                // 后期：暗红-->灰黑
                float fade = (t - 0.9) / (1.0 - 0.9);
                baseColor = mix(vec3(0.8, 0.3, 0.05), vec3(0.15, 0.1, 0.08), fade);
            }
        }

        float alpha = 1.0 - t; // 越到后期烟雾越来越透明



        Color = vec4(baseColor, alpha);
    }
    else{// 如果不需要渲染
        // 那么我们丢到一边去就行了
        gl_Position = uProjection * uView * vec4(-10000,-10000,-10000, 1.0);

        // 纹理坐标：将 [-1,1] 映射到 [0,1] 
        TexCoord = inCorner * 0.5 + 0.5;
        Color = vec4(0.0, 0.0,0.0,0.0);
    }
}