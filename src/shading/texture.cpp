#include "shading/texture.hpp"
#include "core/framebuffer.hpp"

float float_from_uchar(unsigned char c)
{
    return static_cast<float>(c) / 255.0f;
}
// 辅助函数：sRGB to Linear
static float srgb_to_linear(float c)
{
    return (c <= 0.04045f) ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
}
void Texture::ldr_to_texture(const Image& image, TextureUsage usage)
{
    int num_of_pixels = width_ * height_;
    for (int i = 0; i < num_of_pixels; ++i) {
        int channel = image.get_channels();
        Eigen::Vector4f pixel = { 0, 0, 0, 255 };
        for (int j = 0; j < channel; ++j) {
            pixel[j] = image.get_ldr_buffer()[i * channel + j] / 255.0f;
            if (usage == TextureUsage::Linear) {
                pixel.x() = srgb_to_linear(pixel.x());
                pixel.y() = srgb_to_linear(pixel.y());
                pixel.z() = srgb_to_linear(pixel.z());
            }
        }
        buffer_[i] = pixel;
    }
}

void Texture::hdr_to_texture(const Image& image)
{
    int num_of_pixels = width_ * height_;
    for (int i = 0; i < num_of_pixels; ++i) {
        int channel = image.get_channels();
        Eigen::Vector4f pixel = { 0, 0, 0, 1 };
        for (int j = 0; j < channel; ++j) {
            pixel[j] = image.get_hdr_buffer()[i * channel + j];
        }
        buffer_[i] = pixel;
    }
}

void Texture::update_from_color_buffer(const Framebuffer& framebuffer)
{
    assert(width_ == framebuffer.get_width() && "Texture and Framebuffer width mismatch");
    assert(height_ == framebuffer.get_height() && "Texture and Framebuffer height mismatch");

    int nuEIGEN_PIxels = width_ * height_;
    buffer_.resize(nuEIGEN_PIxels);

    for (int i = 0; i < nuEIGEN_PIxels; ++i)
    {
        const std::vector<unsigned char>& color = framebuffer.get_color_buffer();
        float r = float_from_uchar(color[i*4 + 0]);
        float g = float_from_uchar(color[i*4 + 1]);
        float b = float_from_uchar(color[i*4 + 2]);
        float a = float_from_uchar(color[i*4 + 3]);
        buffer_[i] = Eigen::Vector4f(r, g, b, a);
    }
}

void Texture::update_from_depth_buffer(const Framebuffer& framebuffer)
{
    assert(width_ == framebuffer.get_width() && "Texture and Framebuffer width mismatch");
    assert(height_ == framebuffer.get_height() && "Texture and Framebuffer height mismatch");

    int nuEIGEN_PIxels = width_ * height_;
    buffer_.resize(nuEIGEN_PIxels);

    for (int i = 0; i < nuEIGEN_PIxels; ++i)
    {
        float depth = framebuffer.get_depth_buffer()[i];
        // 将深度值存入纹理的 RGBA 通道
        buffer_[i] = Eigen::Vector4f(depth, depth, depth, 1.0f);
    }
}