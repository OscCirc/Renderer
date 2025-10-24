#pragma once

#include <vector>
#include <memory>
#include <array>
#include <Eigen/Core>
#include "core/framebuffer.hpp"
#include "core/rasterizer.hpp"
#include "shading/shader.hpp"



class IProgram
{
public:
    virtual ~IProgram() = default;
};

template <typename Attribs, typename Varyings, typename Uniforms>
class Program : public IProgram
{
public:
    // 使用 using 别名简化类型
    using vertex_shader = vertex_shader_t<Attribs, Varyings, Uniforms>;
    using fragment_shader = fragment_shader_t<Varyings, Uniforms>;

    // 构造函数
    Program(vertex_shader vs, fragment_shader fs, Uniforms initial_uniforms, bool double_sided, bool enable_blend)
        : vertex_shader_ptr(vs),
          fragment_shader_ptr(fs),
          shader_uniforms(std::make_unique<Uniforms>(std::move(initial_uniforms))),
          is_double_sided(double_sided),
          is_blend_enabled(enable_blend)
    {
    }

    // 析构函数会自动处理内存释放
    ~Program() = default;

    // 删除拷贝构造和赋值，因为我们手动管理了内存
    Program(const Program &) = delete;
    Program &operator=(const Program &) = delete;

    // 公共接口
    Uniforms &get_uniforms()
    {
        return *shader_uniforms;
    }
    const Uniforms &get_uniforms() const
    {
        return *shader_uniforms;
    }

    // 成员变量
    // 着色器函数指针
    vertex_shader vertex_shader_ptr;
    fragment_shader fragment_shader_ptr;

    // 渲染状态
    bool is_double_sided;
    bool is_blend_enabled;

    // 内部数据，设为 public 以便渲染器访问，也可以设为 friend
    std::array<Attribs, 3> shader_attribs;
    Varyings shader_varyings;
    std::unique_ptr<Uniforms> shader_uniforms;

    // 裁剪用缓冲区
    static constexpr int MAX_CLIPPED_VERTICES = 10;
    std::array<Eigen::Vector4f, MAX_CLIPPED_VERTICES> in_coords; // 输入裁剪坐标
    std::array<Eigen::Vector4f, MAX_CLIPPED_VERTICES> out_coords;
    std::array<Varyings, MAX_CLIPPED_VERTICES> in_varyings;
    std::array<Varyings, MAX_CLIPPED_VERTICES> out_varyings;

    // 裁剪函数
    int clip_triangle();
};

// 绘制三角形的核心函数
template <typename Attribs, typename Varyings, typename Uniforms>
void graphics_draw_triangle(Framebuffer *framebuffer, Program<Attribs, Varyings, Uniforms> *program)
{
    // 1. 顶点着色器
    for (int i = 0; i < 3; ++i)
    {
        program->in_coords[i] = program->vertex_shader_ptr(program->shader_attribs[i],
                                                           program->in_varyings[i],
                                                           *program->shader_uniforms);
    }

    // 2. 裁剪
    int num_vertices = program->clip_triangle();

    // 3. 三角形组装与光栅化
    if (num_vertices >= 3)
    {
        rasterize_polygon(framebuffer, program, num_vertices);
    }
}

// Program的成员函数：裁剪一个三角形
template <typename Attribs, typename Varyings, typename Uniforms>
int Program<Attribs, Varyings, Uniforms>::clip_triangle();