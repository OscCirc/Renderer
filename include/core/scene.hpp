#pragma once

#include <vector>
#include <memory>
#include <Eigen/Core>
#include "shading/texture.hpp" 
#include "geometry/model.hpp"
#include "geometry/camera.hpp"

struct perframe
{
    float frame_time;                       //总时间
    float delta_time;                       //帧间隔
    Eigen::Vector3f light_dir;              //主平行光
    Eigen::Vector3f camera_pos;             //相机的世界空间坐标
    Eigen::Matrix4f light_view_matrix;      
    Eigen::Matrix4f light_proj_matrix;      
    Eigen::Matrix4f camera_view_matrix;     
    Eigen::Matrix4f camera_proj_matrix;
    float ambient_intensity;                //环境光强
    float punctual_intensity;               //点源光强
    Texture *shadow_map;                    //阴影贴图
    int layer_view;                         //调试视图模式标志
};



class Scene
{
public:
    // 成员变量
    Eigen::Vector4f background_color;
    std::shared_ptr<Model_Base> skybox;
    std::vector<std::shared_ptr<Model_Base>> models;
    std::shared_ptr<TargetCamera> camera;
    perframe frame_data;

    // 阴影贴图资源，由 Scene 类独占所有权
    std::unique_ptr<Framebuffer> shadow_buffer;
    std::unique_ptr<Texture> shadow_map;
    
    Scene(const std::string& filename, const Eigen::Matrix4f& root_transform = Eigen::Matrix4f::Identity());
    
    Scene(const Eigen::Vector3f &background,
          std::shared_ptr<Model_Base> skybox_model,
          std::vector<std::shared_ptr<Model_Base>> model_list,
          float ambient, float punctual,
          int shadow_width, int shadow_height)
        : background_color(background.x(), background.y(), background.z(), 1.0f),
          skybox(std::move(skybox_model)),
          models(std::move(model_list)),
          camera(nullptr)
    {
        // 初始化光照强度
        frame_data.ambient_intensity = ambient;
        frame_data.punctual_intensity = punctual;

        // 条件性创建阴影贴图资源
        if (shadow_width > 0 && shadow_height > 0)
        {
            shadow_buffer = std::make_unique<Framebuffer>(shadow_width, shadow_height);
            shadow_map = std::make_unique<Texture>(shadow_width, shadow_height);
        }
    }

    ~Scene() = default;

    Scene(const Scene &) = delete;
    Scene &operator=(const Scene &) = delete;

    // 更新每帧变化的数据
    void update_per_frame_data(float frame_time, float delta_time,
                               const Eigen::Vector3f &light_dir,
                               const Eigen::Vector3f &camera_pos,
                               const Eigen::Matrix4f &light_view_matrix,
                               const Eigen::Matrix4f &light_proj_matrix,
                               const Eigen::Matrix4f &camera_view_matrix,
                               const Eigen::Matrix4f &camera_proj_matrix,
                               int layer_view)
    {
        frame_data.frame_time = frame_time;
        frame_data.delta_time = delta_time;
        frame_data.light_dir = light_dir;
        frame_data.camera_pos = camera_pos;
        frame_data.light_view_matrix = light_view_matrix;
        frame_data.light_proj_matrix = light_proj_matrix;
        frame_data.camera_view_matrix = camera_view_matrix;
        frame_data.camera_proj_matrix = camera_proj_matrix;
        frame_data.shadow_map = shadow_map.get();
        frame_data.layer_view = layer_view;
    }



};