#pragma once

#include <vector>
#include <memory>
#include <array>
#include <Eigen/Core>
#include "core/framebuffer.hpp"
#include "core/rasterizer.hpp"
#include "shading/shader.hpp"
#include "utils/global.hpp"
#include <iostream>
#include <assert.h>


class IProgram
{
public:
    virtual ~IProgram() = default;
};

// Helper functions for clipping
namespace graphics_detail {

    // Clipping plane enumeration
    enum plane
    {
        POSITIVE_W,
        POSITIVE_X,
        NEGATIVE_X,
        POSITIVE_Y,
        NEGATIVE_Y,
        POSITIVE_Z,
        NEGATIVE_Z
    };

    // Check if a vertex is inside a clipping plane
    inline bool is_inside_plane(const Eigen::Vector4f &coord, plane plane)
    {
        switch (plane)
        {
        case POSITIVE_W: return coord.w() >= EPSILON;
        case POSITIVE_X: return coord.x() <= +coord.w();
        case NEGATIVE_X: return coord.x() >= -coord.w();
        case POSITIVE_Y: return coord.y() <= +coord.w();
        case NEGATIVE_Y: return coord.y() >= -coord.w();
        case POSITIVE_Z: return coord.z() <= +coord.w();
        case NEGATIVE_Z: return coord.z() >= -coord.w();
        default:
            assert(0);
            return false;
        }
    }

    // Compute the interpolation ratio for a clipping intersection
    inline float get_intersect_ratio(const Eigen::Vector4f &prev, const Eigen::Vector4f &curr, plane plane)
    {
        switch (plane)
        {
        case POSITIVE_W: return (prev.w() - EPSILON) / (prev.w() - curr.w());
        case POSITIVE_X: return (prev.w() - prev.x()) / ((prev.w() - prev.x()) - (curr.w() - curr.x()));
        case NEGATIVE_X: return (prev.w() + prev.x()) / ((prev.w() + prev.x()) - (curr.w() + curr.x()));
        case POSITIVE_Y: return (prev.w() - prev.y()) / ((prev.w() - prev.y()) - (curr.w() - curr.y()));
        case NEGATIVE_Y: return (prev.w() + prev.y()) / ((prev.w() + prev.y()) - (curr.w() + curr.y()));
        case POSITIVE_Z: return (prev.w() - prev.z()) / ((prev.w() - prev.z()) - (curr.w() - curr.z()));
        case NEGATIVE_Z: return (prev.w() + prev.z()) / ((prev.w() + prev.z()) - (curr.w() + curr.z()));
        default:
            assert(0);
            return 0;
        }
    }

    // Linearly interpolate Varyings
    template <typename Varyings>
    inline Varyings lerp_varyings(const Varyings &a, const Varyings &b, float t)
    {
        return a + (b - a) * t;
    }

    // Clip against a single plane (one step of Sutherland-Hodgman algorithm)
    template <typename Varyings>
    inline int clip_against_plane(
        plane plane, int in_num_vertices,
        const std::array<Eigen::Vector4f, 10> &in_coords, const std::array<Varyings, 10> &in_varyings,
        std::array<Eigen::Vector4f, 10> &out_coords, std::array<Varyings, 10> &out_varyings)
    {
        int out_num_vertices = 0;
        for (int i = 0; i < in_num_vertices; i++)
        {
            int prev_index = (i - 1 + in_num_vertices) % in_num_vertices;
            const Eigen::Vector4f &prev_coord = in_coords[prev_index];
            const Eigen::Vector4f &curr_coord = in_coords[i];
            const Varyings &prev_varyings = in_varyings[prev_index];
            const Varyings &curr_varyings = in_varyings[i];

            bool prev_inside = is_inside_plane(prev_coord, plane);
            bool curr_inside = is_inside_plane(curr_coord, plane);

            if (prev_inside != curr_inside)
            {
                float ratio = get_intersect_ratio(prev_coord, curr_coord, plane);
                out_coords[out_num_vertices] = prev_coord + (curr_coord - prev_coord) * ratio;
                out_varyings[out_num_vertices] = lerp_varyings(prev_varyings, curr_varyings, ratio);
                out_num_vertices++;
            }

            if (curr_inside)
            {
                out_coords[out_num_vertices] = curr_coord;
                out_varyings[out_num_vertices] = curr_varyings;
                out_num_vertices++;
            }
        }
        return out_num_vertices;
    }

} // namespace graphics_detail

template <typename Attribs, typename Varyings, typename Uniforms>
class Program : public IProgram
{
public:
    using vertex_shader = vertex_shader_t<Attribs, Varyings, Uniforms>;
    using fragment_shader = fragment_shader_t<Varyings, Uniforms>;

    Program(vertex_shader vs, fragment_shader fs, Uniforms initial_uniforms, bool double_sided, bool enable_blend)
        : vertex_shader_ptr(vs),
          fragment_shader_ptr(fs),
          shader_uniforms(std::make_unique<Uniforms>(std::move(initial_uniforms))),
          is_double_sided(double_sided),
          is_blend_enabled(enable_blend)
    {
    }

