#pragma once

#include "app/frame_timer.hpp"
#include "app/input_manager.hpp"
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
    void update_light_direction(float delta_time);
    void render_scene();
    void sort_models(const Eigen::Matrix4f& view_matrix);

    // 窗口和输入状态
    std::unique_ptr<Platform::Win32Window> window_;
    int width_;
    int height_;
    std::string title_;

    std::unique_ptr<InputManager> input_manager_;
    InputState input_state_;

    // 时间管理
    FrameTimer timer_;

    // 核心组件
    std::unique_ptr<Scene> scene_;
    std::shared_ptr<TargetCamera> camera_;
    std::unique_ptr<Framebuffer> framebuffer_;
};