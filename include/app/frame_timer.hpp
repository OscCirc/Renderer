#pragma once

class FrameTimer
{
public:
    void begin_frame(float current_time);

    float curr_time() const;
    float delta_time() const;
private:
    float last_time_ = 0.0f;
    float print_time_ = 0.0f;
    float delta_time_ = 0.0f;
    int frame_count_ = 0; 
};