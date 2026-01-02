cmake_minimum_required(VERSION 3.29)

project(AETHER)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -Wall")
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/bin")

# Check system
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(SYSTEM_IS_MACOS TRUE)
else()
    set(SYSTEM_IS_MACOS FALSE)
endif()

message(STATUS "System: ${CMAKE_SYSTEM_NAME}")

# Directory where submodules are stored
set(REPO_DIR "${CMAKE_SOURCE_DIR}/repo")

# Directory where resources are stored
set(RESOURCE_DIR "${CMAKE_SOURCE_DIR}/resources")

# Find dependencies
if (SYSTEM_IS_MACOS)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(IO_KIT_LIBRARY IOKit REQUIRED)
    message(STATUS "COCOA_LIBRARY found: ${COCOA_LIBRARY}")
    message(STATUS "IO_KIT_LIBRARY found: ${IO_KIT_LIBRARY}")
endif()

find_package(OpenGL REQUIRED)
message(STATUS "OpenGL found: ${OPENGL_LIBRARY}")

# Build dependencies
# GLAD
set(GLAD_DIR "${REPO_DIR}/glad")
add_library(glad ${GLAD_DIR}/src/glad.c)
target_include_directories(glad PRIVATE ${GLAD_DIR}/include)

# GLFW 3.4
find_package(glfw3 QUIET)
if (NOT glfw3_FOUND)
    message(STATUS "glfw3 not found. Building from submodule 'repo/glfw' (#3.4) ...")
    set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "")
    set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "")
    set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "")
    add_subdirectory(repo/glfw EXCLUDE_FROM_ALL)
else()
    message(STATUS "glfw3 found: ${GLFW3_LIBRARIES}")
endif()

# GLM 1.0.2
find_package(GLM QUIET CONFIG)
if (NOT GLM_FOUND)
    message(STATUS "GLM not found. Building from submodule 'repo/glm' (#1.0.2) ...")
    set(GLM_TESTS OFF CACHE INTERNAL "")
    add_subdirectory(repo/glm EXCLUDE_FROM_ALL)
else()
    message(STATUS "GLM found: ${GLM_INCLUDE_DIRS}")
endif()

# STB 
set(STB_INCLUDE_DIR "${REPO_DIR}/stb")
if (NOT EXISTS "${STB_INCLUDE_DIR}/stb_image.h")
    message(FATAL_ERROR "stb_image.h not found in ${STB_INCLUDE_DIR}. Please run: git submodule update --init")
endif()
message(STATUS "STB headers found at: ${STB_INCLUDE_DIR}")

# Assimp
find_package(assimp QUIET CONFIG)
if (NOT assimp_FOUND)
    message(STATUS "Assimp not found. Building from submodule 'repo/assimp' ...")
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
    set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE BOOL "" FORCE)
    add_subdirectory(repo/assimp EXCLUDE_FROM_ALL)

    # 设置变量供后续使用
    set(ASSIMP_LIBRARIES assimp::assimp)
else()
    message(STATUS "Assimp found: ${assimp_LIBRARIES} (version ${assimp_VERSION})")
    set(ASSIMP_LIBRARIES assimp::assimp)
endif()

if (NOT MINGW)
    # glog 0.7.1
    find_package(glog QUIET)
    if (NOT glog_FOUND)
        message(STATUS "glog not found. Building from submodule 'repo/glog' (#v0.7.1) ...")
        add_subdirectory(repo/glog EXCLUDE_FROM_ALL)
    else()
        message(STATUS "glog found: ${GLOG_LIBRARIES}")
    endif()

    # # googletest 1.17.0
    set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
    set(INSTALL_GTEST OFF CACHE INTERNAL "")
    add_subdirectory(repo/googletest EXCLUDE_FROM_ALL)
endif()

# JsonCpp
if (NOT EXISTS "${REPO_DIR}/jsoncpp/include/json/json.h")
    message(FATAL_ERROR "JsonCpp headers missing! Run: git submodule update --init")
endif()
add_definitions(-DJSONCPP_USE_STRING_VIEW=0)
set(JSONCPP_USE_STRING_VIEW OFF CACHE BOOL "Disable string_view in JsonCpp" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(JSONCPP_WITH_TESTS OFF CACHE INTERNAL "")
set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE INTERNAL "")
set(JSONCPP_WITH_PKGCONFIG_SUPPORT OFF CACHE INTERNAL "")
add_subdirectory(${REPO_DIR}/jsoncpp EXCLUDE_FROM_ALL)
if(TARGET jsoncpp_static)
    set(JSONCPP_LIBRARIES jsoncpp_static)
elseif(TARGET jsoncpp_lib)
    set(JSONCPP_LIBRARIES jsoncpp_lib)
else()
    message(FATAL_ERROR "JsonCpp: Neither 'jsoncpp_static' nor 'jsoncpp_lib' target found!")
endif()

set(JSONCPP_INCLUDE_DIRS ${REPO_DIR}/jsoncpp/include)
message(STATUS "Using JsonCpp library: ${JSONCPP_LIBRARIES}")

message(STATUS "custom include: ${CMAKE_CURRENT_SOURCE_DIR}/include")
message(STATUS "OpenGL_INCLUDE_DIRS: ${OPENGL_INCLUDE_DIRS}")

add_subdirectory(src)

install(
    DIRECTORY ${RESOURCE_DIR}/assets ${RESOURCE_DIR}/shaders ${RESOURCE_DIR}/configs
    DESTINATION .
)
