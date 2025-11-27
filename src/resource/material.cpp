#include "resource/material.h"
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

    // std::cout << "Loading material: " << material->GetName().C_Str() << std::endl;

    textures_.resize((size_t)TextureType::_COUNT);

    // Load texture for each texture slot
    for (size_t i = 0; i < (size_t)TextureType::_COUNT; ++i) {
        has_texture_[i] = false;
        auto texture_manager = ServiceLocator<TextureManager>::get();
        if (texture_manager == nullptr) {
            std::cerr << "Failed to get texture manager" << std::endl;
            return false;
        }

        auto assimp_type = kTableTextureType[i];
        unsigned int texture_count = material->GetTextureCount(assimp_type);
        // std::cout << "Texture count for " << i << " is " << texture_count << std::endl;
        if (texture_count > 0) {
            aiString path;
            aiReturn result = material->GetTexture(assimp_type, 0, &path, nullptr);
            if (result == aiReturn_SUCCESS) {
                std::string full_path = (directory / path.data).string();

                textures_[i] = texture_manager->getTexture(full_path);
                if (textures_[i] == nullptr || texture_manager->isDefault(textures_[i])) {
                    std::cerr << "Failed to load texture: " << full_path << std::endl;
                } else {
                    has_texture_[i] = true;
                }
            }
        }
    }

    // Load base color for each texture slot
    aiColor4D base_color(1.0f, 1.0f, 1.0f, 1.0f);
    if (material->Get(AI_MATKEY_COLOR_DIFFUSE, base_color)) {
        base_color = aiColor4D(1.0f, 1.0f, 1.0f, 1.0f);
    }
    base_colors_ = glm::vec4(base_color.r, base_color.g, base_color.b, base_color.a);

    // 导入金属度
    if (AI_SUCCESS == material->Get(AI_MATKEY_METALLIC_FACTOR, metallic_)) {

    }

    // 导入粗糙度
    if (AI_SUCCESS == material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness_)) {
        
    }

    // 导入镜面反射系数
    if (AI_SUCCESS == material->Get(AI_MATKEY_SPECULAR_FACTOR, specular_)) {
        // 镜面反射系数，默认 0.35
    }

    // 导入镜面反射颜色
    aiColor3D specularColor = aiColor3D(1.0f, 1.0f, 1.0f);
    if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_SPECULAR, specularColor)) {
    
    }
    specularColor_ = glm::vec3(specularColor.r, specularColor.g, specularColor.b);

    return true;
}

// Apply material to corresponding shader program
void Material::apply() {
    auto base_color = base_colors_;
    shader_->setUniform("baseColorFactor", base_color);

    bool has_texture = false;
    for (int i = 0; i < (int)TextureType::_COUNT; ++i) {
        if (has_texture_[i]) {
            has_texture = true;
            break;
        }
    }

    shader_->setUniform("hasTexture", has_texture);

    for (int i = 0; i < (int)TextureType::_COUNT; ++i) {
        if (has_texture_[i]) {
            textures_[i]->Bind(GL_TEXTURE0 + i);
        }
    }
    // else {
    //     auto texture_manager = ServiceLocator<TextureManager>::get();
    //     texture_manager->getDefaultTexture()->Bind(textureUnit);
    // }
    shader_->setUniform("metallicFactor", metallic_);// 金属度输入着色器
    shader_->setUniform("roughnessFactor", roughness_);// 粗糙度输入着色器
    // shader_->setUniform("specularFactor", specular_);// 镜面反射系数输入着色器
    // shader_->setUniform("specularColorFactor", specularColor_);// 镜面颜色输入着色器
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
