#pragma once

#include "utils/path_handler.h"
#include <string>
#include <iostream>

class Config {
public:
    Config(const std::string& config_dir = getConfigPath("")) {
        load(config_dir);
    }

    void load(const std::string& config_dir) {
        // TODO: load configuration from config_dir
    }

    void save(const std::string& config_dir) {
        // TODO: save configuration to config_dir
    }

    void output() const {
        std::cout << "Config:" << std::endl;
        std::cout << "  Screen width: " << scr_width << std::endl;
        std::cout << "  Screen height: " << scr_height << std::endl;
        std::cout << "  Z-near: " << z_near << std::endl;
        std::cout << "  Z-far: " << z_far << std::endl;
    }

public:
    // Default values
    unsigned int scr_width = 800;
    unsigned int scr_height = 600;
    float z_near = 0.1f;
    float z_far = 100.0f;
    bool debug_mode = true;
};
