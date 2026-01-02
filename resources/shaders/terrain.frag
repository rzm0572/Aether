#version 410 core

in vec2 TexCoord;
in vec3 FragPos;
in float Height;

out vec4 FragColor;

// 地形法线
uniform sampler2DArray normalMap;
uniform int layerIndex;

// 材质属性 material
uniform sampler2DArray TextureDiffuse;
uniform sampler2D TextureSpecular;    // 镜面反射贴图
uniform sampler2D TextureMetallic;    // 金属度贴图
uniform sampler2D TextureRoughness;   // 粗糙度贴图

uniform bool HasTextureDiffuse;
uniform bool HasTextureSpecular;
uniform bool HasTextureMetallic;
uniform bool HasTextureRoughness;

uniform vec4 ConstantDiffuse;         // 漫反射系数（基本颜色）
uniform vec3 ConstantSpecular;        // 镜面反射系数（镜面颜色）
uniform float ConstantMetallic;       // 金属度
uniform float ConstantRoughness;      // 粗糙度

// 光照处理
uniform vec3 lightDir;   // 太阳光方向
uniform vec3 lightColor; // 太阳光颜色
uniform vec3 camPos;     // 相机位置

//环境光，外界输入
uniform vec3 ambientLight;

uniform float uvTiling;

// 阴影贴图
uniform sampler2D shadowMap;
// uniform sampler2DShadow shadowMap;
uniform mat4 lightSpaceMatrix;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    // 执行透视除法
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // 变换到 [0,1] 范围
    projCoords = projCoords * 0.5 + 0.5;
    // 获取最近点的深度
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    // 获取当前片段的深度
    float currentDepth = projCoords.z;
    // 阴影偏移（防止自阴影 artifacts） TODO: 调整偏移量 shadowOffset，太小会导致自己覆盖自己，太大会导致无法有效产生阴影
    float shadowOffset = 0.00005;
    float bias = shadowOffset * max( (1.0 - dot(normal, lightDir)), 0.1);
    float shadow = 0.0;
    // shadow = currentDepth > closestDepth ? 1.0 : 0.0;
    // PCF
    // 采样范围为 [-1,1]，采样次数为 (2.0 * half_sample + 1.0) * (2.0 * half_sample + 1.0)
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    int half_sample = 5;
    for(int x = -half_sample; x <= half_sample; ++x) {
        for(int y = -half_sample; y <= half_sample; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= (2.0 * half_sample + 1.0) * (2.0 * half_sample + 1.0);
    return shadow;

    // float shadow = 0.0;
    // // PCF
    // vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    // int half_sample = 4;
    // for(int x = -half_sample; x <= half_sample; ++x) {
    //     for(int y = -half_sample; y <= half_sample; ++y) {
    //         if(projCoords.z > 1.0){
    //             shadow += 0.0;
    //         }
    //         else{// 自动比较：projCoords.z vs shadowMap 中的深度
    //             shadow += texture(shadowMap, vec3(projCoords.xy+ vec2(x, y) * texelSize, projCoords.z));
    //         }            
    //     }
    // }
    // shadow /= (2.0 * half_sample + 1.0) * (2.0 * half_sample + 1.0);
    // return 1.0 - shadow;
}


vec3 GetTerrainColor(float height) {
    vec2 uv = TexCoord * uvTiling;

    vec3 colorSand = texture(TextureDiffuse, vec3(uv, 0)).rgb;
    vec3 colorGrass = texture(TextureDiffuse, vec3(uv, 1)).rgb;
    vec3 colorRock = texture(TextureDiffuse, vec3(uv, 2)).rgb;
    vec3 colorSnow = texture(TextureDiffuse, vec3(uv, 3)).rgb;

    const float heightSandMax = -10.0;
    const float heightGrassMin = 0.0;
    const float heightGrassMax = 40.0;
    const float heightRockMin = 50.0;
    const float heightRockMax = 70.0;
    const float heightSnowMin = 80.0;

    vec3 finalColor = vec3(0.0);

    if (height < heightSandMax) {
        finalColor = colorSand;
    } else if (height < heightGrassMin) {
        float t = (height - heightSandMax) / (heightGrassMin - heightSandMax);
        t = smoothstep(0.0, 1.0, t);
        finalColor = mix(colorSand, colorGrass, t);
    } else if (height < heightGrassMax) {
        finalColor = colorGrass;
    } else if (height < heightRockMin) {
        float t = (height - heightGrassMax) / (heightRockMin - heightGrassMax);
        t = smoothstep(0.0, 1.0, t);
        finalColor = mix(colorGrass, colorRock, t);
    } else if (height < heightRockMax) {
        finalColor = colorRock;
    } else if (height < heightSnowMin) {
        float t = (height - heightRockMax) / (heightSnowMin - heightRockMax);
        t = smoothstep(0.0, 1.0, t);
        finalColor = mix(colorRock, colorSnow, t);
    } else {
        finalColor = colorSnow;
    }

    return finalColor;
}


void main() {
    vec3 albedo;
    if (HasTextureDiffuse) {
        albedo = GetTerrainColor(Height) * ConstantDiffuse.rgb;
    } else {
        albedo = ConstantDiffuse.rgb;
    }

    // 准备向量
    vec3 Normal = texture(normalMap, vec3(TexCoord, layerIndex)).rgb;
    if (length(Normal) < 0.001) {
        Normal = vec3(0.0, 1.0, 0.0);
    }

    vec3 N = normalize(Normal);
    vec3 V = normalize(camPos - FragPos);// 视线方向
    vec3 L = normalize(lightDir);// 由于采用的是平行光源所以这里不减去世界位置
    vec3 H = normalize(L + V);//视线和光线的交点，即半角向量

    // 基础光照强度
    float NdotL =dot(N, L);// 法线和光线的点积
    NdotL = max(NdotL, 0.0);//防止背面光照

    // PBR 参数，包括金属度、粗糙度等
    float metallic = ConstantMetallic;
    if (HasTextureMetallic) {
        metallic = texture(TextureMetallic, TexCoord).r;
    }

    float roughness = ConstantRoughness;
    if (HasTextureRoughness) {
        roughness = texture(TextureRoughness, TexCoord).r;
    }
    roughness = max(roughness, 0.04); // 避免完全光滑导致 NaN 或过亮

    // F0: 基础反射率 —— 非金属固定为 0.04，金属则用 albedo
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    //  Cook-Torrance 实现实时PBR

    // 菲涅尔反射项使用 Schlick 近似
    // 菲涅耳效应：视线与表面夹角小，反射越强
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(H, V), 0.0), 5.0);

    // Normal Distribution Function 法线分布函数
    // 模拟表面微观凹凸
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float NdotH = max(dot(N, H), 0.0);
    float denom = NdotH * NdotH * (alpha2 - 1.0) + 1.0;
    float D = alpha2 / (3.14159265359 * denom * denom);

    // Geometry function 几何函数模拟微平面相互遮挡导致光线的能量减少或丢失的现象。
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float G1_L = NdotL / (NdotL * (1.0 - k) + k);
    float G1_V = max(dot(N, V), 0.0) / (max(dot(N, V), 0.0) * (1.0 - k) + k);
    float G = G1_L * G1_V;

    // Cook-Torrance 反射方程，Cook-Torrance BRDF 公式，计算镜面反射强度
    vec3 numerator = F * D * G;
    float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.001;
    vec3 specular = numerator / denominator;

    // 考虑能量守恒的漫反射
    vec3 kS = F;                    // 镜面反射比例
    vec3 kD = vec3(1.0) - kS;       // 漫反射比例
    kD *= 1.0 - metallic;           // 金属无漫反射

    vec3 diffuse = kD * albedo / 3.14159265359;

    // 叠加阴影贴图、自然光、直接光
    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    float shadow = ShadowCalculation(fragPosLightSpace, N, L);
    vec3 directLight = (diffuse + specular) * lightColor * NdotL * (1.0 - shadow);

    // 最终光照，叠加直接光和自然光
    // vec3 directLight = (diffuse + specular) * lightColor * NdotL;
    vec3 ambient = albedo * ambientLight; // 简单环境光

    vec3 color = directLight + ambient;

    // 实现HDR+Bloom，包含了伽马映射和色调矫正
    // color = color / (color + vec3(1.0));
    // color = pow(color, vec3(1.0/2.2)); 

    // 输出，增加透明度信息
    FragColor = vec4(color, ConstantDiffuse.a);
    // FragColor = vec4(N * 0.5 + 0.5, 1.0);
    // FragColor = vec4(Height / 100.0, Height / 100.0, Height / 100.0, 1.0);
}
