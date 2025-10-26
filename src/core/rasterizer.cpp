#include "core/rasterizer.hpp"
#include "core/framebuffer.hpp"
#include "shading/Blinn_shader.hpp" 
#include "shading/skybox_shader.hpp"

#include <iostream>

namespace
{
    constexpr float EPSILON = 1e-6f;

    // ... (is_back_facing, viewport_transform, BBox, find_bounding_box, etc.)
    // ... (calculate_barycentric_weights, interpolate_varyings, draw_fragment)
    // ... (rasterize_triangle)
    // [NOTE: All rasterization helper functions from graphics.cpp go here]

    /**
     * @brief 背面剔除
     */
    bool is_back_facing(const std::array<Eigen::Vector3f, 3> &ndc_coords)
    {
        const auto &a = ndc_coords[0];
        const auto &b = ndc_coords[1];
        const auto &c = ndc_coords[2];
        float signed_area = a.x() * b.y() - a.y() * b.x() +
                            b.x() * c.y() - b.y() * c.x() +
                            c.x() * a.y() - c.y() * a.x();
        return signed_area <= 0;
    }

    /**
     * @brief 视口变换
     */
    Eigen::Vector3f viewport_transform(int width, int height, const Eigen::Vector3f &ndc_coord)
    {
        float x = (ndc_coord.x() + 1.0f) * 0.5f * static_cast<float>(width);
        float y = (ndc_coord.y() + 1.0f) * 0.5f * static_cast<float>(height);
        float z = (ndc_coord.z() + 1.0f) * 0.5f;
        return {x, y, z};
    }

    struct BBox
    {
        int min_x, min_y, max_x, max_y;
    };

    BBox find_bounding_box(const std::array<Eigen::Vector2f, 3> &screen_coords, int width, int height)
    {
        Eigen::Vector2f min_v = screen_coords[0].cwiseMin(screen_coords[1]).cwiseMin(screen_coords[2]);
        Eigen::Vector2f max_v = screen_coords[0].cwiseMax(screen_coords[1]).cwiseMax(screen_coords[2]);

        return {
            std::max(static_cast<int>(floor(min_v.x())), 0),
            std::max(static_cast<int>(floor(min_v.y())), 0),
            std::min(static_cast<int>(ceil(max_v.x())), width - 1),
            std::min(static_cast<int>(ceil(max_v.y())), height - 1)};
    }

    Eigen::Vector3f calculate_barycentric_weights(const std::array<Eigen::Vector2f, 3> &abc, const Eigen::Vector2f &p)
    {
        Eigen::Vector2f ab = abc[1] - abc[0];
        Eigen::Vector2f ac = abc[2] - abc[0];
        Eigen::Vector2f ap = p - abc[0];

        float factor = 1.0f / (ab.x() * ac.y() - ab.y() * ac.x());
        float s = (ac.y() * ap.x() - ac.x() * ap.y()) * factor;
        float t = (ab.x() * ap.y() - ab.y() * ap.x()) * factor;

        return {1.0f - s - t, s, t};
    }

    template <typename Varyings>
    Varyings interpolate_varyings(
        const std::array<Varyings, 3> &src_varyings,
        const Eigen::Vector3f &weights,
        const std::array<float, 3> &recip_w)
    {
        // 透视校正插值公式:
        // V_p = ( (V_a/w_a)*b_a + (V_b/w_b)*b_b + (V_c/w_c)*b_c ) / 
        //       ( (1/w_a)*b_a + (1/w_b)*b_b + (1/w_c)*b_c )
        // 其中 V 是 varying, w 是 w分量, b 是重心坐标

        // 计算分子部分
        Varyings numerator = src_varyings[0] * (recip_w[0] * weights.x()) +
                             src_varyings[1] * (recip_w[1] * weights.y()) +
                             src_varyings[2] * (recip_w[2] * weights.z());

        // 计算分母部分
        float denominator = recip_w[0] * weights.x() +
                            recip_w[1] * weights.y() +
                            recip_w[2] * weights.z();

        // 返回最终结果
        return numerator * (1.0f / denominator);
    }

    template <typename Attribs, typename Varyings, typename Uniforms>
    void draw_fragment(Framebuffer *framebuffer, Program<Attribs, Varyings, Uniforms> *program,
                       int index, float depth, bool is_backface)
    {
        bool discard = false;
        Eigen::Vector4f color = program->fragment_shader_ptr(program->shader_varyings, *program->shader_uniforms, &discard, is_backface);

        if (discard) return;

        color = color.cwiseMax(0.0f).cwiseMin(1.0f);

        if (program->is_blend_enabled)
        {
            Eigen::Vector4f dst_color = framebuffer->get_color(index);
            float src_alpha = color.w();

            // 标准 Alpha 混合
            color.head<3>() = color.head<3>() * src_alpha + dst_color.head<3>() * (1.0f - src_alpha);
            color.w() = src_alpha + dst_color.w() * (1.0f - src_alpha);
        }

        framebuffer->set_color(index, color);
        framebuffer->set_depth(index, depth);
    }

