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
    base_colors_.resize((size_t)TextureType::_COUNT, glm::vec4(1.0f));

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
    for (size_t i = 0; i < (size_t)TextureType::_COUNT; ++i) {
        aiColor4D base_color(1.0f, 1.0f, 1.0f, 1.0f);
        if (material->Get(kTableMatkeyColor[i].pKey, kTableMatkeyColor[i].type, kTableMatkeyColor[i].idx, base_color)) {
            base_color = aiColor4D(1.0f, 1.0f, 1.0f, 1.0f);
        }
        base_colors_[i] = glm::vec4(base_color.r, base_color.g, base_color.b, base_color.a);
    }

    return true;
}

// Apply material to corresponding shader program
void Material::apply(TextureType type, int textureUnit) {
    auto base_color = base_colors_[(size_t)type];
    shader_->setUniform("baseColorFactor", base_color);

    bool has_texture = has_texture_[(size_t)type];
    shader_->setUniform("hasTexture", has_texture);

    if (has_texture) {
        textures_[(size_t)type]->Bind(textureUnit);
    }
    // else {
    //     auto texture_manager = ServiceLocator<TextureManager>::get();
    //     texture_manager->getDefaultTexture()->Bind(textureUnit);
    // }
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
