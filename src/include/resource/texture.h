#pragma once

#include "utils/path_handler.h"
#include <string>
#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glad/glad.h>
#include <stb_image.h>
#include <unordered_map>

// Texture class
class Texture {
    friend class TextureManager;

public:
    // 构造函数：指定纹理类型（通常是 GL_TEXTURE_2D）和文件路径
    Texture(GLenum type, const std::string& filepath)
        : m_type(type), m_filepath(filepath), m_textureID(0), m_width(0), m_height(0), m_channels(0) {}
    
    ~Texture(){
        if (m_textureID) {
            glDeleteTextures(1, &m_textureID);
        }
    }

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&& other) {
        m_type = other.m_type;
        m_filepath = std::move(other.m_filepath);
        m_textureID = other.m_textureID;
        m_width = other.m_width;
        m_height = other.m_height;
        m_channels = other.m_channels;

        other.m_textureID = 0;
    }

    Texture& operator=(Texture&& other) {
        if (this != &other) {
            if (m_textureID) {
                glDeleteTextures(1, &m_textureID);
            }

            m_type = other.m_type;
            m_filepath = std::move(other.m_filepath);
            m_textureID = other.m_textureID;
            m_width = other.m_width;
            m_height = other.m_height;
            m_channels = other.m_channels;

            other.m_textureID = 0;
        }
        return *this;
    }

    // Load texture from file
    // Return true if successful, false otherwise
    bool Load(){
        // 加载图像数据
        stbi_set_flip_vertically_on_load(true); // OpenGL 原点在左下，需翻转
        unsigned char* data = stbi_load(m_filepath.c_str(), &m_width, &m_height, &m_channels, 0);

        if (!data) {
            std::cerr << "Failed to load texture: " << m_filepath << std::endl;
            return false;
        }

        // 确定格式
        GLenum format;
        if (m_channels == 1)
            format = GL_RED;
        else if (m_channels == 3)
            format = GL_RGB;
        else if (m_channels == 4)
            format = GL_RGBA;
        else {
            std::cerr << "Unsupported number of channels: " << m_channels << " in " << m_filepath << std::endl;
            stbi_image_free(data);
            return false;
        }

        // 生成并配置纹理
        glGenTextures(1, &m_textureID);
        glBindTexture(m_type, m_textureID);

        glTexParameteri(m_type, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(m_type, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(m_type, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(m_type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(m_type, 0, format, m_width, m_height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(m_type);

        stbi_image_free(data);
        glBindTexture(m_type, 0);

        std::cout << "Loaded texture: " << m_filepath << " (" << m_width << "x" << m_height << ", " << m_channels << " channels)" << std::endl;
        return true;
    }
    // 绑定纹理到指定纹理单元（如 GL_TEXTURE0）
    void Bind(GLenum textureUnit = GL_TEXTURE0) const {
        glActiveTexture(textureUnit);
        glBindTexture(m_type, m_textureID);
    }

    // 获取 OpenGL 纹理 ID（用于调试或高级用途）
    GLuint getID() const { return m_textureID; }

    const std::string toString() const {
        return "Texture(type: " + std::to_string(m_type) + ", filepath: " + m_filepath + ", textureID: " + std::to_string(m_textureID) + ", width: " + std::to_string(m_width) + ", height: " + std::to_string(m_height) + ", channels: " + std::to_string(m_channels) + ")";
    }

private:
    GLenum m_type;                        // 纹理类型
    std::string m_filepath;               // 纹理文件路径
    GLuint m_textureID;                   // OpenGL 纹理 ID
    int m_width, m_height, m_channels;    // 图像宽度、高度、通道数
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
    void init() {
        default_texture_ = std::make_shared<Texture>(GL_TEXTURE_2D, getAssetPath("textures/white.png"));
        default_texture_->Load();
    }

    // Get texture by filepath
    // If texture is not in cache, load it from file and add it to cache
    // If loading fails, return default texture
    std::shared_ptr<const Texture> getTexture(const std::string& filepath, GLenum type = GL_TEXTURE_2D) {
        // Check if texture is already in cache
        auto it = textures_.find(filepath);
        if (it != textures_.end()) {
            return it->second;
        }

        // Load texture from file and add it to cache
        textures_.emplace(filepath, std::make_shared<Texture>(type, filepath));
        auto texture = textures_.at(filepath);
        bool success = texture->Load();

        // If loading fails, remove texture from cache and return default texture
        if (!success) {
            textures_.erase(filepath);
            return default_texture_;
        }

        return texture;
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

    std::unordered_map<std::string, std::shared_ptr<Texture>> textures_;

    std::shared_ptr<Texture> default_texture_ { nullptr };
};
