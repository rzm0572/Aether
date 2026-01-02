#include "resource/material.h"
#include "assimp/material.h"
#include "assimp/types.h"
#include "resource/texture.h"
#include "service/service_locator.h"

// Load material from Assimp aiMaterial.
// If material already loaded, return the loaded material
// Otherwise, create a new material and load it
// Return default material if failed to load material
bool Material::loadMaterial(const aiMaterial* material, const std::filesystem::path& directory) {
    if (material == nullptr) {
        return false;
    }

    std::cout << "Loading material: " << material->GetName().C_Str() << std::endl;

    name_ = material->GetName().C_Str();

    auto texture_manager = ServiceLocator<TextureManager>::get();
    if (texture_manager == nullptr) {
        std::cerr << "Failed to get texture manager" << std::endl;
        return false;
    }

    const size_t assimp_types = sizeof(kAssimpTextureTypeStr) / sizeof(kAssimpTextureTypeStr[0]);
    for (size_t i = 1; i < assimp_types; ++i) {
        aiTextureType type = static_cast<aiTextureType>(i);
        unsigned int texture_count = material->GetTextureCount(type);
        if (texture_count == 1) {
            aiString path;
            aiReturn result = material->GetTexture(type, 0, &path, nullptr);
            if (result == aiReturn_SUCCESS) {
                std::string full_path = (directory / path.data).string();

                auto texture = texture_manager->getTexture(full_path);
                setTexture(kAssimpTextureTypeStr[i], texture);
                if (texture == nullptr || texture_manager->isDefault(texture)) {
                    std::cerr << "Failed to load texture: " << full_path << std::endl;
                }
            }
        } else if (texture_count > 1) {
            std::vector<std::string> full_paths;
            for (unsigned int j = 0; j < texture_count; ++j) {
                aiString path;
                aiReturn result = material->GetTexture(type, j, &path, nullptr);

                if (result == aiReturn_SUCCESS) {
                    std::string full_path = (directory / path.data).string();
                    full_paths.push_back(full_path);
                } else {
                    full_paths.push_back("");
                }
            }
            
            const std::string name = (directory / kAssimpTextureTypeStr[i]).string();
            auto texture = texture_manager->getTexture(name, full_paths);
            setTexture(kAssimpTextureTypeStr[i], texture);
            if (texture == nullptr) {
                std::cerr << "Failed to load texture: " << name << std::endl;
            }
        }
    }

    // 导入漫反射颜色
    aiColor4D ai_diffuse_color(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec4 diffuse_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, ai_diffuse_color)) {
        diffuse_color = glm::vec4(ai_diffuse_color.r, ai_diffuse_color.g, ai_diffuse_color.b, ai_diffuse_color.a);
    }
    setConstant(kAssimpTextureTypeStr[aiTextureType_DIFFUSE], diffuse_color);

    // 导入镜面反射颜色
    aiColor3D ai_specular_color = aiColor3D(1.0f, 1.0f, 1.0f);
    glm::vec3 specular_color = glm::vec3(1.0f, 1.0f, 1.0f);
    if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_SPECULAR, ai_specular_color)) {
        specular_color = glm::vec3(ai_specular_color.r, ai_specular_color.g, ai_specular_color.b);
    }
    setConstant(kAssimpTextureTypeStr[aiTextureType_SPECULAR], specular_color);

    // 导入金属度
    float metallic = 0.0f;
    if (AI_SUCCESS == material->Get(AI_MATKEY_METALLIC_FACTOR, metallic)) {

    }
    setConstant("Metallic", metallic);
    // std::cout << "Metallic: " << metallic << std::endl;

    // 导入粗糙度
    float roughness = 0.5f;
    if (AI_SUCCESS == material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness)) {
        
    }
    setConstant("Roughness", roughness);

    // 导入镜面反射系数
    float specular_ = 0.35f;
    if (AI_SUCCESS == material->Get(AI_MATKEY_SPECULAR_FACTOR, specular_)) {
        // 镜面反射系数，默认 0.35
    }
    setConstant("Specular", specular_);

    return true;
}

// Apply material to corresponding shader program
void Material::apply() {
    int texture_unit = 0;
    for (auto& it : texture_slots_) {
        it.second.apply(shader_, it.first, texture_unit);
    }
    // std::cout << "Bind " << texture_unit << " textures" << std::endl;
}

// Set shader program for this material
void Material::setShader(const std::string& name) {
    auto shader = ServiceLocator<ShaderManager>::get()->getShader(name);
    if (shader != nullptr) {
        shader_ = shader;
    }
}

// Load default material
void MaterialManager::init() {
    default_material_ = std::make_shared<Material>(
        ServiceLocator<ShaderManager>::get()->getShader("default")
    );
}
