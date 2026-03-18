/*
定义两个相机类：FPS/欧拉角相机和轨道/目标相机
*/

# pragma once

#include "utils/global.hpp"
#include "math/math.hpp"
#include <eigen3/Eigen/Eigen>
#include <stdexcept>
#include <assert.h>

using namespace Eigen;

struct motion{Vector2f orbit; Vector2f pan; float dolly;};

class Camera_Base{
    protected:
        Vector3f m_position;
        Vector3f m_target;
        float m_aspect;

        inline static const float EPSILON = 1e-6f;
        inline static const float NEAR_PLANE = 0.1f;
        inline static const float FAR_PLANE = 10000.0f;
        inline static const float FIELD_OF_VIEW_Y = to_radians(60);
        inline static const Vector3f UP_VECTOR = {0.0f, 1.0f, 0.0f};

    public:
        Camera_Base(const Vector3f& position, const Vector3f& target, float aspect)
            : m_position(position), m_target(target), m_aspect(aspect)
        {
            if ((position - target).norm() <= EPSILON || aspect <= 0.0f) {
                throw std::invalid_argument("Invalid camera parameters.");
            }
        }


        // Property getters
        Matrix4f get_view_matrix() const ;

        Matrix4f get_projection_matrix() const ;

        Matrix4f get_projection_matrix(float half_w, float half_h, float z_near, float z_far) const;

        Vector3f get_position() const { return m_position; }
};


class Camera {
private:
    Vector3f position_;
    Vector3f front_;
    Vector3f up_;
    Vector3f right_;
    
    float yaw_ = -90.0f;    // 偏航角
    float pitch_ = 0.0f;    // 俯仰角
    
    void update_vectors();
public:
    Camera(const Vector3f& position = {0,0,5}, 
           const Vector3f& target = {0,0,0},
           const Vector3f& up = {0,1,0});
    
    // 获取矩阵
    Matrix4f get_view_matrix() const;
    Matrix4f get_projection_matrix(float fov, float aspect, float near, float far) const;
    
    // 相机控制方法
    void rotate(float delta_yaw, float delta_pitch);  // 偏航和俯仰
    void move_forward(float distance);
    void move_right(float distance);
    void move_up(float distance);
    
    // 获取相机参数
    Vector3f get_position() const { return position_; }
    Vector3f get_front() const { return front_; }
    Vector3f get_up() const { return up_; }
};



class TargetCamera: public Camera_Base {
private:
    
    Vector3f calculate_pan(const Vector3f& from_camera, const motion& motion) const {
        // from_camera = target - position
        Vector3f forward = from_camera.normalized();
        Vector3f left = UP_VECTOR.cross(forward);
        Vector3f up = forward.cross(left);

        float distance = from_camera.norm();
        float factor = distance * (float)std::tan(FIELD_OF_VIEW_Y / 2.0f) * 2.0f;
        
        Vector3f delta_x = left * (motion.pan.x() * factor);
        Vector3f delta_y = up * (motion.pan.y() * factor);
        
        return delta_x + delta_y;
    }

    Vector3f calculate_offset(const Vector3f& from_target, const motion& motion) const {
        // from_target = position - target
        float radius = from_target.norm();
        float theta = (float)std::atan2(from_target.x(), from_target.z()); /* azimuth */
        float phi = (float)std::acos(from_target.y() / radius);           /* polar */
        const float factor = PI * 2.0f;

        // Dolly (Zoom)
        radius *= (float)std::pow(0.95, motion.dolly);

        // Orbit
        theta -= motion.orbit.x() * factor;
        phi -= motion.orbit.y() * factor;
        
        phi = float_clamp(phi, EPSILON, PI - EPSILON); // 钳制 polar 角度

        // Convert spherical coordinates back to Cartesian (Y-up convention)
        float sin_phi = (float)std::sin(phi);
        
        Vector3f offset;
        offset.x() = radius * sin_phi * (float)std::sin(theta);
        offset.y() = radius * (float)std::cos(phi);
        offset.z() = radius * sin_phi * (float)std::cos(theta);

        return offset;
    }

public:
    // Initialization
    TargetCamera(const Vector3f& position, const Vector3f& target, float aspect)
        : Camera_Base(position, target, aspect)
    {
    }
    
    // Camera update
    void camera_set_transform(Vector3f position, Vector3f target) {
        assert((position - target).norm() > EPSILON);
        m_position = position;
        m_target = target;
    }

    void update_transform(const motion& motion) {
        Vector3f from_target = m_position - m_target;
        Vector3f from_camera = m_target - m_position;
        
        Vector3f pan = calculate_pan(from_camera, motion);
        Vector3f offset = calculate_offset(from_target, motion);
        
        m_target += pan;
        m_position = m_target + offset;
    }


};


