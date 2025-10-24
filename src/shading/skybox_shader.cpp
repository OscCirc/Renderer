#include "shading/skybox_shader.hpp"
#include "shading/texture.hpp" 



Eigen::Vector4f skybox_vertex_shader(const skybox_attribs &attribs, skybox_varyings &varyings, skybox_uniforms &uniforms)
{
    Eigen::Vector4f local_pos(attribs.position.x(), attribs.position.y(), attribs.position.z(), 1.0f);
    Eigen::Vector4f clip_pos = uniforms.vp_matrix * local_pos;

    // 将深度设置为最大值，确保天空盒在最远处
    clip_pos.z() = clip_pos.w() * (1.0f - EPSILON);

    varyings.direction = attribs.position;
    return clip_pos;
}

Eigen::Vector4f skybox_fragment_shader(const skybox_varyings &varyings, skybox_uniforms &uniforms, bool *discard, bool backface)
{
    // UNUSED_VAR 宏的功能可以通过 (void)variable 实现
    (void)discard;
    (void)backface;

    if (uniforms.skybox)
    {
        return uniforms.skybox->sample(varyings.direction);
    }
    return Eigen::Vector4f(0, 0, 0, 1); // 如果没有天空盒，返回黑色
}