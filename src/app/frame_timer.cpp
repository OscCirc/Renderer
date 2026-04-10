#include "app/frame_timer.hpp"
#include "platform/window.hpp"

void FrameTimer::begin_frame(float current_time)
{
    last_time_ = Platform::Win32Window::get_time();
}

float FrameTimer::curr_time() const
{
    return Platform::Win32Window::get_time();
}


