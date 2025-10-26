#include "geometry/model.hpp"
#include "core/scene.hpp"


void Blinn_Phong_Model::update(perframe *perframe)
{
    if (!mesh)
        return;

    // 类型转换，以便访问具体的 uniforms
    auto blinn_program = static_cast<Program<blinn_attribs, blinn_varyings, blinn_uniforms> *>(this->program.get());
    auto &uniforms = blinn_program->get_uniforms();

    Eigen::Matrix4f model_matrix = this->transform;

    // 更新骨骼动画
    if (skeleton)
    {
        skeleton->update(perframe->frame_time);
        auto &joint_matrices = skeleton->get_joint_matrices();
        auto &joint_n_matrices = skeleton->get_normal_matrices();

        if (this->attached >= 0 && this->attached < joint_matrices.size())
        {
            // 如果模型附加到某个骨骼上
            model_matrix = model_matrix * joint_matrices[this->attached];
            uniforms.joint_matrices = nullptr; // 不再需要蒙皮
            uniforms.joint_n_matrices = nullptr;
        }
        else
        {
            // 否则，进行蒙皮
            uniforms.joint_matrices = joint_matrices.data();
            uniforms.joint_n_matrices = joint_n_matrices.data();
        }
    }
    else
    {
        uniforms.joint_matrices = nullptr;
        uniforms.joint_n_matrices = nullptr;
    }

    // 更新 uniforms
    uniforms.model_matrix = model_matrix;
    uniforms.normal_matrix = model_matrix.block<3, 3>(0, 0).inverse().transpose();
    uniforms.light_dir = perframe->light_dir;
    uniforms.camera_pos = perframe->camera_pos;
    uniforms.light_vp_matrix = perframe->light_proj_matrix * perframe->light_view_matrix;
    uniforms.camera_vp_matrix = perframe->camera_proj_matrix * perframe->camera_view_matrix;
    uniforms.ambient_intensity = std::clamp(perframe->ambient_intensity, 0.0f, 5.0f);
    uniforms.punctual_intensity = std::clamp(perframe->punctual_intensity, 0.0f, 5.0f);
    uniforms.shadow_map = perframe->shadow_map;
}

void Blinn_Phong_Model::draw(Framebuffer *framebuffer, int shadow_pass) 
{
    //std::cout << "Blinn_Phong::draw()" << std::endl;
    if (!mesh || !program)
        return;

    // 类型转换，以便访问具体的 uniforms 和 shader_attribs
    auto blinn_program = static_cast<Program<blinn_attribs, blinn_varyings, blinn_uniforms> *>(this->program.get());
    auto &uniforms = blinn_program->get_uniforms();
    uniforms.shadow_pass = shadow_pass;

    auto &vertices = mesh->getVertices();
    size_t num_faces = mesh->getNumFaces();

    /*std::cout << "program info: " << std::endl;
    std::cout << "camera position:\n " << blinn_program->shader_uniforms->camera_pos.transpose() << std::endl;
    std::cout << "camera vp\n" << blinn_program->shader_uniforms->camera_vp_matrix << std::endl;*/

    for (size_t i = 0; i < num_faces; ++i)
    {
        //std::cout << "Drawing single triangle*********************************************\n";
        for (int j = 0; j < 3; ++j)
        {
            const auto &vertex = vertices[i * 3 + j];
            auto &attribs = blinn_program->shader_attribs[j];
            attribs.position = vertex.position;
            attribs.texcoord = vertex.texcoord;
            attribs.normal = vertex.normal;
            attribs.joint = vertex.joint;
            attribs.weight = vertex.weight;
            //std::cout << "original vertices: " << attribs.position.transpose() << std::endl;
        }
        graphics_draw_triangle(framebuffer, blinn_program);
    }
}



void Skybox_Model::update(perframe *perframe)
{
    auto skybox_program = static_cast<Program<skybox_attribs, skybox_varyings, skybox_uniforms> *>(this->program.get());
    auto &uniforms = skybox_program->get_uniforms();

    // 移除相机视图矩阵的位移部分
    Eigen::Matrix4f view_matrix = perframe->camera_view_matrix;
    view_matrix.col(3).head<3>().setZero();

    uniforms.vp_matrix = perframe->camera_proj_matrix * view_matrix;
}

void Skybox_Model::draw(Framebuffer *framebuffer, int shadow_pass)
{
    if (shadow_pass || !mesh || !program)
    {
        return; // 天空盒不参与阴影计算
    }

    auto skybox_program = static_cast<Program<skybox_attribs, skybox_varyings, skybox_uniforms> *>(this->program.get());
    
    const auto &vertices = mesh->getVertices();
    size_t num_faces = mesh->getNumFaces();

    for (size_t i = 0; i < num_faces; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            const auto &vertex = vertices[i * 3 + j];
            auto &attribs = skybox_program->shader_attribs[j];
            attribs.position = vertex.position;
        }
        graphics_draw_triangle(framebuffer, skybox_program);
    }
}