    template <typename Attribs, typename Varyings, typename Uniforms>
    bool rasterize_triangle(Framebuffer *framebuffer, Program<Attribs, Varyings, Uniforms> *program,
                            const std::array<Eigen::Vector4f, 3>& clip_coords,
                            const std::array<Varyings, 3>& varyings)
    {
        std::array<Eigen::Vector3f, 3> ndc_coords;
        for (int i = 0; i < 3; i++)
        {
            ndc_coords[i] = clip_coords[i].head<3>() / clip_coords[i].w();
        }

        //test code
        //std::cout << "rasterize_triangle() " << std::endl;
        
        /*for (int i = 0; i < 3; ++i) {
            std::cout << "ndc: " << ndc_coords[i].transpose() << std::endl;
        }*/
        
        bool backface = is_back_facing(ndc_coords);
        //bool backface = false;
        if (backface && !program->is_double_sided)
        {
            return true;
        }

        std::array<float, 3> recip_w = {1.0f / clip_coords[0].w(), 1.0f / clip_coords[1].w(), 1.0f / clip_coords[2].w()};

        std::array<Eigen::Vector2f, 3> screen_coords;
        std::array<float, 3> screen_depths;
        for (int i = 0; i < 3; i++)
        {
            Eigen::Vector3f window_coord = viewport_transform(framebuffer->get_width(), framebuffer->get_height(), ndc_coords[i]);
            screen_coords[i] = window_coord.head<2>();
            screen_depths[i] = window_coord.z();
        }

        BBox bbox = find_bounding_box(screen_coords, framebuffer->get_width(), framebuffer->get_height());
        for (int x = bbox.min_x; x <= bbox.max_x; x++)
        {
            for (int y = bbox.min_y; y <= bbox.max_y; y++)
            {
                Eigen::Vector2f p = {static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f};
                Eigen::Vector3f weights = calculate_barycentric_weights(screen_coords, p);

                if (weights.x() > -EPSILON && weights.y() > -EPSILON && weights.z() > -EPSILON)
                {
                    int index = y * framebuffer->get_width() + x;
                    //// 1. 计算插值后的 1/w (这和 interpolate_varyings 中的分母一样)
                    //float interp_recip_w = recip_w[0] * weights.x() +
                    //    recip_w[1] * weights.y() +
                    //    recip_w[2] * weights.z();

                    //// 2. 计算插值后的 z_ndc/w
                    ////    (ndc_coords[i].z() 是 z_ndc)
                    //float interp_z_ndc_over_w = (ndc_coords[0].z() * recip_w[0] * weights.x()) +
                    //    (ndc_coords[1].z() * recip_w[1] * weights.y()) +
                    //    (ndc_coords[2].z() * recip_w[2] * weights.z());

                    //// 3. 获得正确的透视校正后的 z_ndc
                    //float interp_z_ndc = interp_z_ndc_over_w / interp_recip_w;

                    //// 4. 将 z_ndc [-1, 1] 转换回你的深度缓冲范围 [0, 1]
                    //float depth = (interp_z_ndc + 1.0f) * 0.5f;
                    float depth = weights.dot(Eigen::Map<const Eigen::Vector3f>(screen_depths.data()));
                    if (depth <= framebuffer->get_depth(index)) // 深度检查
                    {
                        program->shader_varyings = interpolate_varyings(varyings, weights, recip_w);
                        draw_fragment(framebuffer, program, index, depth, backface);
                    }
                }
            }
        }
        return false;
    }

} // namespace

template <typename Attribs, typename Varyings, typename Uniforms>
void rasterize_polygon(Framebuffer *framebuffer, Program<Attribs, Varyings, Uniforms> *program, int num_vertices)
{
    for (int i = 0; i < num_vertices - 2; ++i)
    {
        std::array<Eigen::Vector4f, 3> current_clip_coords = {
            program->out_coords[0],
            program->out_coords[i + 1],
            program->out_coords[i + 2]};

        std::array<Varyings, 3> current_varyings = {
            program->out_varyings[0],
            program->out_varyings[i + 1],
            program->out_varyings[i + 2]};

        if (rasterize_triangle(framebuffer, program, current_clip_coords, current_varyings))
        {
            break;
        }
    }
}

// Explicit instantiation
template void rasterize_polygon<blinn_attribs, blinn_varyings, blinn_uniforms>(Framebuffer *framebuffer, Program<blinn_attribs, blinn_varyings, blinn_uniforms> *program, int num_vertices);
template void rasterize_polygon<skybox_attribs, skybox_varyings, skybox_uniforms>(Framebuffer* framebuffer, Program<skybox_attribs, skybox_varyings, skybox_uniforms>* program, int num_vertices);