#pragma once

#include <filesystem>
#include "assimp/material.h"
#include "shader.h"
#include "resource/texture.h"

// Texture type
// e.g. diffuse, specular, normal, height
enum class TextureType {
    kDIFFUSE = 0,
    // kSPECULAR = 1,
    _COUNT
};

// A table mapping TextureType to aiTextureType in Assimp
inline aiTextureType kTableTextureType[] = {
    [(size_t)TextureType::kDIFFUSE] = aiTextureType_DIFFUSE,
    // [(size_t)TextureType::kSPECULAR] = aiTextureType_SPECULAR,
};


// Material class
// Provide method to load material from Assimp aiMaterial, and to apply material to shader program
// Each Material instance holds a reference to a Shader instance, and a set of slots for textures and base colors
class Material {
public:
    Material(const Shader* shader) : shader_(shader) {}
    Material(const Shader* shader, const aiMaterial* material, const std::filesystem::path& directory) : shader_(shader) {
        bool success = loadMaterial(material, directory);
        if (!success) {
            std::cerr << "Failed to load material" << std::endl;
        }
    }

    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;

    // Load material from Assimp aiMaterial
    // Return true if success, false otherwise
    bool loadMaterial(const aiMaterial* material, const std::filesystem::path& directory);

    // Apply material to shader program
    void apply();

    // Setters and getters
    void setShader(const Shader* shader) {
        shader_ = shader;
    }

    void setShader(const std::string& name);

    const Shader* getShader() const {
        return shader_;
    }

    glm::vec4 getBaseColor() {
        return base_colors_;
    }

    // Debugging
    const std::string toString() const {
        std::string str = "Material(shader: " + shader_->toString() + ", base_color: (" + std::to_string(base_colors_.r) + " " + std::to_string(base_colors_.g) + " " + std::to_string(base_colors_.b) + " " + std::to_string(base_colors_.a) + "), texture slots: [";
        for (size_t i = 0; i < (size_t)TextureType::_COUNT; ++i) {
            std::string texture_str = std::string("\n\t{\n\t\thas_texture: ") + (has_texture_[i] ? "Y" : "N");
            if (has_texture_[i]) {
                texture_str += std::string(",\n\t\ttexture: ") + textures_[i]->toString();
            }

            str += texture_str + "\n\t},";
        }
        return str + "\n\t], addr = " + std::to_string((size_t)this) + ")";
    }

private:
    // shader program
    const Shader* shader_;

    // texture slots
    bool has_texture_[(size_t)TextureType::_COUNT] { false };
    std::vector<std::shared_ptr<const Texture> > textures_;

    // material properties
    glm::vec4 base_colors_;                     // 材质的基本颜色
    float metallic_ { 0.0f };                   // 材质的金属度
    float roughness_ { 0.5f };                  // 材质的粗糙度
    float specular_ { 0.35f };                  // 材质的高光系数
    glm::vec3 specularColor_ { 1.0f };  // 材质的高光颜色
};


class MaterialManager {
public:
    MaterialManager() = default;
    ~MaterialManager() = default;

    MaterialManager(const MaterialManager&) = delete;
    MaterialManager& operator=(const MaterialManager&) = delete;

    void init();

    void clear() {
        materials_.clear();
    }

    // Load material from Assimp aiMaterial.
    // If material already loaded, return the loaded material
    // Otherwise, create a new material and load it
    // Return default material if failed to load material
    std::shared_ptr<Material> loadMaterial(const Shader* shader, const aiMaterial* material, const std::filesystem::path& directory) {
        std::string key = (directory / material->GetName().C_Str()).string();

        auto loaded_material = getMaterial(key);

        if (loaded_material != nullptr && loaded_material != default_material_) {
            return loaded_material;
        }

        auto material_ptr = std::make_shared<Material>(shader);

        bool success = material_ptr->loadMaterial(material, directory);
        if (!success) {
            std::cerr << "Failed to load material: " << key << std::endl;
            return default_material_;
        }
        
        materials_[key] = material_ptr;

        // std::cout << "Loaded material: " << key << std::endl;

        return material_ptr;
    }

    // Get material by material key
    std::shared_ptr<Material> getMaterial(const std::string& material_key) {
        auto it = materials_.find(material_key);
        if (it != materials_.end()) {
            return it->second;
        } else {
            return default_material_;
        }
    }

private:
    std::unordered_map<std::string, std::shared_ptr<Material>> materials_;
    std::shared_ptr<Material> default_material_;
};
