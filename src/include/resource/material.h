#pragma once

#include <array>
#include <fstream>
#include <filesystem>
#include "assimp/material.h"
#include "utils/profiler.h"
#include "shader.h"
#include "resource/texture.h"
#include <variant>
#include <unordered_map>
#include <string>

// Assimp 的 aiTextureType 枚举最大值（截至 Assimp 5.x，共 17 种）
constexpr size_t kMaxTextureTypes = 17; // 或 aiTextureType_UNKNOWN + 1，但需确认

inline const std::array<std::string, kMaxTextureTypes> kAssimpTextureTypeStr = []() {
    std::array<std::string, kMaxTextureTypes> arr{};
    arr[aiTextureType_DIFFUSE] = "Diffuse";
    arr[aiTextureType_SPECULAR] = "Specular";
    arr[aiTextureType_AMBIENT] = "Ambient";
    arr[aiTextureType_EMISSIVE] = "Emissive";
    arr[aiTextureType_HEIGHT] = "Height";
    arr[aiTextureType_NORMALS] = "Normal";
    arr[aiTextureType_SHININESS] = "Shininess";
    arr[aiTextureType_OPACITY] = "Opacity";
    return arr;
}();

// Material class
// Provide method to load material from Assimp aiMaterial, and to apply material to shader program
// Each Material instance holds a reference to a Shader instance, and a set of slots for textures and base colors
class Material {
private:
    struct TextureSlot {
        using TextureUnion = std::variant<float, glm::vec3, glm::vec4>;
        std::shared_ptr<const Texture> texture { nullptr };
        TextureUnion texture_const { 0.0f };

        TextureSlot(): texture_const(0.0f) {}
        TextureSlot(TextureUnion texture_const): texture_const(texture_const) {}
        TextureSlot(float texture_const): texture_const(texture_const) {}
        TextureSlot(glm::vec3 texture_const): texture_const(texture_const) {}
        TextureSlot(glm::vec4 texture_const): texture_const(texture_const) {}
        TextureSlot(std::shared_ptr<const Texture> texture, TextureUnion texture_const): texture(texture), texture_const(texture_const) {}
        TextureSlot(std::shared_ptr<const Texture> texture, float texture_const): texture(texture), texture_const(texture_const) {}
        TextureSlot(std::shared_ptr<const Texture> texture, glm::vec3 texture_const): texture(texture), texture_const(texture_const) {}
        TextureSlot(std::shared_ptr<const Texture> texture, glm::vec4 texture_const): texture(texture), texture_const(texture_const) {}

        void apply(const Shader* shader, const std::string& name, int& texture_unit) {
            if (texture != nullptr) {
                texture->Bind(texture_unit + GL_TEXTURE0);
                shader->setUniform("HasTexture" + name, true);
                shader->setUniform("Texture" + name, texture_unit);
                texture_unit++;
            } else {
                shader->setUniform("HasTexture" + name, false);
            }

            std::visit([&](auto&& arg) {
                // using T = std::decay_t<decltype(arg)>;
                shader->setUniform("Constant" + name, arg);
            }, texture_const);
        }

        void mult(float constant) {
            std::visit([&](auto&& arg) {
                arg *= constant;
            }, texture_const);
        }
    };

public:
    Material(const Shader* shader) : shader_(shader) {}
    Material(const Shader* shader, const aiMaterial* material, const std::filesystem::path& directory) : shader_(shader) {
        bool success = loadMaterial(material, directory);
        if (!success) {
            std::cerr << "Failed to load material" << std::endl;
        }
    }

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

    void setTexture(std::string name, std::shared_ptr<const Texture> texture, TextureSlot::TextureUnion constant = 0.0f) {
        if (texture_slots_.find(name) != texture_slots_.end()) {
            texture_slots_[name].texture = texture;
        } else {
            texture_slots_[name] = TextureSlot(texture, constant);
        }
    }

    void setConstant(std::string name, TextureSlot::TextureUnion constant) {
        if (texture_slots_.find(name) != texture_slots_.end()) {
            texture_slots_[name].texture_const = constant;
        } else {
            texture_slots_[name] = TextureSlot(constant);
        }
    }

    void multConstant(std::string name, float constant) {
        if (texture_slots_.find(name) != texture_slots_.end()) {
            texture_slots_[name].mult(constant);
        }
    }

    const Shader* getShader() const {
        return shader_;
    }

    const std::string getName() const {
        return name_;
    }

