#include "platform/window.hpp"
#include <iostream>
#include <cassert>
#include <algorithm>
#include <stdexcept>
#include <cstring>

namespace Platform {

    // 静态成员定义
    bool Win32Window::class_registered_ = false;
    const wchar_t* Win32Window::WINDOW_CLASS_NAME = L"SoftRendererWindow";
    const wchar_t* Win32Window::WINDOW_PROP_NAME = L"WindowInstance";

    Win32Window::Win32Window(const std::string& title, int width, int height)
        : title_(title), width_(width), height_(height), should_close_(false),
        hwnd_(nullptr), memory_dc_(nullptr), dib_bitmap_(nullptr), surface_buffer_(nullptr)
    {
        assert(width > 0 && height > 0);
        create_window();
        create_surface();
    }

    Win32Window::~Win32Window() {
        destroy_surface();

        if (hwnd_) {
            RemovePropW(hwnd_, WINDOW_PROP_NAME);
            DestroyWindow(hwnd_);
        }
    }

    void Win32Window::create_window() {
        // 确保窗口类已注册
        if (!class_registered_) {
            WNDCLASSW wc = {};                                                      // Windows API 注册窗口类结构体
            wc.style = CS_HREDRAW | CS_VREDRAW;                                     // 窗口类风格：宽度高度改变时重绘
            wc.lpfnWndProc = window_proc;                                           // 消息回调函数（处理所有窗口消息哦）
            wc.hInstance = GetModuleHandle(nullptr);                                // 指定当前程序实力句柄
            wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);                          // 设置窗口图标
            wc.hCursor = LoadCursor(nullptr, IDC_ARROW);                            // 设置窗口鼠标指针
            wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));    // 设置窗口背景画刷
            wc.lpszClassName = WINDOW_CLASS_NAME;                                   // 设置窗口类名称

            if (!RegisterClassW(&wc)) { // 注册窗口
                throw std::runtime_error("Failed to register window class");
            }
            class_registered_ = true;
        }

        // 计算窗口尺寸
        DWORD style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;                     // 窗口样式
        RECT rect = { 0, 0, width_, height_ };                                      // 描述窗口的矩形区域（初始为内容区大小）
        AdjustWindowRect(&rect, style, FALSE);                                      // 根据窗口样式调整rect

        // 最终窗口的总宽高
        int window_width = rect.right - rect.left;                                  
        int window_height = rect.bottom - rect.top;

        // 转换标题为宽字符
        std::wstring wide_title(title_.begin(), title_.end());

        // 创建窗口
        hwnd_ = CreateWindowW(
            WINDOW_CLASS_NAME,
            wide_title.c_str(),
            style,
            CW_USEDEFAULT, CW_USEDEFAULT,                                           // 初始位置（设置为系统决定）
            window_width, window_height,
            nullptr, nullptr,
            GetModuleHandle(nullptr),                                       
            nullptr
        );

        if (!hwnd_) {
            throw std::runtime_error("Failed to create window");
        }

        // 设置窗口属性指向这个实例
        SetPropW(hwnd_, WINDOW_PROP_NAME, this);                                    

        // 显示窗口
        ShowWindow(hwnd_, SW_SHOW);

        // 强制立即重绘
        UpdateWindow(hwnd_);
    }

    // 为窗口创建一个内存绘图表面(DIB Section, 设备无关位图)
    void Win32Window::create_surface() {
        // 获取窗口DC(Device Context)
        HDC window_dc = GetDC(hwnd_);

        // 创建兼容的内存DC
        memory_dc_ = CreateCompatibleDC(window_dc);
        ReleaseDC(hwnd_, window_dc);

        if (!memory_dc_) {
            throw std::runtime_error("Failed to create memory DC");
        }

        // 创建DIB
        BITMAPINFOHEADER bmi_header = {};
        bmi_header.biSize = sizeof(BITMAPINFOHEADER);
        bmi_header.biWidth = width_;
        bmi_header.biHeight = -height_;                                 // 负值表示自上而下的DIB
        bmi_header.biPlanes = 1;                                        // 位图的颜色平面数，必须为1
        bmi_header.biBitCount = 32;                                     // 每个像素的表示位数，32为BGRA格式
        bmi_header.biCompression = BI_RGB;                              // 压缩方式，设置为不压缩

        BITMAPINFO bmi = {};
        bmi.bmiHeader = bmi_header;

        dib_bitmap_ = CreateDIBSection(
            memory_dc_,
            &bmi,
            DIB_RGB_COLORS,
            reinterpret_cast<void**>(&surface_buffer_),
            nullptr,
            0
        );

        if (!dib_bitmap_ || !surface_buffer_) {
            throw std::runtime_error("Failed to create DIB section");
        }

        // 选择位图到内存DC
        SelectObject(memory_dc_, dib_bitmap_);
    }

    void Win32Window::destroy_surface() {
        if (dib_bitmap_) {
            DeleteObject(dib_bitmap_);
            dib_bitmap_ = nullptr;
            surface_buffer_ = nullptr;
        }

        if (memory_dc_) {
            DeleteDC(memory_dc_);
            memory_dc_ = nullptr;
        }
    }

    // Windows消息循环，用以处理窗口事件
    void Win32Window::poll_events() {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) { // 从当前线程的消息队列中获取一条消息
            TranslateMessage(&msg);     // 对键盘消息进行转换
            DispatchMessageW(&msg);     // 分发消息到注册的窗口
        }
    }

    // 将软件渲染结果显示到窗口上
    void Win32Window::present_framebuffer(const Framebuffer& framebuffer) {
        blit_framebuffer_to_surface(framebuffer);

        // 将内存DC的内容复制到窗口
        HDC window_dc = GetDC(hwnd_);
        BitBlt(window_dc, 0, 0, width_, height_, memory_dc_, 0, 0, SRCCOPY);
        ReleaseDC(hwnd_, window_dc);
    }

    void Win32Window::blit_framebuffer_to_surface(const Framebuffer& framebuffer) {
        assert(framebuffer.get_width() == width_ && framebuffer.get_height() == height_);

        const auto& color_buffer = framebuffer.get_color_buffer();

        // 转换RGBA到BGRA并拷贝到surface
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                int src_index = (y * width_ + x) * 4;
                int dst_index = (y * width_ + x) * 4;

                // RGBA -> BGRA 转换
                surface_buffer_[dst_index + 0] = color_buffer[src_index + 2]; // B
                surface_buffer_[dst_index + 1] = color_buffer[src_index + 1]; // G
                surface_buffer_[dst_index + 2] = color_buffer[src_index + 0]; // R
                surface_buffer_[dst_index + 3] = color_buffer[src_index + 3]; // A
            }
        }
    }

    bool Win32Window::is_key_pressed(KeyCode key) const {
        return keys_[static_cast<size_t>(key)];
    }

    bool Win32Window::is_mouse_button_pressed(MouseButton button) const {
        return mouse_buttons_[static_cast<size_t>(button)];
    }

    void Win32Window::get_cursor_position(float& x, float& y) const {
        POINT point;
        GetCursorPos(&point);
        ScreenToClient(hwnd_, &point);
        x = static_cast<float>(point.x);
        y = static_cast<float>(point.y);
    }

    float Win32Window::get_time() {
        static LARGE_INTEGER frequency = {};
        static LARGE_INTEGER start_time = {};
        static bool initialized = false;

        if (!initialized) {
            QueryPerformanceFrequency(&frequency);
            QueryPerformanceCounter(&start_time);
            initialized = true;
        }

        LARGE_INTEGER current_time;
        QueryPerformanceCounter(&current_time);

        return static_cast<float>(current_time.QuadPart - start_time.QuadPart) /
            static_cast<float>(frequency.QuadPart);
    }

    LRESULT CALLBACK Win32Window::window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
        Win32Window* window = static_cast<Win32Window*>(GetPropW(hwnd, WINDOW_PROP_NAME));

        if (!window) {
            return DefWindowProcW(hwnd, msg, wparam, lparam);
        }

        switch (msg) {
        case WM_CLOSE:
            window->should_close_ = true;
            return 0;

        case WM_KEYDOWN:
            window->handle_key_message(wparam, true);
            return 0;

        case WM_KEYUP:
            window->handle_key_message(wparam, false);
            return 0;

        case WM_LBUTTONDOWN:
            window->handle_mouse_button_message(MouseButton::Left, true);
            return 0;

        case WM_LBUTTONUP:
            window->handle_mouse_button_message(MouseButton::Left, false);
            return 0;

        case WM_RBUTTONDOWN:
            window->handle_mouse_button_message(MouseButton::Right, true);
            return 0;

        case WM_RBUTTONUP:
            window->handle_mouse_button_message(MouseButton::Right, false);
            return 0;

        case WM_MOUSEWHEEL: {
            float offset = GET_WHEEL_DELTA_WPARAM(wparam) / static_cast<float>(WHEEL_DELTA);
            window->handle_scroll_message(offset);
            return 0;
        }

        case WM_MOUSEMOVE: {
            if (window->cursor_pos_callback_) {
                double x = static_cast<double>(LOWORD(lparam));
                double y = static_cast<double>(HIWORD(lparam));
                window->cursor_pos_callback_(x, y);
            }
            return 0;
        }

        default:
            return DefWindowProcW(hwnd, msg, wparam, lparam);
        }
    }

    void Win32Window::handle_key_message(WPARAM virtual_key, bool pressed) {
        KeyCode key = virtual_key_to_keycode(virtual_key);
        if (key != KeyCode::Count) {
            keys_[static_cast<size_t>(key)] = pressed;

            if (key_callback_) {
                key_callback_(key, pressed);
            }
        }
    }

    void Win32Window::handle_mouse_button_message(MouseButton button, bool pressed) {
        mouse_buttons_[static_cast<size_t>(button)] = pressed;

        if (mouse_button_callback_) {
            mouse_button_callback_(button, pressed);
        }
    }

    void Win32Window::handle_scroll_message(float offset) {
        if (scroll_callback_) {
            scroll_callback_(offset);
        }
    }

    KeyCode Win32Window::virtual_key_to_keycode(WPARAM virtual_key) {
        switch (virtual_key) {
        case 'A': return KeyCode::A;
        case 'D': return KeyCode::D;
        case 'S': return KeyCode::S;
        case 'W': return KeyCode::W;
        case VK_SPACE: return KeyCode::Space;
        case VK_ESCAPE: return KeyCode::Escape;
        default: return KeyCode::Count;
        }
    }

    void initialize_platform() {
        // 设置工作目录到assets文件夹
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        *wcsrchr(path, L'\\') = L'\0';
        SetCurrentDirectoryW(path);

        // 尝试进入assets目录
        std::cout << "Trying to enter assets directory" << std::endl;

        if (SetCurrentDirectoryW(L"assets") == 0) {
            // 如果失败，可能是在不同的目录结构中，尝试其他路径
            SetCurrentDirectoryW(L"../assets");
        }
    }

    void terminate_platform() {
        // 清理工作（如果需要的话）
    }

}