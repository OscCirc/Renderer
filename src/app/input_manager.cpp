#include "app/input_manager.hpp"
#include "platform/window.hpp"
#include <Eigen/Core>

InputManager::InputManager(Platform::Win32Window& window)
    : window_(window), width_(window.get_width()), height_(window.get_height())
{
    window_.set_mouse_button_callback([this](Platform::MouseButton button, bool pressed) {
        this->on_mouse_button_pressed(button, pressed);
        });

    window_.set_scroll_callback([this](float offset) {
        this->on_scroll(offset);
        });

    window_.set_cursor_pos_callback([this](double xpos, double ypos) {
        this->on_cursor_pos(xpos, ypos);
        });
}

void InputManager::poll(float current_time, InputState& state)
{
    float last_time = input_record_.release_time;
    if (last_time > 0 && current_time - last_time > CLICK_DELAY) {
        Eigen::Vector2f pos_delta = input_record_.release_pos - input_record_.press_pos;
        if (pos_delta.norm() < CLICK_DISTANCE_THRESHOLD) {
            input_record_.single_click = true;
        }
        input_record_.release_time = 0;
    }

    if (input_record_.single_click || input_record_.double_click) {
        float click_x = input_record_.release_pos.x() / static_cast<float>(width_);
        float click_y = input_record_.release_pos.y() / static_cast<float>(height_);
        input_record_.click_pos = Eigen::Vector2f(click_x, 1.0f - click_y);
    }


    state.orbit_delta = input_record_.orbit_delta;
    state.pan_delta = input_record_.pan_delta;
    state.dolly_delta = input_record_.dolly_delta;
    state.single_click = input_record_.single_click;
    state.double_click = input_record_.double_click;
    state.click_pos = input_record_.click_pos;

    input_record_.reset();
}

Eigen::Vector2f InputManager::get_cursor_position() const
{
    float x, y;
    window_.get_cursor_position(x, y);
    return Eigen::Vector2f(x, y);
}

Eigen::Vector2f InputManager::get_pos_delta(const Eigen::Vector2f& old_pos, const Eigen::Vector2f& new_pos) const {
    Eigen::Vector2f delta = new_pos - old_pos;
    return delta / static_cast<float>(height_);
}

void InputManager::on_mouse_button_pressed(Platform::MouseButton button, bool pressed) {
    Eigen::Vector2f cursor_pos = get_cursor_position();

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

            // double click detection
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

void InputManager::on_scroll(float offset)
{
    input_record_.dolly_delta += offset;
}

void InputManager::on_cursor_pos(double xpos, double ypos) {
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
