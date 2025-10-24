#include <iostream>
#include "utils/global.hpp"
#include "geometry/camera.hpp"
#include <cmath>


inline double DEG2RAD(double deg) { return deg * PI / 180; }

Matrix4f Camera_Base::get_view_matrix() const
{
    Matrix4f view;
    Vector3f z = (m_position - m_target).normalized();
    Vector3f x = UP_VECTOR.cross(z).normalized();
    Vector3f y = z.cross(x).normalized();
    view << x[0], x[1], x[2], 0,
            y[0], y[1], y[2], 0,
            z[0], z[1], z[2], 0,
            0,    0,    0,    1;

    Matrix4f translate;
    translate << 1, 0, 0, -m_position[0],
                 0, 1, 0, -m_position[1],
                 0, 0, 1, -m_position[2],
                 0, 0, 0, 1;

    return translate * view;
}


Matrix4f Camera_Base::get_projection_matrix() const
{
    Matrix4f projection;
    float t = std::tan(FIELD_OF_VIEW_Y / 2) * NEAR_PLANE;
    float r = t * m_aspect;
    projection << NEAR_PLANE / r, 0, 0, 0,
                  0, NEAR_PLANE / t, 0, 0,
                  0, 0, -(FAR_PLANE + NEAR_PLANE) / (FAR_PLANE - NEAR_PLANE), -2 * FAR_PLANE * NEAR_PLANE / (FAR_PLANE - NEAR_PLANE),
                  0, 0, -1, 0;
    return projection;
}

Matrix4f Camera_Base::get_projection_matrix(float half_w, float half_h, float z_near, float z_far) const {
    float z_range = z_far - z_near;
    Eigen::Matrix4f m = Eigen::Matrix4f::Identity();
    assert(half_w > 0 && half_h > 0 && z_range > 0);
    m(0, 0) = 1 / half_w;
    m(1, 1) = 1 / half_h;
    m(2, 2) = -2 / z_range;
    m(2, 3) = -(z_near + z_far) / z_range;
    return m;
}

Camera::Camera(const Vector3f& position, 
               const Vector3f& target,
               const Vector3f& up)
    : position_(position) 
{
    front_ = (target - position).normalized();
    update_vectors();
}

Matrix4f Camera::get_view_matrix() const {
    Matrix4f view = Matrix4f::Identity();
    
    // 构建视图矩阵 (LookAt)
    view.block<3,1>(0,0) = right_;
    view.block<3,1>(0,1) = up_;
    view.block<3,1>(0,2) = -front_;
    
    // 平移部分
    view(0,3) = -right_.dot(position_);
    view(1,3) = -up_.dot(position_);
    view(2,3) = front_.dot(position_);
    
    return view;
}

Matrix4f Camera::get_projection_matrix(float fov, float aspect, float near, float far) const {
    Matrix4f projection = Matrix4f::Zero();
    
    float tan_half_fov = std::tan(fov * 0.5f * 3.14159f / 180.0f);
    float range = near - far;
    
    projection(0,0) = 1.0f / (aspect * tan_half_fov);
    projection(1,1) = 1.0f / tan_half_fov;
    projection(2,2) = (near + far) / range;
    projection(2,3) = 2.0f * near * far / range;
    projection(3,2) = -1.0f;
    
    return projection;
}

void Camera::rotate(float delta_yaw, float delta_pitch) {
    yaw_ += delta_yaw;
    pitch_ += delta_pitch;
    
    // 限制俯仰角，避免万向锁
    if (pitch_ > 89.0f) pitch_ = 89.0f;
    if (pitch_ < -89.0f) pitch_ = -89.0f;
    
    update_vectors();
}

void Camera::move_forward(float distance) {
    position_ += front_ * distance;
}

void Camera::move_right(float distance) {
    position_ += right_ * distance;
}

void Camera::move_up(float distance) {
    position_ += up_ * distance;
}

void Camera::update_vectors() {
    // 根据偏航角和俯仰角计算前向向量
    Vector3f front;
    front.x() = std::cos(yaw_ * 3.14159f / 180.0f) * std::cos(pitch_ * 3.14159f / 180.0f);
    front.y() = std::sin(pitch_ * 3.14159f / 180.0f);
    front.z() = std::sin(yaw_ * 3.14159f / 180.0f) * std::cos(pitch_ * 3.14159f / 180.0f);
    front_ = front.normalized();
    
    // 重新计算右向量和上向量
    right_ = front_.cross(Vector3f(0,1,0)).normalized();
    up_ = right_.cross(front_).normalized();
}