#include "geometry/skeleton.hpp"
#include <fstream>
#include <stdexcept>
#include <iostream>

// --- Joint 方法实现 ---

// 查找关键帧并插值
template<typename T>
T find_and_interpolate(float time, 
                      const std::vector<Keyframe<T>>& keyframes,
                      T default_value) 
{
    // 1. 处理空关键帧序列
    if (keyframes.empty()) 
        return default_value;
    
    // 2. 处理时间在第一个关键帧之前的情况
    if (time <= keyframes.front().time) 
        return keyframes.front().value;
    
    // 3. 处理时间在最后一个关键帧之后的情况
    if (time >= keyframes.back().time) 
        return keyframes.back().value;
    
    // 4. 使用二分查找找到第一个大于等于目标时间的关键帧
    auto it = std::upper_bound(
        keyframes.begin(), keyframes.end(), time,
        [](float t, const Keyframe<T>& kf) {
            return t < kf.time;
        }
    );
    
    // 5. 检查找到的位置
    if (it == keyframes.begin() || it == keyframes.end()) {
        // 理论上不会发生（边界已处理）
        return keyframes.back().value;
    }
    
    // 6. 获取插值区间
    auto prev = it - 1; // 区间起始关键帧
    auto next = it;     // 区间结束关键帧
    
    // 7. 浮点精度容差处理
    const float epsilon = 1e-5f;
    if (std::abs(time - prev->time) < epsilon) {
        return prev->value;
    }
    if (std::abs(time - next->time) < epsilon) {
        return next->value;
    }
    
    // 8. 计算插值比例 t ∈ [0, 1]
    float t = (time - prev->time) / (next->time - prev->time);
    
    // 9. 类型特化的插值计算
    if constexpr (std::is_same_v<T, Eigen::Quaternionf>) {
        // 四元数球面线性插值
        return prev->value.slerp(t, next->value);
    } else {
        // 普通线性插值: result = start + t*(end - start)
        return prev->value + t * (next->value - prev->value);
    }
}

Eigen::Matrix4f Joint::get_local_transform(float frame_time) const
{
    Eigen::Vector3f translation = find_and_interpolate<Eigen::Vector3f>(frame_time, translations, Eigen::Vector3f::Zero());
    Eigen::Quaternionf rotation = find_and_interpolate<Eigen::Quaternionf>(frame_time, rotations, Eigen::Quaternionf::Identity());
    Eigen::Vector3f scale = find_and_interpolate<Eigen::Vector3f>(frame_time, scales, Eigen::Vector3f::Ones());

    Eigen::Affine3f t(Eigen::Translation3f(translation) * rotation * Eigen::Scaling(scale));
    return t.matrix();
}

// --- Skeleton 方法实现 ---

Skeleton::Skeleton(float min_time, float max_time, std::vector<Joint> &&joints)
    : min_time_(min_time), max_time_(max_time), joints_(std::move(joints))
{
    size_t num_joints = joints_.size();
    joint_matrices_.resize(num_joints);
    normal_matrices_.resize(num_joints);
}

std::unique_ptr<Skeleton> Skeleton::load(const std::string &filename)
{
    std::string ext = filename.substr(filename.find_last_of('.') + 1);
    if (ext == "ani")
    {
        return load_ani(filename);
    }
    std::cerr << "Unsupported skeleton format: " << ext << std::endl;
    return nullptr;
}

void Skeleton::update(float frame_time)
{
    frame_time = fmod(frame_time, max_time_);
    if (frame_time == last_time_)
    {
        return; // 缓存命中，无需更新
    }

    for (size_t i = 0; i < joints_.size(); ++i)
    {
        Joint &joint = joints_[i];
        Eigen::Matrix4f local_transform = joint.get_local_transform(frame_time);

        if (joint.parent_index >= 0)
        {
            const Joint &parent = joints_[joint.parent_index];
            joint.global_transform = parent.global_transform * local_transform;
        }
        else
        {
            joint.global_transform = local_transform;
        }

        joint_matrices_[i] = joint.global_transform * joint.inverse_bind_matrix;
        normal_matrices_[i] = joint_matrices_[i].block<3, 3>(0, 0).inverse().transpose();
    }
    last_time_ = frame_time;
}

std::vector<Eigen::Matrix4f> &Skeleton::get_joint_matrices()
{
    return joint_matrices_;
}

std::vector<Eigen::Matrix3f> &Skeleton::get_normal_matrices()
{
    return normal_matrices_;
}

// --- 私有加载逻辑 ---

std::unique_ptr<Skeleton> Skeleton::load_ani(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open skeleton file: " + filename);
    }

    std::string token;
    char delimiter;
    int num_joints;
    float min_time, max_time;

    file >> token >> num_joints;                                                  // "joint-size:"
    file >> token >> delimiter >> min_time >> delimiter >> max_time >> delimiter; // "time-range: [t, t]"

    std::vector<Joint> joints(num_joints);
    for (int i = 0; i < num_joints; ++i)
    {
        int joint_index;
        file >> token >> joint_index >> delimiter; // "joint"
        file >> token >> joints[i].parent_index;   // "parent-index:"
        file >> token;                             // "inverse-bind:"
        for (int row = 0; row < 4; ++row)
            for (int col = 0; col < 4; ++col)
                file >> joints[i].inverse_bind_matrix(row, col);

        auto read_keyframes = [&](auto &keyframes_vec, int num_keys)
        {
            keyframes_vec.resize(num_keys);
            for (int k = 0; k < num_keys; ++k)
            {
                file >> token >> keyframes_vec[k].time >> delimiter; // "time:"
                file >> token >> delimiter;                          // "value:"
                if constexpr (std::is_same_v<decltype(keyframes_vec[k].value), Eigen::Vector3f>)
                {
                    file >> keyframes_vec[k].value.x() >> delimiter >> keyframes_vec[k].value.y() >> delimiter >> keyframes_vec[k].value.z();
                }
                else if constexpr (std::is_same_v<decltype(keyframes_vec[k].value), Eigen::Quaternionf>)
                {
                    file >> keyframes_vec[k].value.x() >> delimiter >> keyframes_vec[k].value.y() >> delimiter >> keyframes_vec[k].value.z() >> delimiter >> keyframes_vec[k].value.w();
                }
                file >> delimiter; // ']'
            }
        };

        int num_keys;
        file >> token >> num_keys >> delimiter; // "translations"
        read_keyframes(joints[i].translations, num_keys);
        file >> token >> num_keys >> delimiter; // "rotations"
        read_keyframes(joints[i].rotations, num_keys);
        file >> token >> num_keys >> delimiter; // "scales"
        read_keyframes(joints[i].scales, num_keys);
    }

    return std::unique_ptr<Skeleton>(new Skeleton(min_time, max_time, std::move(joints)));
}