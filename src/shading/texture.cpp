#include "utils/color_space.hpp"
#include "shading/texture.hpp"
#include "core/framebuffer.hpp"
#include <eigen3/Eigen/Eigen>

SampleMode g_sample_mode = SampleMode::Trilinear;

float float_from_uchar(unsigned char c)
{
    return static_cast<float>(c) / 255.0f;
}

void Texture::ldr_to_texture(const Image& image, TextureColorSpace color_space)
{
    const auto& width_ = mipmaps_[0].width;
    const auto& height_ = mipmaps_[0].height;
    int num_of_pixels = width_ * height_;
    int channels = image.get_channels();
    
    for (int i = 0; i < num_of_pixels; ++i) {
        Eigen::Vector4f pixel = { 0, 0, 0, 1 };
        const auto* source = image.get_ldr_buffer() + i * channels;

        switch (channels)
        {
        case 1: // 灰度
            pixel.head<3>().setConstant(source[0] / 255.0f);
            break;
        case 2: // 灰度 + alpha                
            pixel.head<3>().setConstant(source[0] / 255.0f);
            pixel.w() = source[1] / 255.0f;
            break;
        case 3: // RGB
            pixel.x() = source[0] / 255.0f;
            pixel.y() = source[1] / 255.0f;
            pixel.z() = source[2] / 255.0f;
            break;

        case 4: // RGBA
            pixel.x() = source[0] / 255.0f;
            pixel.y() = source[1] / 255.0f;
            pixel.z() = source[2] / 255.0f;
            pixel.w() = source[3] / 255.0f;
            break;            
        
        default:
            throw std::runtime_error("Unsupported LDR channel count");
        }
        if (color_space == TextureColorSpace::sRGB) {
            pixel.x() = ColorSpace::srgb_to_linear(pixel.x());
            pixel.y() = ColorSpace::srgb_to_linear(pixel.y());
            pixel.z() = ColorSpace::srgb_to_linear(pixel.z());
        } 
        mipmaps_[0].data[i] = pixel;
    }
}

void Texture::hdr_to_texture(const Image& image)
{
    const auto& width_ = mipmaps_[0].width;
    const auto& height_ = mipmaps_[0].height;
    int num_of_pixels = width_ * height_;
    int channels = image.get_channels();

    for (int i = 0; i < num_of_pixels; ++i) {
        Eigen::Vector4f pixel = { 0, 0, 0, 1 };
        const auto* source = image.get_hdr_buffer() + i * channels;

        switch (channels)
        {
        case 1: // 灰度
            pixel.head<3>().setConstant(source[0]);
            break;
        case 2: // 灰度 + alpha                
            pixel.head<3>().setConstant(source[0]);
            pixel.w() = source[1];
            break;
        case 3: // RGB
            pixel.x() = source[0];
            pixel.y() = source[1];
            pixel.z() = source[2];
            break;

        case 4: // RGBA
            pixel.x() = source[0];
            pixel.y() = source[1];
            pixel.z() = source[2];
            pixel.w() = source[3];
            break;            
        
        default:
            throw std::runtime_error("Unsupported HDR channel count");
        }
        
        mipmaps_[0].data[i] = pixel;
    }
}

void Texture::to_texture(
    const Image& image,
    ImageFormat format,
    TextureColorSpace color_space)
{
    switch (format)
    {
    case ImageFormat::LDR:
        ldr_to_texture(image, color_space);
        return;

    case ImageFormat::HDR:
        // .hdr 读取出的 float 数据默认视为线性 HDR 颜色
        hdr_to_texture(image);
        return;
    }

    throw std::runtime_error("Unsupported image format");
}

Eigen::Vector4f Texture::sample_nearest(float u, float v, int level) const
{
    const auto& mip = mipmaps_[level];
    float u_img = u * mip.width - 0.5f;
    float v_img = v * mip.height - 0.5f;
    int x = std::clamp(static_cast<int>(std::round(u_img)), 0, mip.width - 1);
    int y = std::clamp(static_cast<int>(std::round(v_img)), 0, mip.height - 1);
    return mip.data[y * mip.width + x];
}

