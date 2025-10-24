#include "core/graphics.hpp"
#include "core/framebuffer.hpp"
#include "shading/Blinn_shader.hpp" 
#include "shading/skybox_shader.hpp"
namespace
{
    // 裁剪平面枚举
    static enum plane
    {
        POSITIVE_W,
        POSITIVE_X,
        NEGATIVE_X,
        POSITIVE_Y,
        NEGATIVE_Y,
        POSITIVE_Z,
        NEGATIVE_Z
    };
    // 检查顶点是否在平面内侧
    static bool is_inside_plane(const Eigen::Vector4f &coord, plane plane)
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

    // 计算裁剪交点的插值比例
    static float get_intersect_ratio(const Eigen::Vector4f &prev, const Eigen::Vector4f &curr, plane plane)
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

    // 线性插值 Varyings
    template <typename Varyings>
    static Varyings lerp_varyings(const Varyings &a, const Varyings &b, float t)
    {
        return a + (b - a) * t;
    }

    // 针对单个平面进行裁剪 (Sutherland-Hodgman算法的一步)
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
            const Eigen::Vector4f &prev_coord = in_coords[prev_index]; // 上一个顶点的裁剪坐标
            const Eigen::Vector4f &curr_coord = in_coords[i];          // 当前顶点的裁剪坐标
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

} // namespace

template <typename Attribs, typename Varyings, typename Uniforms>
int Program<Attribs, Varyings, Uniforms>::clip_triangle()
{
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

template void graphics_draw_triangle<blinn_attribs, blinn_varyings, blinn_uniforms>(Framebuffer *framebuffer, Program<blinn_attribs, blinn_varyings, blinn_uniforms> *program);
template int Program<blinn_attribs, blinn_varyings, blinn_uniforms>::clip_triangle();

template void graphics_draw_triangle<skybox_attribs, skybox_varyings, skybox_uniforms>(Framebuffer* framebuffer, Program<skybox_attribs, skybox_varyings, skybox_uniforms>* program);
template int Program<skybox_attribs, skybox_varyings, skybox_uniforms>::clip_triangle();