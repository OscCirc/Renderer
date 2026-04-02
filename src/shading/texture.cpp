#include "shading/texture.hpp"
#include "core/framebuffer.hpp"
#include <eigen3/Eigen/Eigen>

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
    const auto& width_ = mipmaps_[0].width;
    const auto& height_ = mipmaps_[0].height;
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
        mipmaps_[0].data[i] = pixel;
    }
}

void Texture::hdr_to_texture(const Image& image)
{
    const auto& width_ = mipmaps_[0].width;
    const auto& height_ = mipmaps_[0].height;
    int num_of_pixels = width_ * height_;
    for (int i = 0; i < num_of_pixels; ++i) {
        int channel = image.get_channels();
        Eigen::Vector4f pixel = { 0, 0, 0, 1 };
        for (int j = 0; j < channel; ++j) {
            pixel[j] = image.get_hdr_buffer()[i * channel + j];
        }
        mipmaps_[0].data[i] = pixel;
    }
}

void Texture::generate_mipmaps()
{
    int level = 0;
    while (mipmaps_[level].width > 1 || mipmaps_[level].height > 1)
    {
        const auto& prev = mipmaps_[level];
        MipLevel next;

        int width = std::max(1, mipmaps_[level].width / 2);
        int height = std::max(1, mipmaps_[level].height / 2);
        next.data.resize(width * height);
        next.width = width;
        next.height = height;

        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int x0 = std::min(2 * x, prev.width - 1);
                int x1 = std::min(2 * x + 1, prev.width - 1);
                int y0 = std::min(2 * y, prev.height - 1);
                int y1 = std::min(2 * y + 1, prev.height - 1);

                auto c00 = mipmaps_[level].data[y0 * prev.width + x0];
                auto c01 = mipmaps_[level].data[y0 * prev.width + x1];
                auto c10 = mipmaps_[level].data[y1 * prev.width + x0];
                auto c11 = mipmaps_[level].data[y1 * prev.width + x1];

                next.data[y * width + x] = (c00 + c01 + c10 + c11) * 0.25f;
            }
        }

        mipmaps_.push_back(std::move(next));

        level++;
    }
}

void Texture::update_from_color_buffer(const Framebuffer& framebuffer)
{
    const auto& width_ = mipmaps_[0].width;
    const auto& height_ = mipmaps_[0].height;
    assert(width_ == framebuffer.get_width() && "Texture and Framebuffer width mismatch");
    assert(height_ == framebuffer.get_height() && "Texture and Framebuffer height mismatch");

    int nuEIGEN_PIxels = width_ * height_;
    mipmaps_[0].data.resize(nuEIGEN_PIxels);

    for (int i = 0; i < nuEIGEN_PIxels; ++i)
    {
        const std::vector<unsigned char>& color = framebuffer.get_color_buffer();
        float r = float_from_uchar(color[i*4 + 0]);
        float g = float_from_uchar(color[i*4 + 1]);
        float b = float_from_uchar(color[i*4 + 2]);
        float a = float_from_uchar(color[i*4 + 3]);
        mipmaps_[0].data[i] = Eigen::Vector4f(r, g, b, a);
    }
}

void Texture::update_from_depth_buffer(const Framebuffer& framebuffer)
{
    const auto& width_ = mipmaps_[0].width;
    const auto& height_ = mipmaps_[0].height;
    assert(width_ == framebuffer.get_width() && "Texture and Framebuffer width mismatch");
    assert(height_ == framebuffer.get_height() && "Texture and Framebuffer height mismatch");

    int nuEIGEN_PIxels = width_ * height_;
    mipmaps_[0].data.resize(nuEIGEN_PIxels);

    for (int i = 0; i < nuEIGEN_PIxels; ++i)
    {
        float depth = framebuffer.get_depth_buffer()[i];
        // 将深度值存入纹理的 RGBA 通道
        mipmaps_[0].data[i] = Eigen::Vector4f(depth, depth, depth, 1.0f);
    }
}