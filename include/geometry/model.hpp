#pragma once

#include "core/graphics.hpp"
#include "core/framebuffer.hpp"
#include "geometry/mesh.hpp"
#include "geometry/skeleton.hpp"
#include "shading/texture.hpp"
#include "shading/Blinn_shader.hpp"
#include "shading/skybox_shader.hpp"

struct perframe;

class Model_Base
{
public:
    std::shared_ptr<Mesh> mesh;
    std::unique_ptr<IProgram> program;
    Eigen::Matrix4f transform;
    std::shared_ptr<Skeleton> skeleton;
    int attached;   // 骨骼附着点索引，-1表示不附着
    int opaque;     // 不透明标志
    float distance; // 距离摄像机的距离，用于排序

    Model_Base() : mesh(nullptr), skeleton(nullptr), attached(0), opaque(0), distance(0.0f) {}
    virtual ~Model_Base() = default;

    virtual void update(perframe *perframe) = 0;

    virtual void draw(Framebuffer *framebuffer, int shadow_pass) = 0;
};

class Blinn_Phong_Model : public Model_Base
{
public:
    Blinn_Phong_Model(const std::string mesh_path, Eigen::Matrix4f transform,
                      const std::string skeleton_path, int attached,
                      const blinn_material &material) : Model_Base()
    {
        blinn_uniforms uniforms;
        uniforms.basecolor = material.basecolor;
        uniforms.shininess = material.shininess;
        uniforms.diffuse_map = acquire_color_texture(material.diffuse_map);
        uniforms.specular_map = acquire_color_texture(material.specular_map);
        uniforms.emission_map = acquire_color_texture(material.emission_map);
        uniforms.alpha_cutoff = material.alpha_cutoff;
        auto blinn_program = std::make_unique<Program<blinn_attribs, blinn_varyings, blinn_uniforms>>(
            blinn_vertex_shader,
            blinn_fragment_shader,
            std::move(uniforms),
            material.double_sided,       
            material.enable_blend);

        this->program = std::move(blinn_program);

        // 初始化其他成员
        this->mesh = cache_acquire_mesh(mesh_path);
        this->transform = transform;
        this->skeleton = cache_acquire_skeleton(skeleton_path);
        this->attached = attached;
        this->opaque = !material.enable_blend;
        this->distance = 0;
    }

    virtual void update(perframe *perframe);

    virtual void draw(Framebuffer *framebuffer, int shadow_pass);

};


class Skybox_Model : public Model_Base
{
public:
    Skybox_Model(const std::string& skybox_name, int blur_level) {
        skybox_uniforms uniforms;
        // 假设 cache_acquire_skybox 返回 Cubemap*
        uniforms.skybox = acquire_cubemap(skybox_name, blur_level);

        auto skybox_program = std::make_unique<Program<skybox_attribs, skybox_varyings, skybox_uniforms>>(
            skybox_vertex_shader,
            skybox_fragment_shader,
            std::move(uniforms),
            true,  // double_sided
            false  // enable_blend
        );

        this->program = std::move(skybox_program);
        this->mesh = cache_acquire_mesh("common/box.obj");
        this->transform = Eigen::Matrix4f::Identity();
        this->skeleton = nullptr;
        this->attached = -1;
        this->opaque = 1;
        this->distance = 0;
    }

    virtual void update(perframe *perframe) override;
    virtual void draw(Framebuffer *framebuffer, int shadow_pass) override;
};