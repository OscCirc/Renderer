#include "app/input_manager.hpp"
#include "app/application.hpp"
#include "core/graphics.hpp"
#include "utils/resource_cache.hpp"
#include "math/math.hpp"
#include "shading/texture.hpp"
#include <iostream>
#include <algorithm>
#include <vector>
#include <cmath>

// 初始常量
static const Eigen::Vector3f CAMERA_POSITION = { 0, 0, 1.5f };
static const Eigen::Vector3f CAMERA_TARGET = { 0, 0, 0 };
static const float LIGHT_THETA = static_cast<float>(EIGEN_PI) / 4.0f;
static const float LIGHT_PHI = static_cast<float>(EIGEN_PI) / 4.0f;
static const float LIGHT_SPEED = static_cast<float>(EIGEN_PI);


Application::Application(int width, int height, const std::string& title, std::unique_ptr<Scene> scene)
    : width_(width), height_(height), title_(title), scene_(std::move(scene))
{
    window_ = std::make_unique<Platform::Win32Window>(title_, width_, height_);
    input_manager_ = std::make_unique<InputManager>(*window_);

    camera_ = std::make_shared<TargetCamera>(CAMERA_POSITION, CAMERA_TARGET, static_cast<float>(width_) / height_);
    framebuffer_ = std::make_unique<Framebuffer>(width_, height_);

    // 将相机赋给场景
    if (scene_) {
        scene_->camera = camera_;
    }

    input_state_.light_theta = LIGHT_THETA;
    input_state_.light_phi = LIGHT_PHI;
}

Application::~Application()
{
    window_.reset();
}

void Application::run()
{
    while (!window_->should_close())
    {
        timer_.begin_frame(Platform::Win32Window::get_time());
        
        tick();
        
        window_->present_framebuffer(*framebuffer_);
        
        timer_.maybe_print_fps();

        window_->poll_events();
    }
}

static Eigen::Vector3f get_light_dir(float light_theta, float light_phi) {
    Eigen::Vector3f light_dir;
    light_dir.x() = sin(light_phi) * sin(light_theta);
    light_dir.y() = cos(light_phi);
    light_dir.z() = sin(light_phi) * cos(light_theta);
    return -light_dir.normalized();
}

void Application::tick()
{
    float current_time = timer_.current_time();
    float delta_time_ = timer_.delta_time();

	input_manager_->poll(current_time, input_state_);
    process_input();
    update_camera();
    update_light_direction(delta_time_);

    // 更新场景的 per-frame 数据
    Eigen::Vector3f light_dir = get_light_dir(input_state_.light_theta, input_state_.light_phi);

    scene_->update_per_frame_data(current_time, delta_time_, light_dir,
        camera_->get_position(),
        mat4_lookat(-light_dir, Eigen::Vector3f(0, 0, 0), Eigen::Vector3f(0, 1, 0)),
        mat4_orthographic(1, 1, 0, 2),
        camera_->get_view_matrix(),
        camera_->get_projection_matrix(),
        -1);

    render_scene();

}

void Application::process_input()
{
    // 按键处理
    if (window_->is_key_pressed(Platform::KeyCode::Space)) {
        camera_->camera_set_transform(CAMERA_POSITION, CAMERA_TARGET);
        input_state_.light_theta = LIGHT_THETA;
        input_state_.light_phi = LIGHT_PHI;
    }

    // 采样模式切换
    if (window_->is_key_pressed(Platform::KeyCode::Key1)) {
        g_sample_mode = SampleMode::Nearest;
        window_->set_title(title_ + " - Nearest");
    }
    if (window_->is_key_pressed(Platform::KeyCode::Key2)) {
        g_sample_mode = SampleMode::Bilinear;
        window_->set_title(title_ + " - Bilinear");
    }
    if (window_->is_key_pressed(Platform::KeyCode::Key3)) {
        g_sample_mode = SampleMode::Trilinear;
        window_->set_title(title_ + " - Trilinear (Mipmap)");
    }
}