    // Debugging
    const std::string toString() const {
        std::string str = "Material(shader: " + shader_->toString() + ", texture slots: [";
        for (auto it : texture_slots_) {
            str += "\n\t{\n\t\tname: " + it.first + ",\n\t\ttexture_const: ";
            std::visit([&](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, std::shared_ptr<const Texture>>) {
                    str += arg->toString();
                } else if constexpr (std::is_same_v<T, glm::vec3>) {
                    str += "vec3(" + std::to_string(arg.x) + ", " + std::to_string(arg.y) + ", " + std::to_string(arg.z) + ")";
                } else if constexpr (std::is_same_v<T, glm::vec4>) {
                    str += "vec4(" + std::to_string(arg.x) + ", " + std::to_string(arg.y) + ", " + std::to_string(arg.z) + ", " + std::to_string(arg.w) + ")";
                } else if constexpr (std::is_same_v<T, float>) {
                    str += "float(" + std::to_string(arg) + ")";
                }
            }, it.second.texture_const);
            if (it.second.texture != nullptr) {
                str += "\n\t\ttexture: " + it.second.texture->toString();
            }
            str += "\n\t},";
        }
        return str + "\n\t], addr = " + std::to_string((size_t)this) + ")";
    }

    void exportToMtl(std::ofstream& ofs, std::string material_name = "default") {
        if (name_.empty()) {
            ofs << "newmtl " << material_name << std::endl;
        } else {
            ofs << "newmtl " << name_ << std::endl;
        }
        
        ofs << "Ka 0.200000 0.200000 0.200000" << std::endl;
        ofs << "Ke 0.000000 0.000000 0.000000" << std::endl;
        ofs << "Ni 1.000000" << std::endl;
        ofs << "d 1.000000" << std::endl;
        ofs << "illum 2" << std::endl;

        const auto& slots = texture_slots_;

        auto toVec3 = [](const auto& arg) -> glm::vec3 {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, glm::vec3>) {
                return arg;
            } else if constexpr (std::is_same_v<T, glm::vec4>) {
                return glm::vec3(arg.x, arg.y, arg.z);
            } else {
                return glm::vec3(arg, arg, arg);
            }
        };

        if (auto it = slots.find("Diffuse"); it != slots.end()) {
            glm::vec3 kd = std::visit(toVec3, it->second.texture_const);
            ofs << "Kd " << kd.x << " " << kd.y << " " << kd.z << std::endl;

            if (it->second.texture != nullptr) {
                std::filesystem::path texture_path = it->second.texture->getFilePath();
                ofs << "map_Kd " << texture_path.filename().string() << std::endl;
            }
        } else {
            ofs << "Kd 1.0 1.0 1.0" << std::endl;
        }

        if (auto it = slots.find("Specular"); it != slots.end()) {
            glm::vec3 ks = std::visit(toVec3, it->second.texture_const);
            ofs << "Ks " << ks.x << " " << ks.y << " " << ks.z << std::endl;

            if (it->second.texture != nullptr) {
                std::filesystem::path texture_path = it->second.texture->getFilePath();
                ofs << "map_Ks " << texture_path.filename().string() << std::endl;
            }
        } else {
            ofs << "Ks 0.35 0.35 0.35" << std::endl;
        }

        if (auto it = slots.find("Metallic"); it != slots.end()) {
            float metallic = std::get<float>(it->second.texture_const);
            ofs << "Pm " << metallic << std::endl;

            if (it->second.texture != nullptr) {
                std::filesystem::path texture_path = it->second.texture->getFilePath();
                ofs << "map_Pm " << texture_path.filename().string() << std::endl;
            }
        } else {
            ofs << "Pm 0.0" << std::endl;
        }

        if (auto it = slots.find("Roughness"); it != slots.end()) {
            float roughness = std::get<float>(it->second.texture_const);
            ofs << "Pr " << roughness << std::endl;

            if (it->second.texture != nullptr) {
                std::filesystem::path texture_path = it->second.texture->getFilePath();
                ofs << "map_Pr " << texture_path.filename().string() << std::endl;
            }
        } else {
            ofs << "Pr 0.5" << std::endl;
        }

        ofs << std::endl;
    }

private:
    // shader program
    const Shader* shader_;

    // material name
    std::string name_;

    // texture slots
    std::unordered_map<std::string, TextureSlot> texture_slots_;
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

    void registerMaterial(const std::string& material_key, std::shared_ptr<Material> material) {
        materials_[material_key] = material;
    }

private:
    std::unordered_map<std::string, std::shared_ptr<Material>> materials_;
    std::shared_ptr<Material> default_material_;
};
