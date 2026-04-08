#include "core/application.hpp"
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
    // 初始化平台
    //Platform::initialize_platform();

    init_window();

    // 初始化相机和帧缓冲区
    camera_ = std::make_shared<TargetCamera>(CAMERA_POSITION, CAMERA_TARGET, static_cast<float>(width_) / height_);
    framebuffer_ = std::make_unique<Framebuffer>(width_, height_);

    // 将相机赋给场景
    if (scene_) {
        scene_->camera = camera_;
    }

    // 初始化输入记录
    input_record_.light_theta = LIGHT_THETA;
    input_record_.light_phi = LIGHT_PHI;
}

Application::~Application()
{
    window_.reset();
}

void Application::init_window()
{
    window_ = std::make_unique<Platform::Win32Window>(title_, width_, height_);

    // 设置回调函数
    window_->set_mouse_button_callback([this](Platform::MouseButton button, bool pressed) {
        this->on_mouse_button_pressed(button, pressed);
        });

    window_->set_scroll_callback([this](float offset) {
        this->on_scroll(offset);
        });

    window_->set_cursor_pos_callback([this](double xpos, double ypos) {
        this->on_cursor_pos(xpos, ypos);
        });

}

void Application::run()
{
    last_frame_time_ = Platform::Win32Window::get_time();
	print_time_ = last_frame_time_;
    int num_frames = 0;
    while (!window_->should_close())
    {
        tick();
        
        window_->present_framebuffer(*framebuffer_);
        num_frames += 1;
        if (last_frame_time_ - print_time_ >= 1) {
            int sum_millis = (int)((last_frame_time_ - print_time_) * 1000);
            int avg_millis = sum_millis / num_frames;
			std::cout << "fps: " << num_frames << ", avg: " << avg_millis << " ms" << std::endl;
            num_frames = 0;
            print_time_ = last_frame_time_;
        }

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
    float current_time = Platform::Win32Window::get_time();
    delta_time_ = current_time - last_frame_time_;
    last_frame_time_ = current_time;

	process_input();
    update_camera();
    update_light_direction();
    update_click(current_time);

    // 更新场景的 per-frame 数据
    Eigen::Vector3f light_dir = get_light_dir(input_record_.light_theta, input_record_.light_phi);

    scene_->update_per_frame_data(current_time, delta_time_, light_dir,
        camera_->get_position(),
        mat4_lookat(-light_dir, Eigen::Vector3f(0, 0, 0), Eigen::Vector3f(0, 1, 0)),
        mat4_orthographic(1, 1, 0, 2),
        camera_->get_view_matrix(),
        camera_->get_projection_matrix(),
        -1);

    render_scene();

    // 重置每帧的增量和点击状态
    input_record_.orbit_delta = { 0, 0 };
    input_record_.pan_delta = { 0, 0 };
    input_record_.dolly_delta = 0;
    input_record_.single_click = false;
    input_record_.double_click = false;
}

void Application::process_input()
{
    // 按键处理
    if (window_->is_key_pressed(Platform::KeyCode::Space)) {
        camera_->camera_set_transform(CAMERA_POSITION, CAMERA_TARGET);
        input_record_.light_theta = LIGHT_THETA;
        input_record_.light_phi = LIGHT_PHI;
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
    motion_data.orbit = input_record_.orbit_delta;
    motion_data.pan = input_record_.pan_delta;
    motion_data.dolly = input_record_.dolly_delta;
    camera_->update_transform(motion_data);
}

void Application::update_light_direction()
{
    float angle = LIGHT_SPEED * delta_time_;

    if (window_->is_key_pressed(Platform::KeyCode::A)) {
        input_record_.light_theta -= angle;
    }
    if (window_->is_key_pressed(Platform::KeyCode::D)) {
        input_record_.light_theta += angle;
    }
    if (window_->is_key_pressed(Platform::KeyCode::S)) {
        input_record_.light_phi = std::min<float>(input_record_.light_phi + angle, static_cast<float>(EIGEN_PI) - EPSILON);
    }
    if (window_->is_key_pressed(Platform::KeyCode::W)) {
        input_record_.light_phi = std::max<float>(input_record_.light_phi - angle, EPSILON);
    }
}

Eigen::Vector2f Application::get_pos_delta(const Eigen::Vector2f& old_pos, const Eigen::Vector2f& new_pos) const {
    Eigen::Vector2f delta = new_pos - old_pos;
    return delta / static_cast<float>(height_);
}

Eigen::Vector2f Application::get_cursor_pos() const {
    float x, y;
    window_->get_cursor_position(x, y);
    return Eigen::Vector2f(x, y);
}

void Application::update_click(float curr_time) {
    float last_time = input_record_.release_time;
    if (last_time > 0 && curr_time - last_time > CLICK_DELAY) {
        Eigen::Vector2f pos_delta = input_record_.release_pos - input_record_.press_pos;
        if (pos_delta.norm() < CLICK_DISTANCE_THRESHOLD) {
            input_record_.single_click = true;
        }
        input_record_.release_time = 0;
    }

    if (input_record_.single_click || input_record_.double_click) {
        float click_x = input_record_.release_pos.x() / static_cast<float>(width_);
        float click_y = input_record_.release_pos.y() / static_cast<float>(height_);
        input_record_.click_pos = Eigen::Vector2f(click_x, 1.0f - click_y);  // 翻转Y坐标
    }
}

void Application::on_mouse_button_pressed(Platform::MouseButton button, bool pressed) {
    Eigen::Vector2f cursor_pos = get_cursor_pos();

    if (button == Platform::MouseButton::Left) {
        float curr_time = Platform::Win32Window::get_time();

        if (pressed) {
            input_record_.is_orbiting = true;
            input_record_.orbit_pos = cursor_pos;
            input_record_.press_time = curr_time;
            input_record_.press_pos = cursor_pos;
        }
        else {
            float prev_time = input_record_.release_time;
            Eigen::Vector2f pos_delta = get_pos_delta(input_record_.orbit_pos, cursor_pos);

            input_record_.is_orbiting = false;
            input_record_.orbit_delta += pos_delta;

            // 双击检测
            if (prev_time > 0 && curr_time - prev_time < CLICK_DELAY) {
                input_record_.double_click = true;
                input_record_.release_time = 0;
            }
            else {
                input_record_.release_time = curr_time;
                input_record_.release_pos = cursor_pos;
            }
        }
    }
    else if (button == Platform::MouseButton::Right) {
        if (pressed) {
            input_record_.is_panning = true;
            input_record_.pan_pos = cursor_pos;
        }
        else {
            Eigen::Vector2f pos_delta = get_pos_delta(input_record_.pan_pos, cursor_pos);

            input_record_.is_panning = false;
            input_record_.pan_delta += pos_delta;
        }
    }
}

void Application::on_scroll(float offset) {
    input_record_.dolly_delta += offset;
}

void Application::on_cursor_pos(double xpos, double ypos) {
    Eigen::Vector2f cursor_pos(static_cast<float>(xpos), static_cast<float>(ypos));

    if (input_record_.is_orbiting) {
        Eigen::Vector2f pos_delta = get_pos_delta(input_record_.orbit_pos, cursor_pos);

        input_record_.orbit_delta += pos_delta;
            input_record_.orbit_pos = cursor_pos;
    }

    if (input_record_.is_panning) {
        Eigen::Vector2f pos_delta = get_pos_delta(input_record_.pan_pos, cursor_pos);

        input_record_.pan_delta += pos_delta;
        input_record_.pan_pos = cursor_pos;
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

    //test code
    int num_opaques = 0;
    for (const auto& model : scene_->models) {
        if (model->opaque) {
            num_opaques++;
        }
        else {
            break;
        }
    }
    for (int i = 0; i < num_opaques; ++i) {
        scene_->models[i]->draw(framebuffer_.get(), 0);
    }

    //scene_->skybox->draw(framebuffer_.get(), 0);

    for (size_t i = num_opaques; i < scene_->models.size(); ++i) {
        scene_->models[i]->draw(framebuffer_.get(), 0);
    }
    //test end


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