#include "common/game_object.h"
#include "common/renderer.h"
#include "resource/shader.h"
#include "service/service_locator.h"
#include "utils/macros.h"
#include "utils/path_handler.h"
#include "utils/profiler.h"
#include "common/window.h"
#include "world/skybox.h"
#include "interaction/input.h"
#include "interaction/camera.h"
#include "common/engine.h"

#include <iostream>
#include <string>
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <stb_image.h>


// TODO: 注释

// 主要参考了 一步步学OpenGL(22) -《OpenGL使用Assimp库导入3d模型》 - Kam92.J的文章 - 知乎 https://zhuanlan.zhihu.com/p/150570465
// 修改了片段着色器的输入，使其能够接受纯色输入，否则会失去颜色，这是模型常用的做法即纯色模型加上细节贴图
// TODO: 金属度贴图（Metalness）
// TODO: 粗糙度贴图（Roughness）
// TODO: 不透明度（Opacity）
// TODO: 自发光（Emission）
// TODO: 法线贴图（Normal Map）
// TODO: 材质捕捉（Material Capture）
// TODO: 自阴影（Self-shadowing）

// TODO: 线框调试
// TODO: 顶点法线


int main() {
    const unsigned int SCR_WIDTH = 800;
    const unsigned int SCR_HEIGHT = 600;

    Window window(SCR_WIDTH, SCR_HEIGHT, "Main");

    window.setFramebufferSizeCallback([](GLFWwindow* window, int width, int height) {
        glViewport(0, 0, width, height);
    });

    window.makeCurrent();

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    GameEngine* engine = new GameEngine();

    Input input;

    window.setWindowUserPointer(&input);
    window.setKeyCallback(input.keyCallback);
    window.setCursorPosCallback(input.mouseCallback);

    FreeCamera camera(
        glm::vec3(0.0f, 0.0f, 3.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        0.0f, 0.0f,
        5.0f, 0.06f
    );
    FreeCameraInputTranslator translator(input);


    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = glm::mat4(1.0f);
    
    // 创建天空盒
    // TODO: 使用合适的图片作为天空盒
    Skybox skybox(
        getAssetPath("skybox/right.jpg"), 
        getAssetPath("skybox/left.jpg"),
        getAssetPath("skybox/top.jpg"),   
        getAssetPath("skybox/bottom.jpg"),
        getAssetPath("skybox/front.jpg"), 
        getAssetPath("skybox/back.jpg")
    );

    auto shader_manager = ServiceLocator<ShaderManager>::get();
    assert(shader_manager);
    shader_manager->registerShader("model", getShaderPath("models.vert"), getShaderPath("models.frag"));

    Renderer renderer;

    // 加载模型
    Model model_test;
    if (!model_test.loadModel(getAssetPath("models/j10/scene.gltf"))) {
        std::cerr << "Failed to load model!" << std::endl;
        return -1;
    }

    // Create a demo plane
    GameObject* plane = GameObject::createFromModel(model_test);

    // Light settings
    auto light = Light(
        &camera,
        glm::vec3(0.5f, 0.75f, 1.0f),     // TODO: 环境光颜色有待实现，暂时用天蓝色代替
        {
            glm::vec3(0.0f, 1.0f, 1.0f),      // TODO: 光照方向有待实现，暂时用物体指向天空
            glm::vec3(1.0f, 1.0f, 1.0f),    // TODO：光照颜色有待实现，暂时用白色代替
        }
    );

    // std::cout << "Model loaded: " << model_test.toString() << std::endl;
    // std::cout << model_test.outputModelTree();

    // 开启深度测试
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    float last_frame = 0.0f;
    float curr_frame = 0.0f;

    float delta_time_sum = 0.0f;
    int frame_count = 0;

    // Game loop
    while (!window.shouldClose()) {
        Profiler::instance().get_timer("io").start_clock();
        input.pollEvents();
        Profiler::instance().get_timer("io").end_clock();

        if (input.getKeyPressed(InputKey::ESC)) {
            window.setWindowShouldClose();
        }

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); 

        // Logical frame
        Profiler::instance().get_timer("logical").start_clock();
        camera.update(translator, curr_frame - last_frame);
        delta_time_sum += curr_frame - last_frame;
        frame_count++;

        view = camera.getViewMatrix();

        plane->getTransformComponent().translate(glm::vec3(0.04f, 0.0f, 0.0f));

        input.endUpdate();
        Profiler::instance().get_timer("logical").end_clock();

        // Render frame
        // TODO: 逻辑帧与渲染帧分离，渲染采用插值算法，提高帧率
        Profiler::instance().get_timer("render").start_clock();
        renderer.submit_recursive(plane);
        renderer.render(view, projection, light);

        // 预览模型
        // model_test.render(model, view, projection, light);
        // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // 显示线框

        // 渲染天空盒（在其他物体之后渲染以优化性能）
        skybox.changeProjection(projection);
        skybox.changeView(view);
        skybox.Render();

        Profiler::instance().get_timer("render").end_clock();

        Profiler::instance().get_timer("swap").start_clock();
        window.swapBuffers();
        Profiler::instance().get_timer("swap").end_clock();

        last_frame = curr_frame;
        curr_frame = glfwGetTime();
    }

    std::cout << "Average frame time: " CYAN << delta_time_sum / frame_count << " s" RESET << std::endl;
    std::cout << "Average FPS: " CYAN << 1.0f / (delta_time_sum / frame_count) << RESET << std::endl;
    Profiler::instance().report();

    return 0;
}