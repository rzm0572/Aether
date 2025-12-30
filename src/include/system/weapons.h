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
    Weapons(int kind, Owner owner, Input& input,Renderer& renderer): owner_(owner), _kind(kind), _input(input), _last_time(glfwGetTime()), _renderer(renderer){
        srand(time(NULL));
        // 尾焰必须渲染
        
        flareback.start_();
        flareback_missle.start_();
        // 空对空导弹模型导入
        if (!missle_model.loadModel(getAssetPath("models/missle1_2/scene.gltf"))) {
            std::cerr << "Failed to load model missle1!" << std::endl;
        }
        // 对地导弹模型导入
        if (!boom_model.loadModel(getAssetPath("models/boom/scene.gltf"))) {
            std::cerr << "Failed to load boom model!" << std::endl;
        }
        
        // Plane* missle = new Plane(input, missle_model, initial_position2, initial_rotation, velocity, angular_velocity);
        // Plane* boom = new Plane(input, plane_model, initial_position, initial_rotation, velocity, angular_velocity,makeTransformMatrix(0.0f,0.0f,0.0f,0.0f,90.0f,0.0f,0.1f,0.1f,0.1f));
    
        // 

        // GPU 粒子的粒子效果测试
        // Particle_Fireball fireball(10000,42,getAssetPath("textures/particles/particle_generated.png"));
        // fireball.start_();



        // Particle_Flareback_Missle flareback_missle(100000, 42, getAssetPath("textures/particles/particle_generated.png"), glm::vec3(0.0f, 0.0f, 0.0f), 0.2f);
        // flareback_missle.start_();

        // 绘制爆炸的粒子效果
        // fireball.draw(plane->getTransformComponent().getPosition(),view, projection, third_person_camera.getPosition(),3.0f,3.0f,0.1f);
        // explosion.draw(plane->getTransformComponent().getPosition(), view, projection, third_person_camera.getPosition(), 3.0f,3.0f,0.1f);

        // flareback_missle.draw(plane->getTransformComponent().getPosition(), view, projection, third_person_camera.getPosition(), 60.0f, 5.0f, 1.0f, 0.01f, plane->physical_component().GetForward());

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
            if(glfwGetTime() - _last_time > 0.0001f){// 间隔时间
                // std::cout<<"check time right "<<_last_time<<std::endl;
                // ++ 武器切换 ++
                if(_input.getKeyPressed(InputKey::H)){
                    // 空射导弹
                    weapon_set = 0;
                }
                if(_input.getKeyPressed(InputKey::J)){
                    // 机炮+魔法
                    weapon_set = 1;
                }
                if(_input.getKeyPressed(InputKey::K)){
                    // 火力支援
                    weapon_set = 2;
                }
                if(_input.getKeyPressed(InputKey::L)){
                    // 主动防御
                    weapon_set = 3;
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
                Fire = true;
            // }
        }
        // std::cout<<"weapon_set:"<<weapon_set<<" weapon_kind:"<<weapon_kind<<" fire: "<<Fire<<std::endl;
        // == 发射武器 ==
        if(Fire && _last_time_fire + 0.5f < glfwGetTime()){            
            switch(weapon_set){
                case 0:
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
                            (missles[slot])->physical_component().initialize(pos - up * 1.0f, rotation, Velocity - up * 4.0f, glm::vec3(0.0f,0.0f,0.0f));
                            missle_is_active[slot] = true;
                            missle_start_time[slot] = glfwGetTime();
                            // missle_flarebacks[slot].start_();
                        }
                        else{
                            missles.push_back(new Plane(owner_, _input, missle_model, pos - up * 1.0f, rotation, Velocity - up * 4.0f, glm::vec3(0.0f,0.0f,0.0f)));
                            missle_is_active.push_back(true);
                            missle_start_time.push_back(glfwGetTime());
                            // missle_flarebacks.push_back(Particle_Flareback(1000, 42, getAssetPath("textures/particles/particle_generated.png"), glm::vec3(0.0f, 0.0f, 0.0f), 0.2f));
                            // missle_flarebacks.back().start_();
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
                            (booms[slot])->physical_component().initialize(pos - up * 2.0f, rotation, Velocity - up * 4.0f, glm::vec3(0.0f,0.0f,0.0f));
                            boom_is_active[slot] = true;
                            boom_start_time[slot] = glfwGetTime();
                            // boom_flarebacks[slot].start_();
                        }
                        else{
                            booms.push_back(new Plane(owner_, _input, boom_model, pos - up * 2.0f, rotation, Velocity - up * 4.0f, glm::vec3(0.0f,0.0f,0.0f)));
                            boom_is_active.push_back(true);
                            boom_start_time.push_back(glfwGetTime());
                            // boom_flarebacks.push_back(Particle_Flareback(1000, 42, getAssetPath("textures/particle_generated.png"), glm::vec3(0.0f, 0.0f, 0.0f), 0.2f));
                            // boom_flarebacks.back().start_();
                        }
                    }
                break;
                case 1:
                    // 机炮+魔法
                    if(weapon_kind == 0){

                    }
                    else if(weapon_kind == 1){

                    }
                    else{

                    }
                break;
                case 2:
                    // 火力支援
                    if(weapon_kind == 0){// 地对空导弹

                    }
                    else{// 空中魔法

                    
                    }
                break;
                case 3:
                    // 主动防御
                    if(weapon_kind == 0){

                    }
                    else{

                    }
                break;
            }
            _last_time_fire = glfwGetTime();
        }

        // == 绘制武器 ==
        // ++ 空空导弹 ++
        for(size_t i=0;i<missles.size();i++){
            if(missle_is_active[i]){
                if(missle_start_time[i] + _last_time_boom >= glfwGetTime()){
                    missles[i]->update(dt,2,target);
                    _renderer.submit_recursive(missles[i]);
                    
                    // flareback2.draw(missles[i]->getTransformComponent().getPosition(), view, projection, third_person_camera.getPosition(), 10.0f, 1.0f, 0.1f, missles[i]->physical_component().GetForward());
                    flareback_missle.draw(missles[i]->getTransformComponent().getPosition(), view, projection, third_person_camera.getPosition(), 3.0f, 0.5f, 0.05f, missles[i]->physical_component().GetForward());
                }
                else{

                    missle_is_active[i] = false;
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
                }
            }
        }

    }
