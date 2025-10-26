#pragma once

#include "utils/global.hpp"
#include "core/image.hpp"
#include <eigen3/Eigen/Eigen>
#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <mutex>


class Framebuffer;

// 定义纹理用途，用于颜色空间转换
enum class TextureUsage
{
    Generic, // 通用数据，不做转换
    Color,   // LDR 颜色纹理 (sRGB)
    Linear   // 线性数据，如法线贴图、金属度贴图
};

// 定义纹理环绕（采样）模式
enum class WrapMode
{
    Repeat,     // 重复
    ClampToEdge // 钳制到边缘
};

class Texture
{
private:
    int width_ = 0;
    int height_ = 0;
    std::vector<Eigen::Vector4f> buffer_;

    void ldr_to_texture(const Image& image, TextureUsage usage);

    void hdr_to_texture(const Image& image);
public:
    // 默认构造函数
    Texture() = default;

    // 从文件加载的构造函数
    Texture(const std::string &filename, TextureUsage usage = TextureUsage::Color)
    {
        Image image(filename);

        width_ = image.get_width();
        height_ = image.get_height();
        buffer_.resize(width_ * height_);

        if (image.get_format() == ImageFormat::LDR) {
            ldr_to_texture(image, usage);
        }
        else if (image.get_format() == ImageFormat::HDR) {
            hdr_to_texture(image);
        }
        else {
            throw std::runtime_error("Unsupported texture format for file: " + filename);
        }
        
    }

    Texture(int width, int height){
        assert(width > 0 && height > 0);
        width_ = width;
        height_ = height;
        buffer_.resize(width_ * height_);
    }

    void update_from_color_buffer(const Framebuffer& framebuffer);
    void update_from_depth_buffer(const Framebuffer& framebuffer);

    // 获取尺寸
    int width() const { return width_; }
    int height() const { return height_; }
    bool is_valid() const { return !buffer_.empty(); }

    // 纹理采样（最近邻）
    Eigen::Vector4f sample(float u, float v, WrapMode mode = WrapMode::Repeat) const
    {
        if (!is_valid())
            return {0, 0, 0, 0};

        if (mode == WrapMode::Repeat)
        {
            u = u - floor(u);
            v = v - floor(v);
        }
        else
        { // ClampToEdge
            u = std::clamp(u, 0.0f, 1.0f);
            v = std::clamp(v, 0.0f, 1.0f);
        }

        auto u_img = u * (width_ - 1);
        auto v_img = v * (height_ - 1);

        int x = static_cast<int>(u_img);
        int y = static_cast<int>(v_img);

        return buffer_[y * width_ + x];
    }

};

class TextureCache{
public:

    static TextureCache& get_instance() {
        static TextureCache instance;
        return instance;
    }

    std::shared_ptr<Texture> acquire(const std::string& filename, TextureUsage usage) {
        if (filename.empty()) {
            return nullptr;
        }

        // 加锁以保证线程安全
        std::lock_guard<std::mutex> lock(cache_mutex_);

        auto it = cache_.find(filename);
        if (it != cache_.end()) {
            if (auto shared = it->second.lock()) {
                // 缓存命中且对象仍然存活
                return shared;
            }
        }

        auto new_Texture = std::make_shared<Texture>(filename, usage);
        cache_[filename] = new_Texture; // 存入 weak_ptr
        return new_Texture;
    }
private:
    TextureCache() = default;
    ~TextureCache() = default;
    TextureCache(const TextureCache&) = delete;
    TextureCache& operator=(const TextureCache&) = delete;

    std::unordered_map<std::string, std::weak_ptr<Texture>> cache_;
    std::mutex cache_mutex_;
};

class Cubemap
{
private:
    std::array<std::shared_ptr<Texture>, 6> faces_;

    // 辅助结构体，用于 select_face 的返回值
    struct FaceSelection
    {
        int index;
        Eigen::Vector2f texcoord;
    };

