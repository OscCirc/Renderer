#pragma once

#include "core/scene.hpp"
#include "geometry/camera.hpp"
#include "core/framebuffer.hpp"
#include "platform/window.hpp"
#include "utils/global.hpp"
#include <memory>
#include <string>
#include <functional>
#include <map>


class Application
{
public:
    Application(int width, int height, const std::string& title, std::unique_ptr<Scene> scene);
    ~Application();

    // 禁止拷贝和赋值
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    // 主循环
    void run();

private:
    // 初始化
    void init_window();

    // 每帧逻辑
    void tick();
    void process_input();
    void update_camera();
    void update_light_direction();
    void render_scene();
    void sort_models(const Eigen::Matrix4f& view_matrix);

    // 辅助函数
    Eigen::Vector2f get_pos_delta(const Eigen::Vector2f& old_pos, const Eigen::Vector2f& new_pos) const;
    Eigen::Vector2f get_cursor_pos() const;
    void update_click(float curr_time);

    // 回调函数
    void on_key_pressed(Platform::KeyCode key, bool pressed);
    void on_mouse_button_pressed(Platform::MouseButton button, bool pressed);
    void on_scroll(float offset);
    void on_cursor_pos(double xpos, double ypos);

    // 常量定义
    static constexpr float CLICK_DELAY = 0.25f;  // 双击延迟时间（秒）
    static constexpr float CLICK_DISTANCE_THRESHOLD = 5.0f;  // 点击距离阈值（像素）

    // 窗口和输入状态
    std::unique_ptr<Platform::Win32Window> window_;
    int width_;
    int height_;
    std::string title_;

    // 输入记录
    struct InputRecord {
        bool is_orbiting = false;
        Eigen::Vector2f orbit_pos{ 0, 0 };
        Eigen::Vector2f orbit_delta{ 0, 0 };

        bool is_panning = false;
        Eigen::Vector2f pan_pos{ 0, 0 };
        Eigen::Vector2f pan_delta{ 0, 0 };

        // zoom
        float dolly_delta = 0.0f;

        // light
        float light_theta;
        float light_phi;

        // click
        float press_time;
        float release_time;
        Eigen::Vector2f press_pos;
        Eigen::Vector2f release_pos;
        bool single_click = false;
        bool double_click = false;
        Eigen::Vector2f click_pos;
    } input_record_;

    // 时间管理
    float last_frame_time_ = 0.0f;
    float delta_time_ = 0.0f;
	float print_time_ = 0.0f;

    // 核心组件
    std::unique_ptr<Scene> scene_;
    std::shared_ptr<TargetCamera> camera_;
    std::unique_ptr<Framebuffer> framebuffer_;
};