private:
    Model missle_model;
    Model boom_model;
    std::vector<Plane*> missles,booms,missles_earth;
    BulletManager bullet_manager;
    std::vector<bool> missle_is_active,boom_is_active,missle_earth_is_active;
    std::vector<float> missle_start_time,boom_start_time,missle_earth_start_time;
    Owner owner_;
    int _kind;// 是可以手动操纵-0/还是自动操纵-1
    int weapon_set = 0;// 0-空射导弹 1-机炮+魔法 2-火力支援 3-主动防御 
    int weapon_num[4] = {2,3,2,2};
    int weapon_kind = 0;
    Input& _input;
    float _last_time;
    Renderer& _renderer;
    float _last_time_boom = 7.0f;
    float _last_time_fire = 0.0f;
    // 尾焰
    Particle_Flareback flareback=Particle_Flareback(1000, 42, getAssetPath("textures/particles/particle_generated.png"),glm::vec3(0.0f, 0.0f, 0.0f),0.2f);
    Particle_Flareback flareback_missle=Particle_Flareback(400, 42, getAssetPath("textures/particles/particle_generated.png"),glm::vec3(0.0f, 0.0f, 0.0f),0.1f);

    // 爆炸
    // Particle_Explosion explosion(100000,5000, 42, getAssetPath("textures/particles/particle_generated.png"));
    // explosion.start_();
};