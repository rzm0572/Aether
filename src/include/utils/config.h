#pragma once

#include "utils/path_handler.h"
#include <string>
class Config {
public:
    Config() {}
    Config(const std::string& config_dir) {
        load(config_dir);
    }

    void load(const std::string& config_dir) {
        // TODO: load configuration from config_dir
    }

    void save(const std::string& config_dir) {
        // TODO: save configuration to config_dir
    }

    void init(const std::string& config_dir = getConfigPath("")) {
        load(config_dir);
    }

    void clear() {}

public:
    // Default values
    unsigned int scr_width = 800;
    unsigned int scr_height = 600;
    float z_near = 0.1f;
    float z_far = 400.0f;
    bool debug_mode = false;
};
