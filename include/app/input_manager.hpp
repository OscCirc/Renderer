#pragma once

#include "platform/window.hpp"
#include <Eigen/Core>


struct InputState
{
    Eigen::Vector2f orbit_delta{0, 0};
    Eigen::Vector2f pan_delta{0, 0};
    float dolly_delta = 0.0f;

    bool single_click = false;
    bool double_click = false;
    Eigen::Vector2f click_pos{0, 0};

    float light_theta;
    float light_phi;
};

class InputManager
{
public:
    InputManager(Platform::Win32Window& window);

    void poll(float current_time, InputState& state);
private:
    // Callback
    void on_mouse_button_pressed(Platform::MouseButton button, bool pressed);
    void on_scroll(float offset);
    void on_cursor_pos(double xpos, double ypos);

    // Getter
    Eigen::Vector2f get_cursor_position() const;
    Eigen::Vector2f get_pos_delta(const Eigen::Vector2f& old_pos, const Eigen::Vector2f& new_pos) const;

    // const definitions
    static constexpr float CLICK_DELAY = 0.25f;                 // double click delay (seconds)
    static constexpr float CLICK_DISTANCE_THRESHOLD = 5.0f;     // click distance threshold (pixels)

    Platform::Win32Window& window_;
    int width_;
    int height_;
    std::string title_;

    // Input Record
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

        void reset()
        {
            orbit_delta = { 0, 0 };
            pan_delta = { 0, 0 };
            dolly_delta = 0;
            single_click = false;
            double_click = false;
        }
    } input_record_;
};