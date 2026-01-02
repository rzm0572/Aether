#pragma once

#include "utils/macros.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <chrono>
#include <ostream>
#include <unordered_map>
#include <string>
#include <iostream>

struct Timer {
    std::string name;
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
    std::chrono::time_point<std::chrono::high_resolution_clock> end;
    std::chrono::duration<double> duration;
    unsigned int count = 0;

    Timer(const std::string& name = "") : name(name) {}

    void start_clock() {
        start = std::chrono::high_resolution_clock::now();
    }

    void end_clock() {
        end = std::chrono::high_resolution_clock::now();
        duration += end - start;
        count++;
    }

    double get_clock() {
        end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double>(end - start).count();
    }
};

class Profiler {
public:
    Profiler() {
        get_timer("global").start_clock();
    }

    static Profiler& instance() {
        static Profiler profiler;
        return profiler;
    }

    Timer& get_timer(const std::string& name) {
        if (timers_.find(name) == timers_.end()) {
            timers_[name] = Timer(name);
        }
        return timers_[name];
    }

    void report() {
        get_timer("global").end_clock();
        double total_time = get_timer("global").duration.count();

        std::cout << "Profiler report:" << std::endl;
        std::vector<Timer> sorted_timers;
        for (auto& pair : timers_) {
            sorted_timers.push_back(pair.second);
        }
        std::sort(sorted_timers.begin(), sorted_timers.end(), [](const Timer& a, const Timer& b) {
            return a.duration.count() > b.duration.count();
        });

        for (auto& timer : sorted_timers) {
            std::cout << GREEN << timer.name << RESET << ":(" CYAN << timer.duration.count() / total_time * 100 << "%" RESET ") " CYAN << timer.duration.count() << RESET " s in total, " CYAN << timer.duration.count() / timer.count << RESET " s in average, called " CYAN << timer.count << RESET " times" << std::endl;
        }
    }

private:
    std::unordered_map<std::string, Timer> timers_;
};

inline std::ostream& operator<<(std::ostream& os, const glm::vec2& vec) {
    os << "(" << vec.x << ", " << vec.y << ")";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const glm::vec3& vec) {
    os << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const glm::vec4& vec) {
    os << "(" << vec.x << ", " << vec.y << ", " << vec.z << ", " << vec.w << ")";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const glm::mat4& mat) {
    std::string indent = "[";
    for (int i = 0; i < 4; i++) {
        indent += "[ ";
        for (int j = 0; j < 4; j++) {
            indent += std::to_string(mat[i][j]) + " ";
        }
        indent = indent + "]" + (i == 3 ? "" : "\n");
    }
    indent = indent + "]";
    os << indent;
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const glm::quat& q) {
    os << "(" << q.x << ", " << q.y << ", " << q.z << ", " << q.w << ")";
    return os;
}
