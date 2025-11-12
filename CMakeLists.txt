cmake_minimum_required(VERSION 3.29)

project(AETHER)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Check system
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(SYSTEM_IS_MACOS TRUE)
else()
    set(SYSTEM_IS_MACOS FALSE)
endif()

message(STATUS "System: ${CMAKE_SYSTEM_NAME}")

# Directory where submodules are stored
set(REPO_DIR "${CMAKE_SOURCE_DIR}/repo")

# Directory where dependencies are stored
set(DEPENDENCIES_DIR "${CMAKE_SOURCE_DIR}/dependencies")

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

if (NOT MINGW)
    # glog 0.7.1
    find_package(GLOG QUIET)
    if (NOT GLOG_FOUND)
        message(STATUS "GLOG not found. Building from submodule 'repo/glog' (#v0.7.1) ...")
        add_subdirectory(repo/glog EXCLUDE_FROM_ALL)
    else()
        message(STATUS "GLOG found: ${GLOG_LIBRARIES}")
    endif()

    # # googletest 1.17.0
    set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
    set(INSTALL_GTEST OFF CACHE INTERNAL "")
    add_subdirectory(repo/googletest EXCLUDE_FROM_ALL)
endif()


# Function for adding OpenGL executable
function(add_opengl_executable EXECUTABLE_NAME CUSTOM_SOURCE)
    message("Adding OpenGL executable: ${EXECUTABLE_NAME} from ${CUSTOM_SOURCE}")

    add_executable(${EXECUTABLE_NAME} ${CUSTOM_SOURCE})

    if (SYSTEM_IS_MACOS)
        target_link_libraries(
            ${EXECUTABLE_NAME}
            PRIVATE ${COCOA_LIBRARY}
            PRIVATE ${IO_KIT_LIBRARY}
        )
    endif()

    if (NOT MINGW)
        target_link_libraries(
            ${EXECUTEABLE_NAME}
            PRIVATE GTest::gtest GTest::gtest_main
            PRIVATE glog::glog
        )
    endif()

    target_link_libraries(
        ${EXECUTABLE_NAME}
        PRIVATE OpenGL::GL
        PRIVATE glfw
        PRIVATE glad
        PRIVATE glm::glm-header-only
        PRIVATE ${CMAKE_DL_LIBS}
    )

    target_include_directories(
        ${EXECUTABLE_NAME}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE ${OPENGL_INCLUDE_DIRS}
        PRIVATE ${GLFW3_INCLUDE_DIRS}
        PRIVATE ${GLAD_DIR}/include
    )
endfunction()

# -------------------------------------------------------------------------------
# Add executables here

add_opengl_executable(test_opengl "src/test.cpp")

