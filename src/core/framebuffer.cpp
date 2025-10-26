#include "core/framebuffer.hpp"
#include <algorithm> // for std::clamp

Framebuffer::Framebuffer(int width, int height)
    : m_width(width), m_height(height)
{
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("Framebuffer dimensions must be positive.");
    }
    m_color_buffer.resize(width * height * 4);
    m_depth_buffer.resize(width * height);

    // 默认进行一次清理
    clear_color({0.0f, 0.0f, 0.0f, 1.0f});
    clear_depth(1.0f);
}

void Framebuffer::clear_color(const Eigen::Vector4f& color)
{
    unsigned char r = float_to_uchar(color.x());
    unsigned char g = float_to_uchar(color.y());
    unsigned char b = float_to_uchar(color.z());
    unsigned char a = float_to_uchar(color.w());

    for (size_t i = 0; i < m_color_buffer.size(); i += 4) {
        m_color_buffer[i + 0] = r;
        m_color_buffer[i + 1] = g;
        m_color_buffer[i + 2] = b;
        m_color_buffer[i + 3] = a;
    }
}

void Framebuffer::test(const Eigen::Vector4f& color, int index) {
    int patch = m_color_buffer.size() / 4;
    unsigned char r = float_to_uchar(color.x());
    unsigned char g = float_to_uchar(color.y());
    unsigned char b = float_to_uchar(color.z());
    unsigned char a = float_to_uchar(color.w());

    for (size_t i = index * patch; i < (index + 1) * patch; i += 4) {
        m_color_buffer[i + 0] = r;
        m_color_buffer[i + 1] = g;
        m_color_buffer[i + 2] = b;
        m_color_buffer[i + 3] = a;
    }
}

void Framebuffer::clear_depth(float depth)
{
    // std::fill 是更高效的写法
    std::fill(m_depth_buffer.begin(), m_depth_buffer.end(), depth);
}

const std::vector<unsigned char>& Framebuffer::get_color_buffer() const
{
    return m_color_buffer;
}

const std::vector<float>& Framebuffer::get_depth_buffer() const
{
    return m_depth_buffer;
}

std::vector<unsigned char>& Framebuffer::get_color_buffer()
{
    return m_color_buffer;
}

std::vector<float>& Framebuffer::get_depth_buffer()
{
    return m_depth_buffer;
}

int Framebuffer::get_width() const
{
    return m_width;
}

int Framebuffer::get_height() const
{
    return m_height;
}

unsigned char Framebuffer::float_to_uchar(float f)
{
    // 将 0.0-1.0 的浮点数映射到 0-255 的整数
    return static_cast<unsigned char>(std::clamp(f, 0.0f, 1.0f) * 255.0f);
}

Eigen::Vector4f Framebuffer::get_color(int index) const {
    index = index * 4;
    return Eigen::Vector4f(
        m_color_buffer[index + 0] / 255.0f,
        m_color_buffer[index + 1] / 255.0f,
        m_color_buffer[index + 2] / 255.0f,
        m_color_buffer[index + 3] / 255.0f
    );
}

float Framebuffer::get_depth(int index) const {
    return m_depth_buffer[index];
}

void Framebuffer::set_color(int index, const Eigen::Vector4f& color) {
    if (index < 0 || index > m_width * m_height - 1) {
        return;
    }
    index = index * 4;
    m_color_buffer[index + 0] = float_to_uchar(color.x());
    m_color_buffer[index + 1] = float_to_uchar(color.y());
    m_color_buffer[index + 2] = float_to_uchar(color.z());
    m_color_buffer[index + 3] = float_to_uchar(color.w());
}

void Framebuffer::set_depth(int index, float depth) {
    if (index < 0 || index > m_width * m_height - 1) {
        return;
    }

    m_depth_buffer[index] = depth;
}