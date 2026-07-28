#pragma once

#include <vector>
#include <Eigen/Core>

class Framebuffer
{
public:
    Framebuffer(int width, int height);

    ~Framebuffer() = default;

    // 禁止拷贝
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    // 允许移动
    Framebuffer(Framebuffer&&) = default;
    Framebuffer& operator=(Framebuffer&&) = default;

    // 清理缓冲区
    void clear_color(const Eigen::Vector4f& color);
    void clear_depth(float depth = 1.0f);

    // 获取缓冲区数据指针（给渲染器或窗口系统使用）
    std::vector<Eigen::Vector4f>& get_color_buffer();
    std::vector<float>& get_depth_buffer();

    const std::vector<Eigen::Vector4f>& get_color_buffer() const;
    const std::vector<float>& get_depth_buffer() const;

    // 获取颜色像素（支持读取单个像素）
    Eigen::Vector4f get_color(int index) const;
    float get_depth(int index) const;
    // 设置颜色像素
    void set_color(int index, const Eigen::Vector4f& color);
    void set_depth(int index, float depth);

    // 获取尺寸
    int get_width() const;
    int get_height() const;

private:
    int width_;
    int height_;
    std::vector<Eigen::Vector4f> color_buffer_;    // RBGA: RBG in [0, \infinity), A in [0,1]
    std::vector<float> depth_buffer_;
};