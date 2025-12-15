#pragma once

#include "utils/config.h"
#include "service/service_locator.h"
#include "interaction/input.h"
#include <glm/glm.hpp>
#include <resource/shader.h>
#include <interaction/camera.h>
#include <cmath>

struct ParallelLight {
    glm::vec3 light_dir;
    glm::vec3 light_color;

    void use(const Shader* shader) const {
        shader->setUniform("lightDir", light_dir);
        shader->setUniform("lightColor", light_color);
    }
};


class Light {
public:
    Light() = default;
    Light(const Camera* camera, const glm::vec3& ambient_color, const ParallelLight& parallel,Input& input)
        : camera_(camera), ambient_color_(ambient_color), parallel_(parallel),input_(input){
            // 昼夜自动循环控制关闭
            auto_daynight_mode_ = false;   // 是否启用自动昼夜

            // 手动控制状态

            
            manual_intensity_ = glm::length(parallel.light_color);
            if(manual_intensity_==0){
                manual_color_ = glm::vec3(0.0f);
            }
            else{
                manual_color_ = parallel.light_color/manual_intensity_;
            }
            
            // 按照 parallel.light_dir 方向，计算出光源位置
            float length = glm::length(parallel.light_dir);
            glm::vec3 normalized_dir = parallel.light_dir / length;
            manual_azimuth_ = asin(normalized_dir.y);
            float horizontal_length = sqrt(normalized_dir.x*normalized_dir.x+normalized_dir.z*normalized_dir.z);
            
            if (horizontal_length > 0.0001f) {
                float x_norm = normalized_dir.x / horizontal_length;
                float z_norm = normalized_dir.z / horizontal_length;
                manual_azimuth_ = atan2(x_norm, z_norm);
                
                // 规范化到 [0, 2π)
                if (manual_azimuth_ < 0.0f) {
                    manual_azimuth_ += 2.0f * glm::pi<float>();
                }
            } else {
                // 垂直方向的情况
                manual_azimuth_ = 0.0f;
            }
        }

    void use(const Shader* shader) const {
        shader->setUniform("camPos", camera_->getPosition());
        shader->setUniform("ambientLight", ambient_color_);
        parallel_.use(shader);
    }
        // 实时计算 Light Space Matrix（用于 Shadow Mapping）
    glm::mat4 getLightSpaceMatrix() const {
        if (!camera_) {
            return glm::mat4(1.0f);
        }

        // 获取相机参数
        auto config = ServiceLocator<Config>::get();
        float fov = glm::radians(45.0f);
        float aspect = (float)config->scr_width / (float)config->scr_height;
        float near_plane = config->z_near;
        float far_plane = config->z_far;

        // 计算视锥体8个角点（世界空间） 
        // TODO: 不行，这里需要考虑摄像机外面的物体，但是如果空中空旷那么未尝不可
        float nh = near_plane * tan(fov / 2.0f); // 近平面高度的一半
        float nw = nh * aspect;                  // 近平面宽度的一半
        float fh = far_plane * tan(fov / 2.0f);  // 远平面高度的一半
        float fw = fh * aspect;                  // 远平面宽度的一半

        glm::vec3 camPos = camera_->getPosition();
        // std::cout<<"camPos: "<<camPos.x<<" "<<camPos.y<<" "<<camPos.z<<std::endl;
        glm::vec3 camFront = glm::normalize(camera_->getFrontVec());
        glm::vec3 camRight = glm::normalize(camera_->getRightVec());
        glm::vec3 camUp = glm::normalize(camera_->getUpVec());

        // 近平面四个点
        glm::vec3 nearCenter = camPos + camFront * near_plane;
        glm::vec3 ntl = nearCenter + camUp * nh - camRight * nw;
        glm::vec3 ntr = nearCenter + camUp * nh + camRight * nw;
        glm::vec3 nbl = nearCenter - camUp * nh - camRight * nw;
        glm::vec3 nbr = nearCenter - camUp * nh + camRight * nw;

        // 远平面四个点
        glm::vec3 farCenter = camPos + camFront * far_plane;
        glm::vec3 ftl = farCenter + camUp * fh - camRight * fw;
        glm::vec3 ftr = farCenter + camUp * fh + camRight * fw;
        glm::vec3 fbl = farCenter - camUp * fh - camRight * fw;
        glm::vec3 fbr = farCenter - camUp * fh + camRight * fw;

        std::array<glm::vec3, 8> frustumCorners = {
            ntl, ntr, nbl, nbr,
            ftl, ftr, fbl, fbr
        };


        glm::vec3 lightDir = -glm::normalize(parallel_.light_dir); // 光的传播方向
        glm::vec3 up = glm::abs(lightDir.y) > 0.99f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);