    ~Program() = default;

    Program(const Program &) = delete;
    Program &operator=(const Program &) = delete;

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

    // 内部数据，设为 public 以便渲染器访问
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
    int clip_triangle(){
        using namespace graphics_detail;
        // 简单可见性测试，若全部顶点都在视锥内则直接返回
        bool v0_visible = is_inside_plane(in_coords[0], POSITIVE_X) && is_inside_plane(in_coords[0], NEGATIVE_X) && is_inside_plane(in_coords[0], POSITIVE_Y) && is_inside_plane(in_coords[0], NEGATIVE_Y) && is_inside_plane(in_coords[0], POSITIVE_Z) && is_inside_plane(in_coords[0], NEGATIVE_Z);
        bool v1_visible = is_inside_plane(in_coords[1], POSITIVE_X) && is_inside_plane(in_coords[1], NEGATIVE_X) && is_inside_plane(in_coords[1], POSITIVE_Y) && is_inside_plane(in_coords[1], NEGATIVE_Y) && is_inside_plane(in_coords[1], POSITIVE_Z) && is_inside_plane(in_coords[1], NEGATIVE_Z);
        bool v2_visible = is_inside_plane(in_coords[2], POSITIVE_X) && is_inside_plane(in_coords[2], NEGATIVE_X) && is_inside_plane(in_coords[2], POSITIVE_Y) && is_inside_plane(in_coords[2], NEGATIVE_Y) && is_inside_plane(in_coords[2], POSITIVE_Z) && is_inside_plane(in_coords[2], NEGATIVE_Z);

        if (v0_visible && v1_visible && v2_visible)
        {
            out_coords[0] = in_coords[0];
            out_coords[1] = in_coords[1];
            out_coords[2] = in_coords[2];
            out_varyings[0] = in_varyings[0];
            out_varyings[1] = in_varyings[1];
            out_varyings[2] = in_varyings[2];
            return 3;
        }

        // 使用乒乓缓冲进行7平面裁剪
        int num_vertices = 3;

        num_vertices = clip_against_plane<Varyings>(POSITIVE_W, num_vertices, in_coords, in_varyings, out_coords, out_varyings);
        if (num_vertices < 3) return 0;
        num_vertices = clip_against_plane<Varyings>(POSITIVE_X, num_vertices, out_coords, out_varyings, in_coords, in_varyings);
        if (num_vertices < 3) return 0;
        num_vertices = clip_against_plane<Varyings>(NEGATIVE_X, num_vertices, in_coords, in_varyings, out_coords, out_varyings);
        if (num_vertices < 3) return 0;
        num_vertices = clip_against_plane<Varyings>(POSITIVE_Y, num_vertices, out_coords, out_varyings, in_coords, in_varyings);
        if (num_vertices < 3) return 0;
        num_vertices = clip_against_plane<Varyings>(NEGATIVE_Y, num_vertices, in_coords, in_varyings, out_coords, out_varyings);
        if (num_vertices < 3) return 0;
        num_vertices = clip_against_plane<Varyings>(POSITIVE_Z, num_vertices, out_coords, out_varyings, in_coords, in_varyings);
        if (num_vertices < 3) return 0;
        num_vertices = clip_against_plane<Varyings>(NEGATIVE_Z, num_vertices, in_coords, in_varyings, out_coords, out_varyings);

        return num_vertices;
    }
    int no_clip(){
        out_coords[0] = in_coords[0];
        out_coords[1] = in_coords[1];
        out_coords[2] = in_coords[2];
        out_varyings[0] = in_varyings[0];
        out_varyings[1] = in_varyings[1];
        out_varyings[2] = in_varyings[2];
        return 3;
    }
};

// 绘制三角形的核心函数
template <typename Attribs, typename Varyings, typename Uniforms>
void graphics_draw_triangle(Framebuffer *framebuffer, Program<Attribs, Varyings, Uniforms> *program)
{
	//std::cout << "graphics draw triangle()\n";
    // 1. 顶点着色器
    for (int i = 0; i < 3; ++i)
    {
        program->in_coords[i] = program->vertex_shader_ptr(program->shader_attribs[i],
                                                           program->in_varyings[i],
                                                           *program->shader_uniforms);
		//std::cout << "clip cord: " << program->in_coords[i].transpose() << std::endl;
    }

    
	//std::cout << "after vertex shader\n";
    // 2. 裁剪
    int num_vertices = program->clip_triangle();
    
	/*for (int i = 0; i < num_vertices; ++i) {
		std::cout << "clipped cord " << i << ":\n" << program->out_coords[i] << std::endl;
		std::cout << "clipped varyings " << i << ":\n" << 
            "world position:\n " << program->out_varyings[i].world_position << 
            " normal:\n" << program->out_varyings[i].normal << std::endl;
	}*/
    //test
    //int num_vertices = program->no_clip();
    //test end

    // 3. 三角形组装与光栅化
    if (num_vertices >= 3)
    {
        rasterize_polygon(framebuffer, program, num_vertices);
    }
}