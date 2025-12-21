#include "common/game_object.h"
#include "common/renderer.h"
#include "resource/shader.h"
#include "service/service_locator.h"
#include "utils/config.h"
#include "utils/macros.h"
#include "utils/path_handler.h"
#include "utils/profiler.h"
#include "common/window.h"
#include "world/skybox.h"
#include "interaction/input.h"
#include "interaction/camera.h"
#include "common/engine.h"
#include "world/terrain.h"
#include "entity/plane.h"
#include "resource/particles.h"

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




// 主要参考了 一步步学OpenGL(22) -《OpenGL使用Assimp库导入3d模型》 - Kam92.J的文章 - 知乎 https://zhuanlan.zhihu.com/p/150570465
// 修改了片段着色器的输入，使其能够接受纯色输入，否则会失去颜色，这是模型常用的做法即纯色模型加上细节贴图

// TODO: 不透明度（Opacity）
// TODO: 自发光（Emission）

// TODO: 法线贴图（Normal Map）
// TODO: 材质捕捉（Material Capture）


// TODO: 线框调试
// TODO: 顶点法线


int main() {
    Config config;
    ServiceLocator<Config>::provide(&config);

    if (config.debug_mode) {
        config.output();
    }

    // Window initialization
    Window window(config.scr_width, config.scr_height, "Aether");

    window.setFramebufferSizeCallback([](GLFWwindow* window, int width, int height) {
        glViewport(0, 0, width, height);
    });

    window.makeCurrent();

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Engine initialization
    // Service initialized and provided to ServiceLocator
    GameEngine* engine = new GameEngine(config);

    // Input initialization
    Input input;

    window.setWindowUserPointer(&input);
    window.setKeyCallback(input.keyCallback);
    window.setCursorPosCallback(input.mouseCallback);

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)config.scr_width / (float)config.scr_height, config.z_near, config.z_far);
    
    // 创建天空盒
    // TODO: 使用合适的图片作为天空盒
    Skybox skybox(
        getAssetPath("skybox/skybox1/right.jpg"), 
        getAssetPath("skybox/skybox1/left.jpg"),
        getAssetPath("skybox/skybox1/top.jpg"),   
        getAssetPath("skybox/skybox1/bottom.jpg"),
        getAssetPath("skybox/skybox1/front.jpg"), 
        getAssetPath("skybox/skybox1/back.jpg")
    );

    auto shader_manager = ServiceLocator<ShaderManager>::get();
    assert(shader_manager);
    shader_manager->registerShader("model", getShaderPath("models.vert"), getShaderPath("models.frag"));
    shader_manager->registerShader("depth", getShaderPath("depth.vert"), getShaderPath("depth.frag"));// 深度渲染着色器

    Renderer renderer;

    // Load models
    Model plane_model;
    if (!plane_model.loadModel(getAssetPath("models/j10/scene.gltf"))) {
        std::cerr << "Failed to load model!" << std::endl;
        return -1;
    }

    // GameObject* plane = GameObject::createFromModel(plane_model);
    glm::vec3 initial_position = glm::vec3(0.0f, 64.0f, 0.0f);
    glm::quat initial_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 velocity = glm::vec3(70.0f, 0.0f, 0.0f);
    glm::vec3 angular_velocity = glm::vec3(0.0f, 0.0f, 0.0f);

    Plane* plane = new Plane(input, plane_model, initial_position, initial_rotation, velocity, angular_velocity);
    

    // Terrain generation
    // PerlinGenerator perlin_generator(-10.0f, 10.0f, 16, 1);
    // Terrain terrain(perlin_generator);
    fBmGenerator fBm_generator(0.0f, 64.0f, 5, 2, 0.6f, -4, 16, 1);
    Terrain terrain(fBm_generator);

    unsigned int terrain_material_index = terrain.createMaterial(getAssetPath("textures/grass_2k/Poliigon_GrassPatchyGround_4585_BaseColor.jpg"));
    terrain.createChunks(0, 31, -2, 2, 4.0f, terrain_material_index);
    GameObject* terrain_obj = GameObject::createFromModel(terrain);

    // std::cout << terrain.toString() << std::endl;
    // terrain.outputModelTree();
    // std::cout << "Model loaded: " << plane_model.toString() << std::endl;
    // plane_model.outputModelTree();

    // Camera settings
    FreeCamera free_camera(
        glm::vec3(0.0f, 10.0f, 3.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        0.0f, 0.0f,
        25.0f, 0.06f
    );
    FreeCameraInputTranslator translator(input);

    // glm::quat base_rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    ThirdPersonCamera third_person_camera(plane, 16.0f, 0.0f, 90.0f, 10.0f, 0.06f, glm::vec3(0.5f, 0.0f, 0.0f));

    // Light settings
    auto light = Light(
        &third_person_camera,
        glm::vec3(0.8f, 0.8f, 0.8f),     // TODO: 环境光颜色有待实现，暂时用天蓝色代替
        {
            glm::vec3(0.0f, 1.0f, 1.0f),      // TODO: 光照方向有待实现，暂时用物体指向天空
            glm::vec3(2.5f, 2.5f, 2.5f),    // TODO:光照颜色有待实现，暂时用白色代替
        },
        input
    );

    // GPU 粒子的粒子效果测试
    Particle_Fireball fireball(10000,42,getAssetPath("textures/particles/particle_generated.png"));
    fireball.start_();
    Particle_Flareback flareback(10000, 42, getAssetPath("textures/particles/particle_generated.png"),glm::vec3(0.0f, 0.0f, 0.0f),1.0f);
    flareback.start_();
    Particle_Explosion explosion(100000,5000, 42, getAssetPath("textures/particles/particle_generated.png"));
    explosion.start_();
    Particle_Ribbon ribbon(1000, getAssetPath("textures/particles/particle_generated.png"));
    ribbon.start_();

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

        float dt = curr_frame - last_frame;

        // Logical frame
        Profiler::instance().get_timer("logical").start_clock();
        plane->update(dt);

        // free_camera.update(translator, curr_frame - last_frame);
        third_person_camera.update(input.getMouseMovement(), dt);
        light.update(dt);

        // glm::mat4 view = free_camera.getViewMatrix();
        glm::mat4 view = third_person_camera.getViewMatrix();

        delta_time_sum += dt;
        frame_count++;

        input.endUpdate();
        Profiler::instance().get_timer("logical").end_clock();

        // Render frame
        // TODO: 逻辑帧与渲染帧分离，渲染采用插值算法，提高帧率
        Profiler::instance().get_timer("render").start_clock();
        
        // --- Submissions ---
        renderer.submit_recursive(plane);
        renderer.submit_recursive(terrain_obj);
        renderer.finishAllSubmissions();                // Sort render queue
        
        // --- Render Pass ---
        renderer.renderShadowMap(light, window);     // Shadow map creation
        renderer.render(view, projection, light);
        renderer.finishAllRender();

        // 预览模型
        // model_test.render(model, view, projection, light);
        // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // 显示线框

        // 绘制爆炸的粒子效果
        // fireball.draw(plane->getTransformComponent().getPosition(),view, projection, third_person_camera.getPosition(),3.0f,3.0f,0.1f);
        flareback.draw(plane->getTransformComponent().getPosition()-glm::vec3(3.0f,1.0f,0.0f), view, projection, third_person_camera.getPosition(), 14.0f, 1.5f, 0.1f, plane->physical_component().GetForward());
        // explosion.draw(plane->getTransformComponent().getPosition(), view, projection, third_person_camera.getPosition(), 3.0f,3.0f,0.1f);
        ribbon.addParticles(plane->getTransformComponent().getPosition(),plane->physical_component().GetVelocity(), 20); // 每帧发射6个粒子
        ribbon.draw(view, projection, third_person_camera.getPosition(),10.0f,0.1f,0.4f,0.3f);

        // 渲染天空盒（在其他物体之后渲染以优化性能）
        skybox.changeProjection(projection);
        skybox.changeView(view);
        skybox.Render(light.getSkyTint());

        Profiler::instance().get_timer("render").end_clock();

        Profiler::instance().get_timer("swap").start_clock();
        window.swapBuffers();
        Profiler::instance().get_timer("swap").end_clock();

        last_frame = curr_frame;
        curr_frame = glfwGetTime();
    }

    if (config.debug_mode) {
        std::cout << "Average frame time: " CYAN << delta_time_sum / frame_count << " s" RESET << std::endl;
        std::cout << "Average FPS: " CYAN << 1.0f / (delta_time_sum / frame_count) << RESET << std::endl;
        Profiler::instance().report();
    }

    delete engine;

    return 0;
}