        // 光源位置：可以设为任意点，只要 view 矩阵正确即可（这里用原点后退）
        // 实际上我们只关心方向，所以 view 矩阵由 lookAt(任意点沿 -lightDir, 任意点, up) 决定
        glm::vec3 lightPos = camPos - lightDir * 1.5f*(far_plane + near_plane); // 足够远
        glm::mat4 lightView = glm::lookAt(lightPos,  camPos,up); // 这个矩阵将世界坐标转换为以光源为原点观察、光线传播方向为 -z 的坐标系

        // 4. 将视锥体角点变换到光源空间（即 lightView * point）
        glm::vec4 minBound(FLT_MAX);
        glm::vec4 maxBound(-FLT_MAX);

        for (const auto& corner : frustumCorners) {
            glm::vec4 lightSpaceCorner = lightView * glm::vec4(corner, 1.0f);
            minBound = glm::min(minBound, lightSpaceCorner);
            maxBound = glm::max(maxBound, lightSpaceCorner);
        }

        // 扩大一点边界防止走样
        float padding = 2.0f;
        minBound.x -= padding; minBound.y -= padding; minBound.z -= padding;
        maxBound.x += padding; maxBound.y += padding; maxBound.z += padding;

        glm::mat4 lightProjection = glm::ortho(
            minBound.x, maxBound.x,
            minBound.y, maxBound.y,
            -maxBound.z  ,-minBound.z//因为ortho需要两个正数的z，所以这里取负值
        );
        // // 输出包围盒调试
        // std::cout << "minBound: " << minBound.x << " " << minBound.y << " " << minBound.z << std::endl;
        // std::cout << "maxBound: " << maxBound.x << " " << maxBound.y << " " << maxBound.z << std::endl;
        // // 输出光线方向调试
        // std::cout << "lightDir: " << lightDir.x << " " << lightDir.y << " " << lightDir.z << std::endl;
        // // 输出飞机位置的light space坐标
        // glm::vec4 lightSpacePos = lightView * glm::vec4(camPos, 1.0f);
        // std::cout << "lightSpacePos: " << lightSpacePos.x << " " << lightSpacePos.y << " " << lightSpacePos.z << std::endl;
        // lightSpacePos = lightProjection * lightSpacePos;
        // std::cout << "lightSpacePos: " << lightSpacePos.x / lightSpacePos.w << " " << lightSpacePos.y / lightSpacePos.w << " " << lightSpacePos.z / lightSpacePos.w << std::endl;
        return lightProjection * lightView;
    }
    void update(float dt) {
        if(input_.getKeyPressedDown(InputKey::SLASH)|input_.getKeyPressed(InputKey::SLASH)){
            // 昼夜自动循环控制
            auto_daynight_mode_ = false;   // 关闭昼夜循环模式
            // 手动控制状态
            manual_azimuth_ = 0.0f;// 经度
            manual_altitude_ = glm::radians(-90.0f);// 纬度
            manual_color_ = glm::vec3(1.0f,1.0f,1.0f);
            manual_intensity_ = 2.5f;
            parallel_.light_dir = glm::vec3(0.0f, 1.0f, 0.0f);
            parallel_.light_color = glm::vec3(2.5f, 2.5f, 2.5f);
            ambient_color_ = 0.333f * parallel_.light_color;
            return ;
        }

        if (input_.getKeyPressed(InputKey::BACKSLASH)) {// 若按下 \\ 则切换自动昼夜循环模式
            auto_daynight_mode_ = true;
        }
        else{
            auto_daynight_mode_ = false;
        }

        glm::vec3 final_dir;// 临时变量，可以不影响原先接口
        glm::vec3 final_color;

        if (auto_daynight_mode_) {  // ========== 自动昼夜循环模式 ==========
            // 计算当前时间
            day_time_ += dt * (24.0f / day_cycle_duration_);
            while (day_time_ >= 24.0f){
                day_time_ -= 24.0f;
            }

            // 太阳从东(-90°)升到西(+90°)，正午在正上方(0°)
            float azimuth_deg = -90.0f + day_time_ * 15.0f; // 每小时15度（360/24）
            float altitude_deg;

            // 高度角：正午最高（~60°），日出日落为0°，夜晚为负
            float t = (day_time_ - 12.0f) / 6.0f;
            altitude_deg = 60.0f * cos(t * glm::half_pi<float>());// 太阳高度角


            float azimuth = glm::radians(azimuth_deg);
            float altitude = glm::radians(altitude_deg);

            // 计算太阳方向（世界空间，Z向前）
            final_dir = glm::vec3(
                cos(altitude) * sin(azimuth),
                sin(altitude),
                cos(altitude) * cos(azimuth)
            );
            final_dir = glm::normalize(final_dir);



            // 根据时间设置光照颜色和强度
            glm::vec3 dawn_color   = glm::vec3(1.0f, 0.5f, 0.3f);   // 清晨/黄昏：暖橙
            glm::vec3 noon_color   = glm::vec3(1.0f, 1.0f, 1.0f);   // 正午：白色
            glm::vec3 dusk_color   = glm::vec3(1.0f, 0.6f, 0.2f);   // 黄昏：更红
            glm::vec3 moon_light= glm::vec3(0.6f, 0.6f, 0.95f); // 半夜 冷色、较暗
            // 插值颜色
            if (day_time_ <= 12.0f) {
                if(day_time_ >=6.0f){
                    float t = (day_time_ - 6.0f) / 6.0f;
                    final_color = glm::mix(dawn_color, noon_color, t);
                }
                else{
                    float t = day_time_ / 6.0f;
                    final_color = glm::mix(moon_light, dawn_color, t);
                }            
            } else {
                if(day_time_ <= 18.0f){
                    float t = (day_time_ - 12.0f) / 6.0f;
                    final_color = glm::mix(noon_color, dusk_color, t);
                }
                else{
                    float t = (day_time_ - 18.0f) / 6.0f;
                    final_color = glm::mix(dusk_color, moon_light, t);
                }
            }

            // 强度：正午最亮，早晚较暗
            float intensity_t;
            if(day_time_ <= 12.0f){
                intensity_t=(day_time_ - 6.0f) / 6.0f ;
            }
            else{
                intensity_t = (18.0f - day_time_) / 6.0f;
            }
            if(intensity_t<0.0f){
                intensity_t=0.0f;
            }
            float intensity = 1.0f + 1.8f * intensity_t;
            final_color *= intensity;


        } else {// ========== 手动控制模式 ==========

            // 方向控制
            const float rot_speed = glm::radians(30.0f) * dt;
            if (input_.getKeyPressed(InputKey::LEFT_BRACKET)){
                manual_azimuth_ -= rot_speed;
            } 
            if (input_.getKeyPressed(InputKey::RIGHT_BRACKET)){
                manual_azimuth_ += rot_speed;
            } 
            if (input_.getKeyPressed(InputKey::SEMICOLON)){
                manual_altitude_ += rot_speed;
            }     
            if (input_.getKeyPressed(InputKey::APOSTROPHE)){
                manual_altitude_ -= rot_speed;
            }
            float bias = 0.02f;
            while(manual_altitude_ <-glm::pi<float>()-bias){
                manual_altitude_ += 2.0f*glm::pi<float>();
            }
            while(manual_altitude_ >glm::pi<float>()+bias){
                manual_altitude_ -= 2.0f*glm::pi<float>();
            }
            while(manual_azimuth_ < -glm::pi<float>()-bias){
                manual_azimuth_ += 2.0f*glm::pi<float>();
            }
            while(manual_azimuth_ > glm::pi<float>() + bias){
                manual_azimuth_ -= 2.0f*glm::pi<float>();
            }
            
            // 构建方向
            final_dir = glm::vec3(
                cos(manual_altitude_) * sin(manual_azimuth_),
                sin(manual_altitude_),
                cos(manual_altitude_) * cos(manual_azimuth_)
            );
            bool is_morning=true;// 判断是否是白天
            if(final_dir.z < 0.0f){// 如果太阳下山，即指向太阳的向量向下那么就是晚上
                is_morning=false;
                final_dir = -final_dir;// 月光，方向反转
            }

            // 光强
            const float intensity_step = 0.5f * dt;
            if (input_.getKeyPressed(InputKey::COMMA)){
                manual_intensity_ -= intensity_step;
            }  
            if (input_.getKeyPressed(InputKey::PERIOD)){
                manual_intensity_ += intensity_step;
            }
            manual_intensity_ = glm::clamp(manual_intensity_, 0.0f, 4.0f);// 限制光强区间



            // 颜色比例（保持亮度）
            glm::vec3 color = manual_color_;// 手动颜色
            float current_L = 0.299f * color.r + 0.587f * color.g + 0.114f * color.b;// 使用人眼感知公式
            const float delta = 0.01f;
            if (input_.getKeyPressed(InputKey::B)) {
                color.r = glm::min(1.0f, color.r + delta);
                float new_L = 0.299f * color.r + 0.587f * color.g + 0.114f * color.b;
                float diff = new_L - current_L;
                color.g -= diff * (0.587f / (0.587f + 0.114f));
                color.b -= diff * (0.114f / (0.587f + 0.114f));
                color.g = glm::clamp(color.g, 0.0f, 1.0f);
                color.b = glm::clamp(color.b, 0.0f, 1.0f);
            }
            if (input_.getKeyPressed(InputKey::N)) {
                color.g = glm::min(1.0f, color.g + delta);
                float new_L = 0.299f * color.r + 0.587f * color.g + 0.114f * color.b;
                float diff = new_L - current_L;
                color.r -= diff * (0.299f / (0.299f + 0.114f));
                color.b -= diff * (0.114f / (0.299f + 0.114f));
                color.r = glm::clamp(color.r, 0.0f, 1.0f);
                color.b = glm::clamp(color.b, 0.0f, 1.0f);
            }
            if (input_.getKeyPressed(InputKey::M)) {
                color.b = glm::min(1.0f, color.b + delta);
                float new_L = 0.299f * color.r + 0.587f * color.g + 0.114f * color.b;
                float diff = new_L - current_L;
                color.r -= diff * (0.299f / (0.299f + 0.587f));
                color.g -= diff * (0.587f / (0.299f + 0.587f));
                color.r = glm::clamp(color.r, 0.0f, 1.0f);
                color.g = glm::clamp(color.g, 0.0f, 1.0f);
            }


            manual_color_ = color;


            final_dir = glm::normalize(final_dir);

            if(is_morning){
                final_color = manual_color_ * manual_intensity_;
            }
            else{
                final_color = manual_color_ * manual_intensity_/1.5f;
            }
            
            // std::cout<<"manual_color: "<<manual_color_.x<<" "<<manual_color_.y<<" "<<manual_color_.z<<std::endl;
            // std::cout<<"manual_intensity: "<<manual_intensity_<<std::endl;
            // std::cout<<"final_dir: "<<final_dir.x<<" "<<final_dir.y<<" "<<final_dir.z<<std::endl;
        }

        // ========== 应用最终结果 ==========
        parallel_.light_dir = final_dir;
        parallel_.light_color = final_color;
        ambient_color_ = 0.333f * parallel_.light_color;

        // 打印中间结果调试
        // std::cout<<"final_dir: "<<final_dir.x<<" "<<final_dir.y<<" "<<final_dir.z<<std::endl;
        // std::cout<<"final_color: "<<final_color.x<<" "<<final_color.y<<" "<<final_color.z<<std::endl;
        // std::cout<<"ambient_color: "<<ambient_color_.x<<" "<<ambient_color_.y<<" "<<ambient_color_.z<<std::endl;

    }

private:
    const Camera* camera_ {nullptr};
    glm::vec3 ambient_color_;
    ParallelLight parallel_;


    // 昼夜自动循环控制
    bool auto_daynight_mode_ = false;   // 是否启用自动昼夜
    float day_time_ = 6.0f;             // 一天中的时间（0 ~ 24 小时）
    const float day_cycle_duration_ = 24.0f; // 自动循环一圈的时间，用秒表示

    // 手动控制状态
    float manual_azimuth_ = 0.0f;// 经度
    float manual_altitude_ = glm::radians(45.0f);// 纬度
    glm::vec3 manual_color_ = glm::vec3(2.5f);
    float manual_intensity_ = 1.0f;

protected:
    const Input& input_;
};
