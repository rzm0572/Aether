#pragma once

#include <filesystem>
#include <string>

#ifdef _WIN32
    #include <windows.h>
#elif defined(__linux__)
    #include <unistd.h>
    #include <limits.h>
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
    #include <glog/logging.h>
#endif

inline std::filesystem::path get_executable_path() {
    #ifdef _WIN32
        char path[MAX_PATH];
        GetModuleFileNameW(NULL, path, MAX_PATH);
        return std::filesystem::path(path).parent_path();
    #elif defined(__linux__)
        char path[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", path, PATH_MAX);
        if (len < 0) {
            return std::filesystem::path();
        }
        return std::filesystem::path(path).parent_path();
    #elif defined(__APPLE__)
        char path[PATH_MAX];
        uint32_t size = sizeof(path);
        if (_NSGetExecutablePath(path, &size) != 0) {
            LOG(ERROR) << "Failed to get executable path";
            return std::filesystem::path();
        }
        return std::filesystem::path(path).parent_path();
    #else
        static_assert(false, "Unsupported platform");
    #endif
}

#define GENERATE_GET_RESOURCE_PATH_FUNC(func_name, resource_subdir) \
    inline std::string func_name(std::string resource) { \
        std::filesystem::path executable_path = get_executable_path(); \
        return (executable_path / resource_subdir / resource).string(); \
    }

GENERATE_GET_RESOURCE_PATH_FUNC(get_shader_path, "shaders");
GENERATE_GET_RESOURCE_PATH_FUNC(get_asset_path, "assets");
GENERATE_GET_RESOURCE_PATH_FUNC(get_config_path, "configs");
