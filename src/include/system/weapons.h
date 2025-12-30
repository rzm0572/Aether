#include "common/game_object.h"
#include "common/renderer.h"
#include "resource/shader.h"
#include "service/service_locator.h"
#include "utils/config.h"
#include "utils/macros.h"
#include "utils/path_handler.h"
#include "utils/profiler.h"
#include "interaction/input.h"
#include "interaction/camera.h"
#include "common/engine.h"
#include "entity/plane.h"
#include "resource/particles.h"
#include "system/bullet.h"

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
#include <vector>

class Weapons{
public:    
    Weapons(int kind,Input& input,Renderer& renderer): _kind(kind), _input(input), _last_time(glfwGetTime()), _renderer(renderer){
        srand(time(NULL));
        // 尾焰必须渲染
        
        flareback.start_();
        flareback_missle.start_();
        fireball.start_();
        autocannon.start_();
        tergeo.start_();
        kendavra.start_();

        // 空对空导弹模型导入
        if (!missle_model.loadModel(getAssetPath("models/missle1/scene.gltf"))) {
            std::cerr << "Failed to load model missle1!" << std::endl;
        }
        // 对地导弹模型导入
        if (!boom_model.loadModel(getAssetPath("models/boom/scene.gltf"))) {
            std::cerr << "Failed to load boom model!" << std::endl;
        }
        // 地对空导弹模型导入
        if (!missle_earth_model.loadModel(getAssetPath("models/missle2/scene.gltf"))) {
            std::cerr << "Failed to load surface to air missle model!" << std::endl;
        }


    }
    ~Weapons(){
        for(size_t i=0;i<missles.size();i++){
            delete missles[i];
        }
        missles.clear();
        for(size_t i=0;i<booms.size();i++){
            delete booms[i];
        }
        booms.clear();
        for(size_t i=0;i<missles_earth.size();i++){
            delete missles_earth[i];
        }
        missles_earth.clear();

    }
    void use(float dt,glm::vec3 pos,glm::vec3 Velocity,glm::vec3 up,glm::vec3 right,glm::vec3 forward,glm::quat rotation ,glm::vec3 target,glm::mat4 view,glm::mat4 projection,ThirdPersonCamera third_person_camera){
        // == 飞机自身的粒子 ==
        // 尾焰
        flareback.draw(pos, view, projection, third_person_camera.getPosition(), 6.0f, 1.0f, 0.1f, forward);

        // 拉烟
        // ribbon1.addParticles(pos + right * 1.5f,Velocity, 10, 0.1f); // 每帧发射10个粒子
        // ribbon2.addParticles(pos - right * 1.5f,Velocity, 10, 0.1f); // 每帧发射10个粒子
        // ribbon1.draw(view, projection, third_person_camera.getPosition(),40.0f,0.1f,0.4f,0.1f);
        // ribbon2.draw(view, projection, third_person_camera.getPosition(),40.0f,0.1f,0.4f,0.1f);
        // == 根据输入发射武器或者根据计时器自动发射 ==
        bool Fire = false;
        if(_kind == 0){// 手动发射
            if(glfwGetTime() - _last_time > 0.001f){// 间隔时间
                // std::cout<<"check time right "<<_last_time<<std::endl;
                // ++ 武器切换 ++
                if(_input.getKeyPressed(InputKey::H)){
                    // 空射导弹
                    weapon_set = 0;
                    weapon_kind = 0;
                }
                if(_input.getKeyPressed(InputKey::J)){
                    // 机炮+魔法
                    weapon_set = 1;
                    weapon_kind = 0;
                }
                if(_input.getKeyPressed(InputKey::K)){
                    // 火力支援
                    weapon_set = 2;
                    weapon_kind = 0;
                }
                if(_input.getKeyPressed(InputKey::L)){
                    // 主动防御
                    weapon_set = 3;
                    weapon_kind = 0;
                }
                if(_input.getKeyPressed(InputKey::U)){
                    switch(weapon_set){
                        case 0:
                            // 空射导弹
                            weapon_kind = (weapon_kind + 1)%weapon_num[0];
                        break;
                        case 1:
                            // 机炮+魔法
                            weapon_kind = (weapon_kind + 1)%weapon_num[1];
                        break;
                        case 2:
                            // 火力支援
                            weapon_kind = (weapon_kind + 1)%weapon_num[2];
                        break;
                        case 3:
                            // 主动防御
                            weapon_kind = (weapon_kind + 1)%weapon_num[3];
                        break;
                    }
                }
                // ++ 发射武器 ++
                if(_input.getKeyPressed(InputKey::Y)){
                    // std::cout<<"fire"<<std::endl;
                    Fire = true;
                }
                _last_time = glfwGetTime();
            }
            

        }
        else{ // 自动发射
            // if(glfwGetTime() - _last_time > 10.0f){
            //     // ++ 武器切换 ++
            //     weapon_set = rand()%4;
            //     weapon_kind = rand()%weapon_num[weapon_set];
            //     _last_time = glfwGetTime();
            // }
            // if(glfwGetTime() - _last_time_fire > 5.0f){
                // Fire = true;
            // }
        }
        // std::cout<<"weapon_set:"<<weapon_set<<" weapon_kind:"<<weapon_kind<<" fire: "<<Fire<<std::endl;
        // == 发射武器 ==
        if(Fire){            
            switch(weapon_set){
                case 0:
                    if(_last_time_fire + 0.7f < glfwGetTime()){
                        _last_time_fire = glfwGetTime();
                    // 空射导弹
                        if(weapon_kind == 0){

                            int slot = -1;
                            for(size_t i=0;i<missles.size();i++){
                                if(!missle_is_active[i]){
                                    slot = i;
                                    break;
                                }
                            }
                            if(slot != -1){
                                (missles[slot])->physical_component().initialize(pos - up * 1.5f, rotation, Velocity - up * 5.0f + 2.0f * forward, glm::vec3(0.0f,0.0f,0.0f));
                                missle_is_active[slot] = true;
                                missle_start_time[slot] = glfwGetTime();
                            }
                            else{
                                missles.push_back(new Plane(_input, missle_model, pos - up * 1.5f, rotation, Velocity - up * 5.0f + 2.0f * forward, glm::vec3(0.0f,0.0f,0.0f)));
                                missle_is_active.push_back(true);
                                missle_start_time.push_back(glfwGetTime());
                            }
                        }
                        else{// 航弹
                            int slot = -1;
                            for(size_t i=0;i<booms.size();i++){
                                if(!boom_is_active[i]){
                                    slot = i;
                                    break;
                                }
                            }
                            if(slot != -1){
                                (booms[slot])->physical_component().initialize(pos - up * 2.0f, rotation, Velocity - up * 5.0f, glm::vec3(0.0f,0.0f,0.0f));
                                boom_is_active[slot] = true;
                                boom_start_time[slot] = glfwGetTime();
                            }
                            else{
                                booms.push_back(new Plane(_input, boom_model, pos - up * 2.0f, rotation, Velocity - up * 5.0f, glm::vec3(0.0f,0.0f,0.0f)));
                                boom_is_active.push_back(true);
                                boom_start_time.push_back(glfwGetTime());
                            }
                        }
                    }
                break;
                case 1:
                    if(_last_time_fire + 0.1f < glfwGetTime()){
                        _last_time_fire = glfwGetTime();
                        // 机炮+魔法
                        if(weapon_kind == 0){// 火球
                            bullet_manager.fire(BulletType::FireBall, pos- up * 2.0f + forward * 6.0f, Velocity + 120.0f * forward, glfwGetTime());
                        }
                        else if(weapon_kind == 1){// 机炮
                            bullet_manager.fire(BulletType::Autocannon, pos- up * 2.0f + right * 1.0f + forward * 6.0f, Velocity + 120.0f * forward, glfwGetTime());
                            bullet_manager.fire(BulletType::Autocannon, pos- up * 2.0f - right * 1.0f + forward * 6.0f, Velocity + 120.0f * forward, glfwGetTime());
                        }
                        else if(weapon_kind == 2){// 旋风
                            bullet_manager.fire(BulletType::Tergeo, pos- up * 1.0f + forward * 8.0f, Velocity + 120.0f * forward, glfwGetTime());
                        }
                    }
                    if(_last_time_fire + 0.02f < glfwGetTime()){
                        _last_time_fire = glfwGetTime();
                        if(weapon_kind == 3){// 啃大瓜
                            float theta1 = static_cast<float>((rand()%20000-10000))/100000.0f;
                            float theta2 = static_cast<float>((rand()%20000-10000))/20000.0f*glm::pi<float>();
                            bullet_manager.fire(BulletType::Kendavra, pos- up * 2.0f + forward * 6.0f + right * (static_cast<float>((rand()%20000-10000)))/2500.0f, Velocity + 120.0f * forward * glm::cos(theta1) + 120.0f * right * glm::sin(theta1) * glm::cos(theta2) + 120.0f * up * glm::sin(theta1) * glm::sin(theta2), glfwGetTime());
                        }
                    }
                break;
                case 2:
                    // 火力支援
                    if(_last_time_help + 1.0f < glfwGetTime()){
                        _last_time_help = glfwGetTime();
                        if(weapon_kind == 0){// 地对空导弹
                            int slot = -1;
                            for(size_t i=0;i<missles_earth.size();i++){
                                if(!missle_earth_is_active[i]){
                                    slot = i;
                                    break;
                                }
                            }
                            glm::vec3 my_pos = glm::vec3(pos.x,100.0f,pos.z);
                            glm::vec3 my_velocity = target - my_pos;
                            my_velocity = glm::normalize(my_velocity);
                            my_velocity *= 20.0f;
                            glm::vec3 my_forward = glm::normalize(target - my_pos);
                            glm::vec3 defaultForward = glm::vec3(1.0f, 0.0f, 0.0f);
                            float dot = glm::dot(defaultForward, my_forward);
                            glm::quat my_quat;
                            // 处理同向情况（无需旋转）
                            if (dot > 0.99999f) {
                                my_quat=glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // 单位四元数
                            }
                            else if(dot < -0.99999f){
                                my_quat=glm::quat(0.0f, 0.0f, 1.0f, 0.0f);
                            }
                            else{
                                glm::vec3 axis = glm::cross(defaultForward, my_forward);
                                my_quat.w = 1.0f + dot;
                                my_quat.x = axis.x;
                                my_quat.y = axis.y;
                                my_quat.z = axis.z;
                            }

                            if(slot != -1){
                                (missles_earth[slot])->physical_component().initialize(my_pos,my_quat, my_velocity, glm::vec3(0.0f,0.0f,0.0f));
                                missle_earth_is_active[slot] = true;
                                missle_earth_start_time[slot] = glfwGetTime();
                            }
                            else{
                                missles_earth.push_back(new Plane(_input, missle_earth_model, my_pos,my_quat,my_velocity , glm::vec3(0.0f,0.0f,0.0f)));
                                missle_earth_is_active.push_back(true);
                                missle_earth_start_time.push_back(glfwGetTime());
                            }
                        }
                        else{// 空中魔法


                        }
                    }
                break;
                case 3:
                    // 主动防御
                    if(weapon_kind == 0){// 箔条

                    }
                    else{// 信号弹

                    }
                break;
            }
            
        }

        // == 绘制武器 ==
        // ++ 空空导弹 ++
        for(size_t i=0;i<missles.size();i++){
            if(missle_is_active[i]){
                if(missle_start_time[i] + _last_time_boom >= glfwGetTime()){
                    missles[i]->update(dt,2,target);
                    _renderer.submit_recursive(missles[i]);
                    flareback_missle.draw(missles[i]->getTransformComponent().getPosition(), view, projection, third_person_camera.getPosition(), 3.0f, 0.5f, 0.05f, missles[i]->physical_component().GetForward());
                }
                else{
                    missle_is_active[i] = false;
                    add_explode(missles[i]->getTransformComponent().getPosition());
                }
            }
        }
        // ++ 航弹 ++
        for(size_t i=0;i<booms.size();i++){
            if(boom_is_active[i]){
                if(boom_start_time[i] + _last_time_boom >= glfwGetTime()){
                    booms[i]->update(dt,3,target);
                    _renderer.submit_recursive(booms[i]);
                }
                else{
                    boom_is_active[i] = false;
                    add_explode(booms[i]->getTransformComponent().getPosition());
                    // std::cout<<"boom explode! at "<< booms[i]->getTransformComponent().getPosition()<<std::endl;
                }
            }
        }
        // ++ 地对空导弹 ++
        for(size_t i=0;i<missles_earth.size();i++){
            if(missle_earth_is_active[i]){
                if(missle_earth_start_time[i] + _last_time_boom >= glfwGetTime()){
                    missles_earth[i]->update(dt,2,target);
                    _renderer.submit_recursive(missles_earth[i]);
                    flareback_missle.draw(missles_earth[i]->getTransformComponent().getPosition(), view, projection, third_person_camera.getPosition(), 3.0f, 0.5f, 0.05f, missles_earth[i]->physical_component().GetForward());
                }
                else{
                    missle_earth_is_active[i] = false;
                    add_explode(missles_earth[i]->getTransformComponent().getPosition());
                }
            }
        }
        // std::cout<<"missles size:"<<missles.size()<<" booms size:"<<booms.size()<<" missles_earth size:"<<missles_earth.size()<<std::endl;
        // ++ 各种子弹 ++
        bullet_manager.update(dt);
        std::vector<glm::vec3> bullet_explod_positions = bullet_manager.cleanBullets(glfwGetTime());
        std::vector<Bullet>& bullets = bullet_manager.getBullets();
        for(size_t i=0;i<bullets.size();i++){
            if(bullets[i].type == BulletType::FireBall){
                // 火球
                fireball.draw(bullets[i].position, view, projection, third_person_camera.getPosition(), 0.8f, 0.5f, 0.05f);
            }
            else if(bullets[i].type == BulletType::Autocannon){
                // 机炮
                autocannon.draw(bullets[i].position,bullets[i].velocity, view, projection, third_person_camera.getPosition(), 8.0f, 0.4f, 0.03f,0.2f);
            }
            else if(bullets[i].type == BulletType::Tergeo){
                // 旋风
                // 随机一点转轴
                float theta1 = static_cast<float>((rand()%20000-10000))/100000.0f;
                float theta2 = static_cast<float>((rand()%20000-10000))/20000.0f*glm::pi<float>();
                tergeo.draw(bullets[i].position,up * glm::cos(theta1) + right * glm::sin(theta1) * glm::cos(theta2) + up * glm::sin(theta1) * glm::sin(theta2), view, projection, third_person_camera.getPosition(), 8.0f, 7.0f, 0.1f,3.0f,18.0f,glm::vec3(0.63f,0.81f,0.9f));
            }
            else if(bullets[i].type == BulletType::Kendavra){
                // 啃大瓜
                kendavra.draw(bullets[i].position,bullets[i].velocity, view, projection, third_person_camera.getPosition(), 0.0f, 0.4f, 0.5f,0.2f,9.0f,glm::vec3(0.33f,1.0f,0.5f));
            }
        }

        // == 爆炸 ==
        for(size_t i=0;i<bullet_explod_positions.size();i++){
            add_fireball_explode(bullet_explod_positions[i]);
        }
        for(size_t i=0;i<explosions.size();i++){
            if(explosions[i]->exists_now()){
                // std::cout<<"explosions position "<<explosion_positions[i] << "  pos "<<pos<<std::endl;
                // explosions[i]->draw(pos,view, projection, third_person_camera.getPosition(),15.0f,2.0f,0.05f);
                explosions[i]->draw(explosion_positions[i],view, projection, third_person_camera.getPosition(),15.0f,2.0f,0.1f);
            }
        }
        for(size_t i=0;i<fireball_explosions.size();i++){
            if(fireball_explosions[i]->exists_now()){
                fireball_explosions[i]->draw(fireball_explosion_positions[i],view, projection, third_person_camera.getPosition(),15.0f,2.0f,0.05f);
            }
        }
    }
private:
    Model missle_model;
    Model boom_model;
    Model missle_earth_model;
    std::vector<Plane*> missles,booms,missles_earth;
    BulletManager bullet_manager;
    std::vector<bool> missle_is_active,boom_is_active,missle_earth_is_active;
    std::vector<float> missle_start_time,boom_start_time,missle_earth_start_time;
    int _kind;// 是可以手动操纵-0/还是自动操纵-1
    int weapon_set = 0;// 0-空射导弹 1-机炮+魔法 2-火力支援 3-主动防御 
    int weapon_num[4] = {2,4,2,2};
    int weapon_kind = 0;
    Input& _input;
    float _last_time;
    Renderer& _renderer;
    float _last_time_boom = 7.0f;
    float _last_time_fire = 0.0f;
    float _last_time_help = 0.0f;
    // 尾焰
    Particle_Flareback flareback=Particle_Flareback(1000, 42, getAssetPath("textures/particles/particle_generated.png"),glm::vec3(0.0f, 0.0f, 0.0f),0.2f);
    Particle_Flareback flareback_missle=Particle_Flareback(400, 42, getAssetPath("textures/particles/particle_generated.png"),glm::vec3(0.0f, 0.0f, 0.0f),0.1f);
    // 火球
    Particle_Fireball fireball=Particle_Fireball(1000,42,getAssetPath("textures/particles/particle_generated.png"));
    // 子弹
    Particle_Bullet autocannon=Particle_Bullet(100,42,getAssetPath("textures/particles/particle_generated.png"));
    // 魔法1
    Tergeo tergeo=Tergeo(10000,42,6.0f,getAssetPath("textures/particles/particle_generated2.png"));
    Kendavra kendavra=Kendavra(1,42,getAssetPath("textures/particles/particle_generated2.png"));
    // 爆炸
    std::vector<Particle_Explosion*> explosions;
    std::vector<glm::vec3> explosion_positions;
    std::vector<Particle_Fireball_explosioin*> fireball_explosions;
    std::vector<glm::vec3> fireball_explosion_positions;
    // Particle_Explosion explosion(100000,5000, 42, getAssetPath("textures/particles/particle_generated.png"));
    // explosion.start_();

    void add_explode(glm::vec3 pos){
        for(size_t i=0;i<explosions.size();i++){
            if(!explosions[i]->exists_now()){
                explosions[i]->start_();
                explosion_positions[i] = pos;
                return;
            }
        }
        explosions.push_back(new Particle_Explosion(20000,1000, 42, getAssetPath("textures/particles/particle_generated.png")));
        explosion_positions.push_back(pos);
        explosions.back()->start_();
    }
    void add_fireball_explode(glm::vec3 pos){
        for(size_t i=0;i<fireball_explosions.size();i++){
            if(!fireball_explosions[i]->exists_now()){
                fireball_explosions[i]->start_();
                fireball_explosion_positions[i] = pos;
                return;
            }
        }
        fireball_explosions.push_back(new Particle_Fireball_explosioin(400, 42, getAssetPath("textures/particles/particle_generated.png")));
        fireball_explosion_positions.push_back(pos);
        fireball_explosions.back()->start_();
    }



    // 碰撞单元
    std::vector<Plane *> collider_planes;
};