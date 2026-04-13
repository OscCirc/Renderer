#pragma once

class FrameTimer
{
public:
    void begin_frame(float current_time);
    void maybe_print_fps();

    float current_time() const {return last_time_;}
    float delta_time() const {return delta_time_;}
private:
    float last_time_ = 0.0f;
    float print_time_ = 0.0f;
    float delta_time_ = 0.0f;
    int frame_count_ = 0; 

    bool start_ = false;
};