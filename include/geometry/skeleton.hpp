#pragma once

#include <vector>
#include <string>
#include <memory>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <mutex>
#include <unordered_map>
#include <iostream> 

// 前向声明
class Skeleton;

// 关键帧模板
template<typename T>
struct Keyframe {
    float time;
    T value;
};

// 骨骼类
class Joint {
public:
    int parent_index = -1;
    Eigen::Matrix4f inverse_bind_matrix;
    Eigen::Matrix4f global_transform; // 最终应用到顶点的变换

    std::vector<Keyframe<Eigen::Vector3f>> translations;
    std::vector<Keyframe<Eigen::Quaternionf>> rotations;
    std::vector<Keyframe<Eigen::Vector3f>> scales;

    // 根据时间计算局部变换矩阵
    Eigen::Matrix4f get_local_transform(float frame_time) const;
};

// 骨骼动画系统类
class Skeleton {
private:
    float min_time_ = 0.0f;
    float max_time_ = 0.0f;
    float last_time_ = -1.0f;
    std::vector<Joint> joints_;

    // 缓存的最终矩阵
    std::vector<Eigen::Matrix4f> joint_matrices_;
    std::vector<Eigen::Matrix3f> normal_matrices_;

    // 私有构造函数，只能通过 load 工厂方法创建
    Skeleton(float min_time, float max_time, std::vector<Joint>&& joints);

    // 私有加载逻辑
    static std::unique_ptr<Skeleton> load_ani(const std::string& filename);

public:
    // 公共静态工厂方法
    static std::unique_ptr<Skeleton> load(const std::string& filename);

    // 禁用拷贝
    Skeleton(const Skeleton&) = delete;
    Skeleton& operator=(const Skeleton&) = delete;
    // 默认移动
    Skeleton(Skeleton&&) = default;
    Skeleton& operator=(Skeleton&&) = default;
    ~Skeleton() = default;

    // 更新所有骨骼的变换
    void update(float frame_time);

    // 获取矩阵数组
    std::vector<Eigen::Matrix4f>& get_joint_matrices();
    std::vector<Eigen::Matrix3f>& get_normal_matrices();
};


class SkeletonCache
{
public:
    // 获取单例实例
    static SkeletonCache& get_instance()
    {
        static SkeletonCache instance;
        return instance;
    }

    // 获取资源的 shared_ptr
    std::shared_ptr<Skeleton> acquire(const std::string& filename)
    {
        if (filename.empty())
        {
            return nullptr;
        }

        // 加锁以保证线程安全
        std::lock_guard<std::mutex> lock(cache_mutex_);

        auto it = cache_.find(filename);
        if (it != cache_.end())
        {
            if (auto shared = it->second.lock())
            {
                // 缓存命中且对象仍然存活
                return shared;
            }
        }
        auto unique_Skeleton = Skeleton::load(filename);
        if (!unique_Skeleton)
        {
            std::cerr << "Error: Failed to load Skeleton " << filename << std::endl;
            return nullptr;
        }
        auto new_Skeleton = std::shared_ptr<Skeleton>(std::move(unique_Skeleton));
        cache_[filename] = new_Skeleton; // 存入 weak_ptr
        return new_Skeleton;
    }

protected:
    SkeletonCache() = default;
    ~SkeletonCache() = default;
    SkeletonCache(const SkeletonCache&) = delete;
    SkeletonCache& operator=(const SkeletonCache&) = delete;

    std::unordered_map<std::string, std::weak_ptr<Skeleton>> cache_;
    std::mutex cache_mutex_;
};


// 缓存获取函数
inline std::shared_ptr<Skeleton> cache_acquire_skeleton(const std::string& filename) {
    if (filename.empty()) return nullptr;
    return SkeletonCache::get_instance().acquire(filename);
}