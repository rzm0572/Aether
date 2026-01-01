#include "common/game_object.h"
#include "common/renderer.h"
#include "entity/collision_config.h"
#include "resource/shader.h"
#include "service/service_locator.h"
#include "system/collision.h"
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
// #include "resource/particles.h"
#include "system/weapons.h"
#include "system/explosion.h"
#include "resource/healthbar.h"

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

glm::mat4 makeTransformMatrix(
    float tx, float ty, float tz,   // 平移
    float rx, float ry, float rz,   // 旋转（弧度，XYZ 顺序）
    float sx, float sy, float sz    // 缩放
) {
    glm::mat4 trans = glm::translate(glm::mat4(1.0f), glm::vec3(tx, ty, tz));
    rx = glm::radians(rx);
    ry = glm::radians(ry);
    rz = glm::radians(rz);
    glm::mat4 rot   = glm::rotate(glm::mat4(1.0f), rx, glm::vec3(1, 0, 0))
                    * glm::rotate(glm::mat4(1.0f), ry, glm::vec3(0, 1, 0))
                    * glm::rotate(glm::mat4(1.0f), rz, glm::vec3(0, 0, 1));
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(sx, sy, sz));

    // 注意顺序：M = T * R * S
    return trans * rot * scale;
}

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

    // ExplosionSystem explosion_system;
    auto* explosion_system = ServiceLocator<ExplosionSystem>::get();
    explosion_system->init();

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

    auto& collision_configs = CollisionConfigRegistry::getInstance();
    collision_configs.initialize();
    // collision_configs.output();

    // Load models
    Model plane_model;
    if (!plane_model.loadModel(getAssetPath("models/j10/scene.gltf"))) {
        std::cerr << "Failed to load model!" << std::endl;
        return -1;
    }

    // GameObject* plane = GameObject::createFromModel(plane_model);
    glm::vec3 initial_position = glm::vec3(0.0f, 192.0f, 0.0f);
    glm::quat initial_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); 
    glm::vec3 velocity = glm::vec3(70.0f, 0.0f, 0.0f);
    glm::vec3 angular_velocity = glm::vec3(0.0f, 0.0f, 0.0f);

    glm::vec3 initial_position2 = glm::vec3(200.0f,192.0f, 0.0f);

    Plane* plane = new Plane(Owner::PLAYER, input, plane_model, initial_position, initial_rotation, velocity, angular_velocity, collision_configs.registry["j-10"]);
    Plane *plane_enemy = new Plane(Owner::ENEMY, input, plane_model, initial_position2, initial_rotation, velocity, angular_velocity, collision_configs.registry["j-10"]);

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
        // &free_camera,
        glm::vec3(0.8f, 0.8f, 0.8f),
        {
            glm::vec3(0.0f, 1.0f, 1.0f),
            glm::vec3(2.5f, 2.5f, 2.5f),   
        },
        input
    );

    Weapons weapons_player(0, Owner::PLAYER, input,renderer);//玩家武器系统
    Weapons weapons_enemy(1, Owner::ENEMY, input,renderer);//敌人武器系统
    Particle_Ribbon ribbon1  = Particle_Ribbon(2000, getAssetPath("textures/particles/particle_generated.png"));
    ribbon1.start_();
    Particle_Ribbon ribbon2 = Particle_Ribbon(2000, getAssetPath("textures/particles/particle_generated.png"));
    ribbon2.start_();
    // Particle_Ribbon ribbon3 = Particle_Ribbon(2000, getAssetPath("textures/particles/particle_generated.png"));
    // ribbon3.start_();
    // Particle_Ribbon ribbon4 = Particle_Ribbon(2000, getAssetPath("textures/particles/particle_generated.png"));
    // ribbon4.start_();
    HealthBar health_bar_plane;
    HealthBar health_bar_enemy;

    auto* collision_system = ServiceLocator<CollisionSystem>::get();
    collision_system->setTerrainGenerator(&fBm_generator);

    // 开启深度测试
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    float last_frame = 0.0f;
    float curr_frame = 0.0f;

    float delta_time_sum = 0.0f;
    int frame_count = 0;
    // Particle_Explosion explosion(100000,5000, 42, getAssetPath("textures/particles/particle_generated.png"));
    // explosion.start_();
    // Game loop
    while (!window.shouldClose()) {
        Profiler::instance().get_timer("io").start_clock();
        input.pollEvents();
        Profiler::instance().get_timer("io").end_clock();

        if (input.getKeyPressed(InputKey::ESC)) {
            window.setWindowShouldClose();
        }

        // glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); 

        float dt = curr_frame - last_frame;

        // Logical frame
        Profiler::instance().get_timer("logical").start_clock();
        if (input.getKeyPressedDown(InputKey::RIGHT_BRACKET)) {
            explosion_system->addExplosion(plane_model, plane, curr_frame, 5.0f);
        }

        // Entity logical update
        plane->update(dt,0,glm::vec3(0.0f));
        plane_enemy->update(dt,1,glm::vec3(0.0f));
        collision_system->submit(plane);
        collision_system->submit(plane_enemy);

        auto& player_transform = plane->getTransformComponent();
        auto& player_physical = plane->physical_component();
        auto& enemy_tranform = plane_enemy->getTransformComponent();
        auto& enemy_physical = plane_enemy->physical_component();

        weapons_player.use(dt, curr_frame, player_transform.getPosition(),player_physical.getVelocity(),player_physical.getUp(),player_physical.getRight(),player_physical.GetForward(),player_physical.getRotation(),enemy_tranform.getPosition());
        weapons_enemy.use(dt, curr_frame, enemy_tranform.getPosition(),enemy_physical.getVelocity(),enemy_physical.getUp(),enemy_physical.getRight(),enemy_physical.GetForward(),enemy_physical.getRotation(),player_transform.getPosition());
        
        weapons_player.submitCollision();
        weapons_enemy.submitCollision();

        // collision_system->debugOutput();

        // Collision detection
        std::vector<DebugLine> render_lines;
        collision_system->generateDebugLines(render_lines);
        collision_system->update(curr_frame);

        // Collision handling
        plane->handleCollision();
        plane_enemy->handleCollision();
        weapons_player.handleCollision();
        weapons_enemy.handleCollision();

        // Explosion handling
        explosion_system->update(renderer, curr_frame);

        // Camera and light update
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

        if(!plane->getHealthComponent().isDead()){
            // ribbon1.addParticles(plane->getTransformComponent().getPosition()+plane->physical_component().getRight()*3.5f + plane->physical_component().GetForward() * 2.0f - plane->physical_component().getUp()*0.5f,plane->physical_component().getVelocity(), 6, 0.2f);
            // ribbon2.addParticles(plane->getTransformComponent().getPosition()-plane->physical_component().getRight()*3.5f + plane->physical_component().GetForward() * 2.0f - plane->physical_component().getUp()*0.5f,plane->physical_component().getVelocity(), 6, 0.2f);
            // // ribbon3.addParticles(plane_enemy->getTransformComponent().getPosition()+plane_enemy->physical_component().getRight()*2.0f,plane_enemy->physical_component().getVelocity(), 8, 0.1f);
            // // ribbon4.addParticles(plane_enemy->getTransformComponent().getPosition()-plane_enemy->physical_component().getRight()*2.0f,plane_enemy->physical_component().getVelocity(), 8, 0.1f);
            // ribbon1.draw(view, projection, third_person_camera.getPosition(),40.0f,0.05f,0.3f,0.1f);
            // ribbon2.draw(view, projection, third_person_camera.getPosition(),40.0f,0.05f,0.3f,0.1f);
        }
        // ribbon3.draw(view, projection, third_person_camera.getPosition(),40.0f,0.05f,0.3f,0.1f);
        // ribbon4.draw(view, projection, third_person_camera.getPosition(),40.0f,0.05f,0.3f,0.1f);
        // --- weapons update and submit ---
        glm::vec3 camera_pos = third_person_camera.getPosition();
        weapons_player.postProcess(dt, curr_frame, player_transform.getPosition(), enemy_tranform.getPosition(),
            player_physical.getUp(), player_physical.getRight(), player_physical.GetForward(), player_physical.getRotation(),
            camera_pos, view, projection, plane->getHealthComponent().isDead()
        );
        weapons_enemy.postProcess(dt, curr_frame, enemy_tranform.getPosition(), player_transform.getPosition(),
            enemy_physical.getUp(), enemy_physical.getRight(), enemy_physical.GetForward(), enemy_physical.getRotation(),
            camera_pos, view, projection, plane_enemy->getHealthComponent().isDead()
        );
        health_bar_plane.draw(plane->getTransformComponent().getPosition() + glm::vec3(0.0f, 2.5f, 0.0f),view,projection,third_person_camera.getPosition(),5.0f,0.25f,plane->getHealthComponent().getHealth());
        health_bar_enemy.draw(plane_enemy->getTransformComponent().getPosition() + glm::vec3(0.0f, 2.5f, 0.0f),view,projection,third_person_camera.getPosition(),5.0f,0.25f,plane_enemy->getHealthComponent().getHealth());
        renderCollisionBox(render_lines, view, projection);
        // --- Submissions ---
        renderer.submit_recursive(plane);
        renderer.submit_recursive(plane_enemy);
        renderer.submit_recursive(terrain_obj);
        renderer.finishAllSubmissions();                // Sort render queue
        
        // --- Render Pass ---
        renderer.renderShadowMap(light, window);     // Shadow map creation
        renderer.render(view, projection, light);
        renderer.finishAllRender();

        // 预览模型
        // model_test.render(model, view, projection, light);
        // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // 显示线框

        // explosion.draw(plane->getTransformComponent().getPosition(), view, projection, third_person_camera.getPosition(), 3.0f,3.0f,0.1f);
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

// TODO: 有空做个HDL/Bloom 的实现
// TODO: 有空做个不同魔法飞弹的爆炸效果
// TODO: 火焰
// TODO: 至少完成单一飞弹，曲线光线飞弹，火球飞弹 等效果