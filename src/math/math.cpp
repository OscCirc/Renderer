#include "math/math.hpp"
#include <cassert>

float float_clamp(float f, float min, float max) {
    return f < min ? min : (f > max ? max : f);
}

Eigen::Matrix4f mat4_lookat(const Eigen::Vector3f& eye,
    const Eigen::Vector3f& target,
    const Eigen::Vector3f& up)
{
    // 计算视线方向 (z轴)
    Eigen::Vector3f z_axis = (eye - target).normalized();

    // 计算右方向 (x轴)
    Eigen::Vector3f x_axis = up.cross(z_axis).normalized();

    // 计算上方向 (y轴)
    Eigen::Vector3f y_axis = z_axis.cross(x_axis);

    // 创建单位矩阵
    Eigen::Matrix4f m = Eigen::Matrix4f::Identity();

    // 严格按照原始代码逻辑填充矩阵
    // 第一行
    m(0, 0) = x_axis.x();
    m(0, 1) = x_axis.y();
    m(0, 2) = x_axis.z();

    // 第二行
    m(1, 0) = y_axis.x();
    m(1, 1) = y_axis.y();
    m(1, 2) = y_axis.z();

    // 第三行
    m(2, 0) = z_axis.x();
    m(2, 1) = z_axis.y();
    m(2, 2) = z_axis.z();

    // 平移部分
    m(0, 3) = -x_axis.dot(eye);
    m(1, 3) = -y_axis.dot(eye);
    m(2, 3) = -z_axis.dot(eye);

    return m;
}



Eigen::Matrix4f mat4_orthographic(float right, float top, float near, float far) {
    // 确保参数有效性
    assert(right > 0 && top > 0 && far > near);

    // 计算z轴范围
    float z_range = far - near;

    // 创建单位矩阵
    Eigen::Matrix4f m = Eigen::Matrix4f::Identity();

    // 设置正交投影参数
    m(0, 0) = 1.0f / right;   // x轴缩放
    m(1, 1) = 1.0f / top;     // y轴缩放
    m(2, 2) = -2.0f / z_range; // z轴缩放
    m(2, 3) = -(near + far) / z_range; // z轴平移

    return m;
}