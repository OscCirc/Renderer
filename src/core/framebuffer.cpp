#include "core/framebuffer.hpp"

Framebuffer::Framebuffer(int width, int height)
    : width_(width), height_(height)
{
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("Framebuffer dimensions must be positive.");
    }
    color_buffer_.resize(width * height);
    depth_buffer_.resize(width * height);

    // 默认进行一次清理
    clear_color({0.0f, 0.0f, 0.0f, 1.0f});
    clear_depth(1.0f);
}

void Framebuffer::clear_color(const Eigen::Vector4f& color)
{
    for (size_t i = 0; i < color_buffer_.size(); i++) {
        color_buffer_[i] = color;
    }
}

void Framebuffer::clear_depth(float depth)
{
    // std::fill 是更高效的写法
    std::fill(depth_buffer_.begin(), depth_buffer_.end(), depth);
}

const std::vector<Eigen::Vector4f>& Framebuffer::get_color_buffer() const
{
    return color_buffer_;
}

const std::vector<float>& Framebuffer::get_depth_buffer() const
{
    return depth_buffer_;
}

std::vector<Eigen::Vector4f>& Framebuffer::get_color_buffer()
{
    return color_buffer_;
}

std::vector<float>& Framebuffer::get_depth_buffer()
{
    return depth_buffer_;
}

int Framebuffer::get_width() const
{
    return width_;
}

int Framebuffer::get_height() const
{
    return height_;
}

Eigen::Vector4f Framebuffer::get_color(int index) const {
    return color_buffer_[index];
}

float Framebuffer::get_depth(int index) const {
    return depth_buffer_[index];
}

void Framebuffer::set_color(int index, const Eigen::Vector4f& color) {
    if (index < 0 || index > width_ * height_ - 1) {
        return;
    }
    color_buffer_[index] = color;
}

void Framebuffer::set_depth(int index, float depth) {
    if (index < 0 || index > width_ * height_ - 1) {
        return;
    }

    depth_buffer_[index] = depth;
}