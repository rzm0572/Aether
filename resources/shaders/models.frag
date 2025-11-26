#version 410 core
in vec2 TexCoord; //  纹理坐标
in vec3 FragPos;  //  顶点在世界的位置
in vec3 Normal;  //  法向量


// 输出颜色
out vec4 FragColor;//  输出颜色


// 材质属性 material
uniform vec4 baseColorFactor;   //  传入颜色
uniform bool hasTexture;        //  是否使用纹理
uniform sampler2D ourTexture;   //  纹理可能未使用
uniform float metallicFactor;   //  金属度
uniform float roughnessFactor;  //  粗糙度
// uniform float specularFactor;      // 镜面度
// uniform vec3 specularColorFactor;  // 镜面颜色


// 光照处理
uniform vec3 lightDir;   // 太阳光方向
uniform vec3 lightColor; // 太阳光颜色
uniform vec3 camPos;     // 相机位置

//环境光，外界输入
uniform vec3 ambientLight; 

void main() {
    // 获取不包含任何效果的基础颜色
    vec3 albedo;
    if (hasTexture) {
        albedo = texture(ourTexture, TexCoord).rgb * baseColorFactor.rgb;
    } else {
        albedo = baseColorFactor.rgb;
    }

    // 准备向量
    vec3 N = normalize(Normal);
    vec3 V = normalize(camPos - FragPos);// 视线方向
    vec3 L = normalize(lightDir);// 由于采用的是平行光源所以这里不减去世界位置
    vec3 H = normalize(L + V);//视线和光线的交点，即半角向量

    // 基础光照强度
    float NdotL =dot(N, L);// 法线和光线的点积
    NdotL = max(NdotL, 0.0);//防止背面光照

    // PBR 参数，包括金属度、粗糙度等
    float metallic = metallicFactor;
    float roughness = max(roughnessFactor, 0.04); // 避免完全光滑导致 NaN 或过亮

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

    // 最终光照，叠加直接光和自然光
    vec3 directLight = (diffuse + specular) * lightColor * NdotL;
    vec3 ambient = albedo * ambientLight; // 简单环境光

    vec3 color = directLight + ambient;

    // 伽马映射和色调矫正
    // color = color / (color + vec3(1.0));
    // color = pow(color, vec3(1.0/2.2)); 

    // 输出，增加透明度信息
    FragColor = vec4(color, baseColorFactor.a);

    // === 调试： ===
    // FragColor = vec4(NdotL, NdotL, NdotL, 1.0);//输出 NdotL 的灰度图
    // FragColor = vec4(normalize(Normal), 1.0); // 输出法线颜色
    // FragColor = vec4(albedo, 1.0); // 输出 albedo
    // FragColor = vec4(L, 1.0); // 输出太阳光
}