#pragma once

#include "platform/window.hpp"
#include "core/framebuffer.hpp"
#include <memory>

class WindowAdapter {
public:
    WindowAdapter(int width, int height, const std::string& title);
    ~WindowAdapter();

    // GLFW兼容接口
    bool should_close() const;
    void poll_events();
    void swap_buffers();
    void present_framebuffer(const Framebuffer& framebuffer);

    // 输入接口
    bool is_key_pressed(int key) const;
    bool is_mouse_button_pressed(int button) const;
    void get_cursor_position(double& x, double& y) const;

    // 时间接口
    static float get_time();

    // 回调设置
    void set_user_pointer(void* ptr);
    void* get_user_pointer() const;

    template<typename Callback>
    void set_key_callback(Callback callback);

    template<typename Callback>
    void set_mouse_button_callback(Callback callback);

    template<typename Callback>
    void set_scroll_callback(Callback callback);

    template<typename Callback>
    void set_cursor_pos_callback(Callback callback);

private:
    std::unique_ptr<Platform::Win32Window> window_;
    void* user_pointer_;

    // 将平台特定的键码转换为GLFW兼容的键码
    static int platform_key_to_glfw_key(Platform::KeyCode key);
    static int platform_button_to_glfw_button(Platform::MouseButton button);
};