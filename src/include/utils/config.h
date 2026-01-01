#pragma once

#include "utils/path_handler.h"
#include <string>
#include <iostream>

/**
 * @brief Configuration class
 * 
 * This class stores global settings for the application.
 * Settings can be loaded and saved from/to a file.
 * If not loaded, default values will be used.
 */
class Config {
public:
    /**
     * @brief Construct a new Config object
     * 
     * @param config_dir The directory path where the configuration file is located. 
     *                   Defaults to the path returned by getConfigPath("").
     */
    Config(const std::string& config_dir = getConfigPath("")) {
        load(config_dir);
    }

    /**
     * @brief Load configuration settings from a file.
     * 
     * @param config_dir The directory path containing the configuration file.
     */
    void load(const std::string& config_dir) {
        // TODO: load configuration from config_dir
    }

    /**
     * @brief Save current configuration settings to a file.
     * 
     * @param config_dir The directory path where the configuration file will be saved.
     */
    void save(const std::string& config_dir) {
        // TODO: save configuration to config_dir
    }

    /**
     * @brief Output the current configuration settings to the console.
     */
    void output() const {
        std::cout << "Config:" << std::endl;
        std::cout << "  Screen width: " << scr_width << std::endl;
        std::cout << "  Screen height: " << scr_height << std::endl;
        std::cout << "  Z-near: " << z_near << std::endl;
        std::cout << "  Z-far: " << z_far << std::endl;
        std::cout << "  Debug mode: " << debug_mode << std::endl;
        std::cout << "  GPU memory allocation: " << alloc_gpu << std::endl;
    }

public:
    // Default values
    unsigned int scr_width = 800;  ///< Width of the screen in pixels
    unsigned int scr_height = 600; ///< Height of the screen in pixels
    float z_near = 0.1f;           ///< Near clipping plane distance
    float z_far = 500.0f;          ///< Far clipping plane distance
    bool debug_mode = true;        ///< Flag to enable debug mode
    bool alloc_gpu = true;         ///< Flag to enable GPU memory allocation
};
