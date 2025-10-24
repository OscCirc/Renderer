#pragma once

#include <Eigen/Core>
#include "utils/global.hpp"
#include "shading/texture.hpp" 

class Cubemap;

struct skybox_attribs
{
    Eigen::Vector3f position;
};

struct skybox_varyings
{
    Eigen::Vector3f direction;

    skybox_varyings operator-(const skybox_varyings& other) const
    {
        return {
            direction - other.direction };
    }

    skybox_varyings operator+(const skybox_varyings& other) const
    {
        return {
            direction + other.direction };
    }

    skybox_varyings operator*(float scalar) const
    {
        return {
            direction * scalar };
    }
};

struct skybox_uniforms
{
    Eigen::Matrix4f vp_matrix;
    std::shared_ptr<Cubemap> skybox;
};

Eigen::Vector4f skybox_vertex_shader(const skybox_attribs &attribs, skybox_varyings &varyings, skybox_uniforms &uniforms);
Eigen::Vector4f skybox_fragment_shader(const skybox_varyings &varyings, skybox_uniforms &uniforms, bool *discard, bool backface);