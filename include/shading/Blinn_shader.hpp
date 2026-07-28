#pragma once

#include <eigen3/Eigen/Eigen>
#include "core/graphics.hpp"
#include "shading/texture.hpp"

// Attribs 结构体，定义了顶点属性
struct blinn_attribs{
    Eigen::Vector3f position;   // 模型空间位置
    Eigen::Vector2f texcoord;   // UV
    Eigen::Vector3f normal;     // 模型空间法线
    Eigen::Vector4f tangent;    // xyz: 切线; w: 副切线方向符号
    Eigen::Vector4f joint;      // 最多4个骨骼索引
    Eigen::Vector4f weight;     // 最多4个骨骼权重
};

// Material 结构体，定义了 Blinn-Phong 着色模型的材质属性
struct blinn_material{
    Eigen::Vector4f basecolor;
    float shininess;
    std::string diffuse_map;
    std::string specular_map;
    std::string emission_map;
    std::string normal_map;
    /* render settings */
    int double_sided;
    int enable_blend;
    float alpha_cutoff;
};

// Varyings 结构体，用于在顶点着色器和片段着色器之间传递数据
struct blinn_varyings {
    Eigen::Vector3f world_position; // 世界空间位置
    Eigen::Vector3f depth_position; // 用于阴影映射的深度位置
    Eigen::Vector2f texcoord;       // 纹理坐标
    Eigen::Vector3f normal;         // 世界空间法线
    Eigen::Vector3f tangent;        // 世界空间切线
    Eigen::Vector3f bitangent;      // 世界空间副切线

    Eigen::Vector2f dUV_dx = {0.0f, 0.0f};
    Eigen::Vector2f dUV_dy = {0.0f, 0.0f};

    blinn_varyings operator-(const blinn_varyings& other) const
    {
        return {
            world_position - other.world_position,
            depth_position - other.depth_position,
            texcoord - other.texcoord,           
            normal - other.normal,
            tangent - other.tangent,
            bitangent - other.bitangent
        };
    }

    blinn_varyings operator+(const blinn_varyings& other) const
    {
        return {
            world_position + other.world_position,
            depth_position + other.depth_position,
            texcoord + other.texcoord,           
            normal + other.normal,
            tangent + other.tangent,
            bitangent + other.bitangent
        };
    }

    blinn_varyings operator*(float scalar) const
    {
        return {
            world_position * scalar, 
            depth_position * scalar, 
            texcoord * scalar,       
            normal * scalar,
            tangent * scalar,
            bitangent * scalar
        };
    }
};

// Uniforms 结构体，包含着色器所需的全局数据
struct blinn_uniforms{
    // 逐帧状态
    Eigen::Vector3f light_dir;
    Eigen::Vector3f camera_pos;
    float ambient_intensity;                // 环境光强度
    float punctual_intensity;               // 点光源强度
    Texture *shadow_map;

    // 变换阵
    Eigen::Matrix4f model_matrix;           // 模型空间 -> 世界空间
    Eigen::Matrix3f normal_matrix;          // 法线变换矩阵
    Eigen::Matrix4f light_vp_matrix;        // projection * view 
    Eigen::Matrix4f camera_vp_matrix;       // projection * view
    
    // 骨骼
    Eigen::Matrix4f *joint_matrices;
    Eigen::Matrix3f *joint_n_matrices;
    
    // 材质，Blinn_Phong_Model创建时即初始化
    Eigen::Vector4f basecolor;              
    float shininess;
    std::shared_ptr<Texture> diffuse_map;
    std::shared_ptr<Texture> specular_map;
    std::shared_ptr<Texture> emission_map;
    std::shared_ptr<Texture> normal_map;
    /* render controls */
    float alpha_cutoff;
    int shadow_pass;
};


Eigen::Vector4f blinn_vertex_shader(const blinn_attribs &attribs, blinn_varyings &varyings, blinn_uniforms &uniforms);
Eigen::Vector4f blinn_fragment_shader(const blinn_varyings &varyings, blinn_uniforms &uniforms, bool *discard, bool backface);