#pragma once

#include "core/framebuffer.hpp"
#include <Eigen/Core>
#include <windows.h>
#include <string>
#include <functional>
#include <memory>

namespace Platform {

    // 键码枚举
    enum class KeyCode {
        A, D, S, W, Space, Escape,
        Count  // 用于计数
    };

    // 鼠标按钮枚举
    enum class MouseButton {
        Left, Right,
        Count  // 用于计数
    };

    // 回调函数类型
    using KeyCallback = std::function<void(KeyCode key, bool pressed)>;
    using MouseButtonCallback = std::function<void(MouseButton button, bool pressed)>;
    using ScrollCallback = std::function<void(float offset)>;
    using CursorPosCallback = std::function<void(double xpos, double ypos)>;

    // Win32窗口类
    class Win32Window {
    public:
        Win32Window(const std::string& title, int width, int height);
        ~Win32Window();

        // 禁止拷贝和赋值
        Win32Window(const Win32Window&) = delete;
        Win32Window& operator=(const Win32Window&) = delete;

        // 主要接口
        bool should_close() const { return should_close_; }
        void poll_events();
        void present_framebuffer(const Framebuffer& framebuffer);

        // 输入查询
        bool is_key_pressed(KeyCode key) const;
        bool is_mouse_button_pressed(MouseButton button) const;
        void get_cursor_position(float& x, float& y) const;

        // 设置回调
        void set_key_callback(KeyCallback callback) { key_callback_ = std::move(callback); }
        void set_mouse_button_callback(MouseButtonCallback callback) { mouse_button_callback_ = std::move(callback); }
        void set_scroll_callback(ScrollCallback callback) { scroll_callback_ = std::move(callback); }
        void set_cursor_pos_callback(CursorPosCallback callback) { cursor_pos_callback_ = std::move(callback); }

        // 获取窗口尺寸
        int get_width() const { return width_; }
        int get_height() const { return height_; }

        // 时间函数
        static float get_time();

    private:
        // 窗口创建和销毁
        void create_window();
        void create_surface();
        void destroy_surface();

        // 消息处理
        static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
        void handle_key_message(WPARAM virtual_key, bool pressed);
        void handle_mouse_button_message(MouseButton button, bool pressed);
        void handle_scroll_message(float offset);

        // 辅助函数
        static KeyCode virtual_key_to_keycode(WPARAM virtual_key);
        void blit_framebuffer_to_surface(const Framebuffer& framebuffer);

        // 成员变量
        std::string title_;
        int width_;
        int height_;
        bool should_close_;

        // Win32相关
        HWND hwnd_;
        HDC memory_dc_;
        HBITMAP dib_bitmap_;
        unsigned char* surface_buffer_;

        // 输入状态
        std::array<bool, static_cast<size_t>(KeyCode::Count)> keys_{};
        std::array<bool, static_cast<size_t>(MouseButton::Count)> mouse_buttons_{};

        // 回调函数
        KeyCallback key_callback_;
        MouseButtonCallback mouse_button_callback_;
        ScrollCallback scroll_callback_;
        CursorPosCallback cursor_pos_callback_;

        // 静态成员
        static bool class_registered_;
        static const wchar_t* WINDOW_CLASS_NAME;
        static const wchar_t* WINDOW_PROP_NAME;
    };

    // 平台初始化/清理函数
    void initialize_platform();


} // namespace Platform