void Application::update_camera()
{
    motion motion_data;
    motion_data.orbit = input_state_.orbit_delta;
    motion_data.pan = input_state_.pan_delta;
    motion_data.dolly = input_state_.dolly_delta;
    camera_->update_transform(motion_data);
}

void Application::update_light_direction(float delta_time)
{
    float angle = LIGHT_SPEED * delta_time;

    if (window_->is_key_pressed(Platform::KeyCode::A)) {
        input_state_.light_theta -= angle;
    }
    if (window_->is_key_pressed(Platform::KeyCode::D)) {
        input_state_.light_theta += angle;
    }
    if (window_->is_key_pressed(Platform::KeyCode::S)) {
        input_state_.light_phi = std::min<float>(input_state_.light_phi + angle, static_cast<float>(EIGEN_PI) - EPSILON);
    }
    if (window_->is_key_pressed(Platform::KeyCode::W)) {
        input_state_.light_phi = std::max<float>(input_state_.light_phi - angle, EPSILON);
    }
}


// 对场景中所有物体排序
void Application::sort_models(const Eigen::Matrix4f& view_matrix)
{
    for (auto& model : scene_->models) {
        Eigen::Vector3f center = model->mesh ? model->mesh->getCenter() : Eigen::Vector3f::Zero();
        Eigen::Vector4f world_pos = model->transform * Eigen::Vector4f(center.x(), center.y(), center.z(), 1.0f);
        Eigen::Vector4f view_pos = view_matrix * world_pos;
        model->distance = -view_pos.z();
    }

    std::sort(scene_->models.begin(), scene_->models.end(),
        [](const std::shared_ptr<Model_Base>& a, const std::shared_ptr<Model_Base>& b) {
            if (a->opaque && !b->opaque) return true;
            if (!a->opaque && b->opaque) return false;
            if (a->opaque) return a->distance < b->distance;
            return a->distance > b->distance;
        });
}

void Application::render_scene()
{
    if (!scene_) return;

    // 更新所有模型的 uniform
    for (auto& model : scene_->models) {
        model->update(&scene_->frame_data);
    }
    if (scene_->skybox) {
        scene_->skybox->update(&scene_->frame_data);
    }

    // 1. 阴影通道
    if (scene_->shadow_buffer && scene_->shadow_map) {
        sort_models(scene_->frame_data.light_view_matrix); // 从光源视角对模型排序
        scene_->shadow_buffer->clear_depth(1.0f);
        for (const auto& model : scene_->models) {
            if (model->opaque) {
                model->draw(scene_->shadow_buffer.get(), 1); // shadow_pass置为1，表示阴影贴图渲染阶段
            }
        }
        scene_->shadow_map->update_from_depth_buffer(*scene_->shadow_buffer);
    }

    // 2. 主渲染通道
    sort_models(scene_->frame_data.camera_view_matrix); // 从相机视角对模型排序
    framebuffer_->clear_color(scene_->background_color);
    framebuffer_->clear_depth(1.0f);

    if (scene_->skybox == nullptr || scene_->frame_data.layer_view >= 0)
    {
        for (const auto& model : scene_->models) {
            model->draw(framebuffer_.get(), 0);
        }
    }
    else
    {

        int num_opaques_skybox = 0;
        for (const auto& model : scene_->models) {
            if (model->opaque) {
                num_opaques_skybox++;
            }
            else {
                break;
            }
        }
        for (int i = 0; i < num_opaques_skybox; ++i) {
            scene_->models[i]->draw(framebuffer_.get(), 0);
        }

        scene_->skybox->draw(framebuffer_.get(), 0);

        for (size_t i = num_opaques_skybox; i < scene_->models.size(); ++i) {
            scene_->models[i]->draw(framebuffer_.get(), 0);
        }
    }
}