Eigen::Vector4f Texture::sample_bilinear(float u, float v, int level) const
{
    const auto& width_ = mipmaps_[level].width;
    const auto& height_ = mipmaps_[level].height;
    const auto& buffer_ = mipmaps_[level].data;

    auto u_img = u * width_ - 0.5f;
    auto v_img = v * height_ - 0.5f;

    // 确保坐标在有效范围内
    u_img = std::max(0.0f, static_cast<float>(u_img));
    v_img = std::max(0.0f, static_cast<float>(v_img));

    int x0 = static_cast<int>(u_img);
    int y0 = static_cast<int>(v_img);

    int x1 = std::min(x0 + 1, width_ - 1);
    int y1 = std::min(y0 + 1, height_ - 1);

    float tx = u_img - x0;
    float ty = v_img - y0;

    auto c00 = buffer_[y0 * width_ + x0];
    auto c10 = buffer_[y0 * width_ + x1];
    auto c01 = buffer_[y1 * width_ + x0];
    auto c11 = buffer_[y1 * width_ + x1];

    auto top = c00 * (1.0f - tx) + c10 * tx;
    auto bottom = c01 * (1.0f - tx) + c11 * tx;

    return top * (1.0f - ty) + bottom * ty;
}

Eigen::Vector4f Texture::sample_trilinear(float u, float v, float lod) const
{
    float max_level = static_cast<float>(mipmaps_.size() - 1);
    lod = std::clamp(lod, 0.0f, max_level);

    int level0 = static_cast<int>(std::floor(lod));
    int level1 = std::min(level0 + 1, static_cast<int>(max_level));

    float frac = lod - static_cast<float>(level0);

    auto c0 = sample_bilinear(u, v, level0);
    auto c1 = sample_bilinear(u, v, level1);

    return frac * c1 + (1 - frac) * c0;
}

std::shared_ptr<Texture> Texture::create_checkerboard(int size, int grid)
{
    auto tex = std::make_shared<Texture>();
    tex->mipmaps_.push_back(MipLevel{});
    auto& base = tex->mipmaps_[0];
    base.width = size;
    base.height = size;
    base.data.resize(size * size);

    int cell_size = size / grid;
    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            int cx = x / cell_size;
            int cy = y / cell_size;
            bool white = (cx + cy) % 2 == 0;
            float c = white ? 1.0f : 0.0f;
            base.data[y * size + x] = Eigen::Vector4f(c, c, c, 1.0f);
        }
    }

    tex->generate_mipmaps();
    return tex;
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

    const std::vector<Eigen::Vector4f>& color = framebuffer.get_color_buffer();
    for (int i = 0; i < nuEIGEN_PIxels; ++i)
    {
        mipmaps_[0].data[i] = color[i];
    }

    mipmaps_.resize(1);
    generate_mipmaps();
}

void Texture::update_from_depth_buffer(const Framebuffer& framebuffer)
{
    const auto& width_ = mipmaps_[0].width;
    const auto& height_ = mipmaps_[0].height;
    assert(width_ == framebuffer.get_width() && "Texture and Framebuffer width mismatch");
    assert(height_ == framebuffer.get_height() && "Texture and Framebuffer height mismatch");

    int nuEIGEN_PIxels = width_ * height_;
    mipmaps_[0].data.resize(nuEIGEN_PIxels);

    const std::vector<float>& depth_buffer = framebuffer.get_depth_buffer();
    for (int i = 0; i < nuEIGEN_PIxels; ++i)
    {
        float depth = depth_buffer[i];
        // 将深度值存入纹理的 RGBA 通道
        mipmaps_[0].data[i] = Eigen::Vector4f(depth, depth, depth, 1.0f);
    }
}