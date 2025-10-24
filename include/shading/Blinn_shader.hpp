#pragma once

#include <eigen3/Eigen/Eigen>
#include "core/graphics.hpp"
#include "shading/texture.hpp"

// Attribs 结构体，定义了顶点属性
struct blinn_attribs{
    Eigen::Vector3f position;
    Eigen::Vector2f texcoord;
    Eigen::Vector3f normal;
    Eigen::Vector4f joint;
    Eigen::Vector4f weight;
};

// Material 结构体，定义了 Blinn-Phong 着色模型的材质属性
struct blinn_material{
    Eigen::Vector4f basecolor;
    float shininess;
    std::string diffuse_map;
    std::string specular_map;
    std::string emission_map;
    /* render settings */
    int double_sided;
    int enable_blend;
    float alpha_cutoff;
};

// Varyings 结构体，用于在顶点着色器和片段着色器之间传递数据
struct blinn_varyings{
    Eigen::Vector3f world_position; // 世界空间位置
    Eigen::Vector3f depth_position; // 用于阴影映射的深度位置
    Eigen::Vector2f texcoord;       // 纹理坐标
    Eigen::Vector3f normal;         // 世界空间法线

    blinn_varyings operator-(const blinn_varyings &other) const
    {
        return {
            world_position - other.world_position,
            normal - other.normal,
            texcoord - other.texcoord};
    }

    blinn_varyings operator+(const blinn_varyings &other) const
    {
        return {
            world_position + other.world_position,
            normal + other.normal,
            texcoord + other.texcoord};
    }

    blinn_varyings operator*(float scalar) const
    {
        return {
            world_position * scalar,
            normal * scalar,
            texcoord * scalar};
    }
};

// Uniforms 结构体，包含着色器所需的全局数据
struct blinn_uniforms{
    Eigen::Vector3f light_dir;
    Eigen::Vector3f camera_pos;
    Eigen::Matrix4f model_matrix;
    Eigen::Matrix3f normal_matrix;
    Eigen::Matrix4f light_vp_matrix;
    Eigen::Matrix4f camera_vp_matrix;
    Eigen::Matrix4f *joint_matrices;
    Eigen::Matrix3f *joint_n_matrices;
    float ambient_intensity;             // 环境光强度
    float punctual_intensity;            // 点光源强度
    Texture *shadow_map;
    /* surface parameters */
    Eigen::Vector4f basecolor;
    float shininess;
    std::shared_ptr<Texture> diffuse_map;
    std::shared_ptr<Texture> specular_map;
    std::shared_ptr<Texture> emission_map;
    /* render controls */
    float alpha_cutoff;
    int shadow_pass;
};


Eigen::Vector4f blinn_vertex_shader(const blinn_attribs &attribs, blinn_varyings &varyings, blinn_uniforms &uniforms);
Eigen::Vector4f blinn_fragment_shader(const blinn_varyings &varyings, blinn_uniforms &uniforms, bool *discard, bool backface);