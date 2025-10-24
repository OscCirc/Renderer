#pragma once

#include <string>
#include <vector>
#include <memory>

enum class ImageFormat {
    LDR,
    HDR
};

class Image {
public:
    // 工厂函数：从文件加载图像
    Image(const std::string& filename);

    // 构造函数：创建一个空白图像
    Image(int width, int height, int channels, ImageFormat format);

    // 保存图像到文件
    bool save(const std::string& filename) const;

    // 图像处理
    void flip_h();
    void flip_v();

    // 访问器
    int get_width() const { return width; }
    int get_height() const { return height; }
    int get_channels() const { return channels; }
    ImageFormat get_format() const { return format; }

    // 获取原始像素数据指针
    const unsigned char* get_ldr_buffer() const;
    unsigned char* get_ldr_buffer();
    const float* get_hdr_buffer() const;
    float* get_hdr_buffer();

private:
    ImageFormat format;
    int width;
    int height;
    int channels;
    std::vector<unsigned char> ldr_buffer;
    std::vector<float> hdr_buffer;
};

void convertBGRAtoRGBA(unsigned char* data, int& width, int& height);
unsigned char* loadRGBA(const std::string& filename,
    int& width,
    int& height,
    bool flipVertical = true);