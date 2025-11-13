set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
            ${EXECUTABLE_NAME}
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
        PRIVATE ${GLAD_DIR}/include
    )
endfunction()

# -------------------------------------------------------------------------------
# Set library sources here

set(
    AETHER_LIBS
    lib/shader.cpp
)

# -------------------------------------------------------------------------------
# Add executables here

add_opengl_executable(test_opengl "test.cpp")
add_opengl_executable(retangle "${AETHER_LIBS};retangle.cpp")

# -------------------------------------------------------------------------------
# Install executables here

install(
    TARGETS test_opengl retangle
    RUNTIME DESTINATION .
)
