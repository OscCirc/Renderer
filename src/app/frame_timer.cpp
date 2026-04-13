#include "app/frame_timer.hpp"
#include "platform/window.hpp"
#include <iostream>

void FrameTimer::begin_frame(float current_time)
{
    if(!start_)
    {
        last_time_ = current_time;
        print_time_ = current_time;
        
        start_ = true;
    }
    delta_time_ = current_time - last_time_;
    last_time_ = current_time;
}

void FrameTimer::maybe_print_fps()
{
    frame_count_ += 1;
    if (last_time_ - print_time_ >= 1) {
        int sum_millis = (int)((last_time_ - print_time_) * 1000);
        int avg_millis = sum_millis / frame_count_;
        std::cout << "fps: " << frame_count_ << ", avg: " << avg_millis << " ms" << std::endl;
        frame_count_ = 0;
        print_time_ = last_time_;
    }
}