    // 辅助函数：根据方向选择面和计算纹理坐标
    static FaceSelection select_face(const Eigen::Vector3f &direction)
    {
        float abs_x = std::abs(direction.x());
        float abs_y = std::abs(direction.y());
        float abs_z = std::abs(direction.z());
        float ma, sc, tc;
        int face_index;

        if (abs_x > abs_y && abs_x > abs_z)
        { // 主轴 -> x
            ma = abs_x;
            if (direction.x() > 0)
            { // +X
                face_index = 0; sc = -direction.z(); tc = -direction.y();
            }
            else
            { // -X
                face_index = 1; sc = +direction.z(); tc = -direction.y();
            }
        }
        else if (abs_y > abs_z)
        { // 主轴 -> y
            ma = abs_y;
            if (direction.y() > 0)
            { // +Y
                face_index = 2; sc = +direction.x(); tc = +direction.z();
            }
            else
            { // -Y
                face_index = 3; sc = +direction.x(); tc = -direction.z();
            }
        }
        else
        { // 主轴 -> z
            ma = abs_z;
            if (direction.z() > 0)
            { // +Z
                face_index = 4; sc = +direction.x(); tc = -direction.y();
            }
            else
            { // -Z
                face_index = 5; sc = -direction.x(); tc = -direction.y();
            }
        }

        Eigen::Vector2f texcoord((sc / ma + 1.0f) * 0.5f, (tc / ma + 1.0f) * 0.5f);
        return {face_index, texcoord};
    }

public:
    // 构造函数：从六个文件加载
    Cubemap(const std::array<std::string, 6> &filenames, TextureUsage usage = TextureUsage::Color)
    {
        for (int i = 0; i < 6; ++i)
        {
            faces_[i] = TextureCache::get_instance().acquire(filenames[i], usage);
        }
    }

    // 默认析构函数即可，shared_ptr 会自动管理内存
    ~Cubemap() = default;

    // Cubemap 采样
    Eigen::Vector4f sample(const Eigen::Vector3f &direction, WrapMode mode = WrapMode::Repeat) const
    {
        FaceSelection selection = select_face(direction);
        const auto &face = faces_[selection.index];

        if (face)
        {
            return face->sample(selection.texcoord.x(), selection.texcoord.y(), mode);
        }
        return {0, 0, 0, 1}; // 如果纹理无效，返回黑色
    }
};

class Cubemap_cache
{
public:
    static Cubemap_cache& get_instance() {
        static Cubemap_cache instance;
        return instance;
    }

    std::shared_ptr<Cubemap> acquire(const std::string& base_name,
        int blur_level = -1)
    {
        if (base_name.empty()) {
            return nullptr;
        }

        // 根据blur_level生成文件名后缀
        std::string prefix;
        if (blur_level == -1) {
            prefix = "i";
        }
        else if (blur_level == 1) {
            prefix = "m1";
        }
        else {
            prefix = "m0"; // 默认blur_level=0
        }

        // 生成六个面的文件名（根据C项目的命名规则）
        const std::array<std::string, 6> face_suffixes = {
            "px", "nx", "py", "ny", "pz", "nz"
        };

        std::array<std::string, 6> filenames;
        for (int i = 0; i < 6; i++) {
            filenames[i] = base_name + "/" + prefix + "_" + face_suffixes[i] + ".hdr";
        }

        // 创建组合键：基础名称+模糊级别
        std::string key = base_name + "_" + std::to_string(blur_level);

        // 加锁以保证线程安全
        std::lock_guard<std::mutex> lock(cache_mutex_);

        // 尝试从缓存中获取
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            if (auto shared = it->second.lock()) {
                return shared;
            }
        }

        auto new_cubemap = std::make_shared<Cubemap>(filenames, TextureUsage::Color);
        cache_[key] = new_cubemap;
        return new_cubemap;
    }
private:
    Cubemap_cache() = default;
    ~Cubemap_cache() = default;
    Cubemap_cache(const Cubemap_cache&) = delete;
    Cubemap_cache& operator=(const Cubemap_cache&) = delete;

    std::unordered_map<std::string, std::weak_ptr<Cubemap>> cache_;
    std::mutex cache_mutex_;
};


inline std::shared_ptr<Texture> acquire_color_texture(const std::string& filename)
{
    return TextureCache::get_instance().acquire(filename, TextureUsage::Color);
}

inline std::shared_ptr<Cubemap> acquire_cubemap(const std::string& skybox_name, int blur_level) {
    return Cubemap_cache::get_instance().acquire(skybox_name, blur_level);
}