# pragma once
#include <Eigen/Dense>

float float_clamp(float f, float min, float max);

Eigen::Matrix4f mat4_lookat(const Eigen::Vector3f& eye,
    const Eigen::Vector3f& target,
    const Eigen::Vector3f& up);

Eigen::Matrix4f mat4_orthographic(float right, float top, float near, float far);