#pragma once

#include "utils/path_handler.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glad/glad.h>
#include <stb_image.h>
#include <glm/glm.hpp>

#include <string>
#include <iostream>
#include <unordered_map>

// Texture class
class Texture {
    friend class TextureManager;

public:
    // 构造函数：指定纹理类型（通常是 GL_TEXTURE_2D）和文件路径
    Texture(
        GLenum type,
        const std::string& filepath,
        const std::string& name = "",
        bool alloc_gpu = true
    ): m_alloc_gpu(alloc_gpu), m_type(type), m_name(name), m_filepath(filepath), m_textureID(0), m_width(0), m_height(0), m_channels(0), m_depth(1) {
        if (m_name.empty() && !m_filepath.empty()) {
            m_name = m_filepath;
        }
    }
    
    ~Texture(){
        if (m_alloc_gpu && m_textureID) {
            glDeleteTextures(1, &m_textureID);
        }
        if (m_mipmapFBO) {
            glDeleteFramebuffers(1, &m_mipmapFBO);
        }
    }

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&& other) {
        m_alloc_gpu = other.m_alloc_gpu;
        m_type = other.m_type;
        m_filepath = std::move(other.m_filepath);
        m_textureID = other.m_textureID;
        m_width = other.m_width;
        m_height = other.m_height;
        m_channels = other.m_channels;
        m_depth = other.m_depth;

        other.m_textureID = 0;
    }

    Texture& operator=(Texture&& other) {
        if (this != &other) {
            if (m_alloc_gpu && m_textureID) {
                glDeleteTextures(1, &m_textureID);
            }

            m_alloc_gpu = other.m_alloc_gpu;
            m_type = other.m_type;
            m_filepath = std::move(other.m_filepath);
            m_textureID = other.m_textureID;
            m_width = other.m_width;
            m_height = other.m_height;
            m_channels = other.m_channels;
            m_depth = other.m_depth;

            other.m_textureID = 0;
        }
        return *this;
    }

    // Load texture from file
    // Return true if successful, false otherwise
    bool Load(){
        if (m_type != GL_TEXTURE_2D) {
            return false;
        }

        if (!m_alloc_gpu) {
            return true;
        }

        // 加载图像数据
        stbi_set_flip_vertically_on_load(true); // OpenGL 原点在左下，需翻转
        unsigned char* data = stbi_load(m_filepath.c_str(), &m_width, &m_height, &m_channels, 0);

        if (!data) {
            std::cerr << "Failed to load texture: " << m_filepath << std::endl;
            return false;
        }

        // 确定格式
        if (m_channels == 1) {
            format_ = GL_RED;
            internal_format_ = GL_RED;
        }
        else if (m_channels == 3) {
            format_ = GL_RGB;
            internal_format_ = GL_RGB;
        }
        else if (m_channels == 4) {
            format_ = GL_RGBA;
            internal_format_ = GL_RGBA;
        }
        else {
            std::cerr << "Unsupported number of channels: " << m_channels << " in " << m_filepath << std::endl;
            stbi_image_free(data);
            return false;
        }

        type_ = GL_UNSIGNED_BYTE;

        // 生成并配置纹理
        glGenTextures(1, &m_textureID);
        glBindTexture(m_type, m_textureID);

        glTexParameteri(m_type, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(m_type, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(m_type, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(m_type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(m_type, 0, internal_format_, m_width, m_height, 0, format_, type_, data);
        glGenerateMipmap(m_type);

        stbi_image_free(data);
        glBindTexture(m_type, 0);

        std::cout << "Loaded texture: " << m_filepath << " (" << m_width << "x" << m_height << ", " << m_channels << " channels)" << std::endl;
        return true;
    }
    // 绑定纹理到指定纹理单元（如 GL_TEXTURE0）
    void Bind(GLenum textureUnit = GL_TEXTURE0) const {
        if (!m_alloc_gpu) {
            return;
        }
        glActiveTexture(textureUnit);
        glBindTexture(m_type, m_textureID);
    }

    template<typename T>
    bool Load(const T* data, int width, int height, int channel, int depth = 1, bool use_mipmap = true, GLenum expand_mode = GL_REPEAT) {
        if (depth < 1 || (m_type == GL_TEXTURE_2D && depth > 1)) {
            return false;
        }

        switch (channel) {
            case 1:  format_ = GL_RED;   break;
            case 2:  format_ = GL_RG;    break;
            case 3:  format_ = GL_RGB;   break;
            case 4:  format_ = GL_RGBA;  break;
            default: {
                std::cerr << "Unsupported number of channels: " << channel << std::endl;
                return false;
            }
        }

        if constexpr (std::is_same_v<T, unsigned char>) {
            type_ = GL_UNSIGNED_BYTE;
            if (channel == 1) {
                internal_format_ = GL_R8;
            } else if (channel == 3) {
                internal_format_ = GL_RGB8;
            } else if (channel == 4) {
                internal_format_ = GL_RGBA8;
            }
        } else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, glm::vec3>) {
            type_ = GL_FLOAT;
            if (channel == 1) {
                internal_format_ = GL_R32F;
            } else if (channel == 3) {
                internal_format_ = GL_RGB32F;
            } else if (channel == 4) {
                internal_format_ = GL_RGBA32F;
            }
        } else {
            static_assert(false, "Unsupported texture data type");
        }

        glGenTextures(1, &m_textureID);
        glBindTexture(m_type, m_textureID);

        glTexParameteri(m_type, GL_TEXTURE_WRAP_S, expand_mode);
        glTexParameteri(m_type, GL_TEXTURE_WRAP_T, expand_mode);
        if (use_mipmap) {
            glTexParameteri(m_type, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        } else {
            glTexParameteri(m_type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }

        glTexParameteri(m_type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        int max_level = maxMipmapLevel(width, height);

        if (data != nullptr) {
            glTexImage3D(m_type, 0, internal_format_, width, height, depth, 0, format_, type_, data);
            if (use_mipmap) {
                glGenerateMipmap(m_type);
            }
        } else {
            if (use_mipmap) {
                for (int i = 0; i < max_level; ++i) {
                    int mip_width = std::max(1, width >> i);
                    int mip_height = std::max(1, height >> i);
                    glTexImage3D(m_type, i, internal_format_, mip_width, mip_height, depth, 0, format_, type_, nullptr);
                }
            } else {
                glTexImage3D(m_type, 0, internal_format_, width, height, depth, 0, format_, type_, nullptr);
            }
        }

        glBindTexture(m_type, 0);

        m_width = width;
        m_height = height;
        m_channels = channel;
        m_depth = depth;

        std::cout << "Loaded texture: " << m_filepath << " (" << m_width << "x" << m_height << ", " << m_channels << " channels, depth: " << m_depth << ")" << std::endl;
        return true;
    }

    template<typename T>
    bool updateTextureLayer(const T* data, int layer_index) {
        if (m_type != GL_TEXTURE_2D_ARRAY || layer_index < 0 || layer_index >= m_depth) {
            return false;
        }

        glBindTexture(m_type, m_textureID);

        // std::cout << "Update texture layer: " << m_filepath << " (" << m_width << "x" << m_height << ", " << m_channels << " channels, layer: " << layer_index << ")" << std::endl;

        glTexSubImage3D(m_type, 0, 0, 0, layer_index, m_width, m_height, 1, format_, type_, data);

        glBindTexture(m_type, 0);

        return true;
    }

    // 获取 OpenGL 纹理 ID（用于调试或高级用途）
    GLuint getID() const { return m_textureID; }

    const std::string& getFilePath() const {
        return m_filepath;
    }

    int getWidth() const {
        return m_width;
    }

    int getHeight() const {
        return m_height;
    }

    const std::string toString() const {
        return "Texture(type: " + std::to_string(m_type) + ", name: " + m_name + ", filepath: " + m_filepath + ", textureID: " + std::to_string(m_textureID) + ", width: " + std::to_string(m_width) + ", height: " + std::to_string(m_height) + ", channels: " + std::to_string(m_channels) + ", m_depth: " + std::to_string(m_depth) + ")";
    }

private:
    void initMipmapFBO() {
        if (m_mipmapFBO == 0) {
            glGenFramebuffers(1, &m_mipmapFBO);
        }
    }

    int maxMipmapLevel(int width, int height) const {
        return static_cast<int>(std::floor(std::log2(std::max(width, height)))) + 1;
    }

    bool m_alloc_gpu;
    GLenum m_type;                        // 纹理类型
    std::string m_name;                   // 纹理名称
    std::string m_filepath;               // 纹理文件路径
    GLuint m_textureID;                   // OpenGL 纹理 ID
    int m_width, m_height, m_channels;    // 图像宽度、高度、通道数
    int m_depth;                          // 纹理深度
    GLenum internal_format_;
    GLenum format_;
    GLenum type_;

    GLuint m_mipmapFBO { 0 };
};

// Texture manager class
// Hold a cache of textures which are loaded into GPU memory
// Provide methods to load and manage textures
class TextureManager {
public:
    TextureManager() = default;
    ~TextureManager() = default;

    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    // Load default texture
    void init(bool alloc_gpu = true) {
        alloc_gpu_ = alloc_gpu;
        default_texture_ = std::make_shared<Texture>(GL_TEXTURE_2D, getAssetPath("textures/white.png"), "default", alloc_gpu);
        default_texture_->Load();
    }

    // Get texture by filepath
    // If texture is not in cache, load it from file and add it to cache
    // If loading fails, return default texture
    std::shared_ptr<const Texture> getTexture(const std::string& name, const std::string& filepath, GLenum type = GL_TEXTURE_2D) {
        // Check if texture is already in cache
        auto it = textures_.find(name);
        if (it != textures_.end()) {
            return it->second;
        }

        // Load texture from file and add it to cache
        textures_.emplace(name, std::make_shared<Texture>(type, filepath, name, alloc_gpu_));
        auto texture = textures_.at(name);
        if (alloc_gpu_) {
            bool success = texture->Load();

            // If loading fails, remove texture from cache and return default texture
            if (!success) {
                textures_.erase(name);
                return default_texture_;
            }
        }

        return texture;
    }

    std::shared_ptr<const Texture> getTexture(const std::string& name, GLenum type = GL_TEXTURE_2D) {
        return getTexture(name, name, type);
    }

    std::shared_ptr<const Texture> getProceduralTexture(const std::string& name, GLenum type = GL_TEXTURE_2D) {
        auto it = textures_.find(name);
        if (it != textures_.end()) {
            return it->second;
        }

        textures_.emplace(name, std::make_shared<Texture>(type, "", name, alloc_gpu_));
        return textures_.at(name);
    }

    std::shared_ptr<const Texture> getTexture(const std::string name, const std::vector<std::string>& filepaths, GLenum type = GL_TEXTURE_2D_ARRAY) {
        auto it = textures_.find(name);
        if (it != textures_.end()) {
            return it->second;
        }

        if (!alloc_gpu_) {
            return nullptr;
        }

        auto texture = std::make_shared<Texture>(type, filepaths[0], name, alloc_gpu_);
        stbi_set_flip_vertically_on_load(true);

        int width = 0, height = 0, channels = 0;
        int depth = static_cast<int>(filepaths.size());
        unsigned char* data_first = stbi_load(filepaths[0].c_str(), &width, &height, &channels, 0);
        
        size_t image_size = width * height * channels;
        std::vector<unsigned char> data(image_size * depth);
        std::copy(data_first, data_first + image_size, data.data());
        stbi_image_free(data_first);

        for (int i = 1; i < depth; ++i) {
            int width_ = 0, height_ = 0, channels_ = 0;
            unsigned char* data_ = stbi_load(filepaths[i].c_str(), &width_, &height_, &channels_, 0);
            if (width_ != width || height_ != height || channels_ != channels) {
                std::cerr << "Texture array loading failed: " << filepaths[i] << " has different size or channels from " << filepaths[0] << std::endl;
                stbi_image_free(data_);
                return nullptr;
            }
            
            std::copy(data_, data_ + image_size, data.data() + i * image_size);
            stbi_image_free(data_);
        }

        std::cout << "Loaded texture array: " << name << " (" << width << "x" << height << ", " << channels << " channels, " << depth << " layers)" << std::endl;

        bool success = texture->Load(data.data(), width, height, channels, depth);
        if (success) {
            textures_.emplace(name, texture);
            return texture;
        }

        return nullptr;
    }

    // Clear texture cache
    void clearCache() {
        textures_.clear();
    }

    // Clear texture manager
    void clear() {
        clearCache();
        default_texture_.reset();
    }

    // Check if texture is default texture
    bool isDefault(const std::shared_ptr<const Texture>& texture) const {
        if (texture == nullptr) {
            return false;
        }
        return texture == default_texture_;
    }

    // Get default texture
    std::shared_ptr<const Texture> getDefaultTexture() const {
        return default_texture_;
    }

private:
    bool alloc_gpu_ { true };

    std::unordered_map<std::string, std::shared_ptr<Texture>> textures_;

    std::shared_ptr<Texture> default_texture_ { nullptr };
};
