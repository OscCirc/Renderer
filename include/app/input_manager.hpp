#pragma once

#include "platform/window.hpp"
#include <Eigen/Core?

struct InputState
{
    Eigen::Vector2f orbit_delta{0, 0};
    Eigen::Vector2f pan_delta{0, 0};
    float dolly_delta = 0.0f;

    bool single_click = false;
    bool double_click = false;
    Eigen::Vector2f click_pos{0, 0};
};

class InputManager
{
public:
    InputManager(Platform::Win32Window& window, int screen_height);

    InputState poll(float current_time);

private:
    // Callback
    void on_mouse_button(Platform::MouseButton button, bool pressed);
    void on_scroll(float offset);
    void on_cursor_pos(double xpos, double ypos);

    
}