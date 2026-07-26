#include "core/image.hpp"
#include <stdexcept>
#include <algorithm>
#include <cassert>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// 辅助函数：获取文件扩展名
static std::string get_extension(const std::string& filename) {
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return "";
    }
    std::string ext = filename.substr(dot_pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

template <typename T>
static void flip_h_impl(std::vector<T>& buf, int w, int h, int c)
{
    const int half_w = w / 2;
    const size_t pitch = static_cast<size_t>(w) * c;
    for(int i = 0; i < h; i++)
    {
        for(int j = 0; j < half_w; j++)
        {
            for(int k = 0; k < c; k++)
            {
                std::swap(buf[i * pitch + j * c + k],
                          buf[i * pitch + (w - 1 - j) * c + k]);
            }
        }
    }
}

template <typename T>
static void flip_v_impl(std::vector<T>& buf, int w, int h, int c)
{
    const size_t row_size = static_cast<size_t>(w) * c;
    const int half_h = h / 2;
    for(int row = 0; row < half_h; row++)
    {
        std::swap_ranges(
            buf.begin() + row * row_size,
            buf.begin() + (row + 1) * row_size,
            buf.begin() + (h - 1 - row) * row_size
        );
    }
}
void convertBGRAtoRGBA(unsigned char* data, int& width, int& height) {
    const int totalPixels = width * height;
    for (int i = 0; i < totalPixels; i++) {
        unsigned char* pixel = data + i * 4;
        std::swap(pixel[0], pixel[2]); // 交换R和B通道
    }
}
unsigned char* loadRGBA(const std::string& filename,
    int& width,
    int& height,
    bool flipVertical) {

    // 设置垂直翻转（可选）
    stbi_set_flip_vertically_on_load(flipVertical);

    // 加载图像并强制4通道
    int origChannels;
    unsigned char* data = stbi_load(filename.c_str(),
        &width,
        &height,
        &origChannels,
        4);

    if (!data) return nullptr;

    // 根据原始格式转换通道顺序
    std::string ext = filename.substr(filename.find_last_of(".") + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == "tga" || ext == "bmp") {
        // TGA和BMP是BGRA顺序，转换为RGBA
        convertBGRAtoRGBA(data, width, height);
    }
    // 其他格式如PNG、JPG通常是RGBA或RGB，不需要转换

    return data;
}

// --- Image 类实现 ---

Image::Image(int width, int height, int channels, ImageFormat format)
    : format(format), width(width), height(height), channels(channels) {
    if (width <= 0 || height <= 0 || channels <= 0) {
        throw std::invalid_argument("Image dimensions must be positive.");
    }
    size_t num_elements = static_cast<size_t>(width) * height * channels;
    if (format == ImageFormat::LDR) {
        ldr_buffer.resize(num_elements);
    }
    else {
        hdr_buffer.resize(num_elements);
    }
}

Image::Image(const std::string& filename) {

    stbi_set_flip_vertically_on_load(true);

    if (stbi_is_hdr(filename.c_str())) {
        format = ImageFormat::HDR;
        float* data = stbi_loadf(filename.c_str(), &width, &height, &channels, 0);
        if (!data) {
            throw std::runtime_error("Failed to load HDR image: " + filename);
        }
        hdr_buffer.assign(data, data + static_cast<size_t>(width) * height * channels);
        stbi_image_free(data);
    }
    else {
        format = ImageFormat::LDR;
        unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 0);
        if (!data) {
            throw std::runtime_error("Failed to load LDR image: " + filename);
        }

        ldr_buffer.assign(data, data + static_cast<size_t>(width) * height * channels);
        stbi_image_free(data);
    }
}

bool Image::save(const std::string& filename) const {
    stbi_flip_vertically_on_write(true);
    std::string ext = get_extension(filename);
    int result = 0;
    if (ext == "tga") {
        assert(format == ImageFormat::LDR);
        result = stbi_write_tga(filename.c_str(), width, height, channels, ldr_buffer.data());
    }
    else if (ext == "hdr") {
        assert(format == ImageFormat::HDR);
        result = stbi_write_hdr(filename.c_str(), width, height, channels, hdr_buffer.data());
    }
    else if (ext == "png") {
        assert(format == ImageFormat::LDR);
        result = stbi_write_png(filename.c_str(), width, height, channels, ldr_buffer.data(), width * channels);
    }
    else if (ext == "jpg" || ext == "jpeg") {
        assert(format == ImageFormat::LDR);
        result = stbi_write_jpg(filename.c_str(), width, height, channels, ldr_buffer.data(), 90); // 90% quality
    }
    return result != 0;
}

void Image::flip_h() {
    if (format == ImageFormat::LDR) {
        flip_h_impl(ldr_buffer, width, height, channels);
    }
    else {
        flip_h_impl(hdr_buffer, width, height, channels);
    }
}

void Image::flip_v() {
    if (format == ImageFormat::LDR) {
        flip_v_impl(ldr_buffer, width, height, channels);
    }
    else {
        flip_v_impl(hdr_buffer, width, height, channels);
    }
}

const unsigned char* Image::get_ldr_buffer() const {
    return ldr_buffer.data();
}

unsigned char* Image::get_ldr_buffer() {
    return ldr_buffer.data();
}

const float* Image::get_hdr_buffer() const {
    return hdr_buffer.data();
}

float* Image::get_hdr_buffer() {
    return hdr_buffer.data();
}