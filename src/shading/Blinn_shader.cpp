#include "shading/Blinn_shader.hpp"

/* Vertex Shader */

// 获取模型矩阵
static Eigen::Matrix4f get_model_matrix(const blinn_attribs& attribs, blinn_uniforms& uniforms){
    
    Eigen::Matrix4f skin_matrix = Eigen::Matrix4f::Identity();

    if(uniforms.joint_matrices){ // 蒙皮
        skin_matrix = Eigen::Matrix4f::Zero();
        for(int i=0;i<4;i++){
            float w = attribs.weight[i];
            if (w <= 0.0f) continue; // 跳过权重为0的分量

            int joint_index = attribs.joint[i];
            if (joint_index < 0) continue; // 防御性检查

            // 将该关节的矩阵按权重累加
            skin_matrix += uniforms.joint_matrices[joint_index] * w;
        }
        return uniforms.model_matrix * skin_matrix;
    }
    else{ // 无骨骼或骨骼附着
        return uniforms.model_matrix;
    }

}

// 获取法线矩阵
static Eigen::Matrix3f get_normal_matrix(const blinn_attribs& attribs, blinn_uniforms& uniforms){
    if(uniforms.joint_n_matrices){ // 蒙皮
        Eigen::Matrix3f skin_n_matrix = Eigen::Matrix3f::Zero();
        for(int i=0;i<4;i++){
            float w = attribs.weight[i];
            if (w <= 0.0f) continue; // 跳过权重为0的分量

            int joint_index = attribs.joint[i];
            if (joint_index < 0) continue; // 防御性检查

            // 将该关节的法线矩阵按权重累加
            skin_n_matrix += uniforms.joint_n_matrices[joint_index] * w;
        }
        return uniforms.normal_matrix * skin_n_matrix;
    }
    else{ // 无骨骼或骨骼附着
        return uniforms.normal_matrix;
    }
}

// 仅计算深度图的顶点着色器
static Eigen::Vector4f shadow_vertex_shader(const blinn_attribs& attribs, blinn_varyings& varyings, blinn_uniforms& uniforms){
    Eigen::Matrix4f model_matrix = get_model_matrix(attribs, uniforms);
    Eigen::Matrix4f light_vp_matrix = uniforms.light_vp_matrix;

    Eigen::Vector4f input_position = Eigen::Vector4f(attribs.position.x(), attribs.position.y(), attribs.position.z(), 1.0f);
    Eigen::Vector4f world_position = model_matrix * input_position;
    Eigen::Vector4f depth_position = light_vp_matrix * world_position;

    varyings.texcoord = attribs.texcoord; 

    return depth_position;
}

// 常规着色器，计算顶点裁切空间坐标，并将相关数据传递到varyings
static Eigen::Vector4f common_vertex_shader(const blinn_attribs& attribs, blinn_varyings& varyings, blinn_uniforms& uniforms){ 
    Eigen::Matrix4f model_matrix = get_model_matrix(attribs, uniforms);
    Eigen::Matrix3f normal_matrix = get_normal_matrix(attribs, uniforms);
    Eigen::Matrix4f camera_vp_matrix = uniforms.camera_vp_matrix;
    Eigen::Matrix4f light_vp_matrix = uniforms.light_vp_matrix;

    Eigen::Vector4f input_position = Eigen::Vector4f(attribs.position.x(), attribs.position.y(), attribs.position.z(), 1.0f);
    Eigen::Vector4f world_position = model_matrix * input_position;
    Eigen::Vector4f depth_position = light_vp_matrix * world_position; // 光源视图投影空间位置
    Eigen::Vector4f clip_position = camera_vp_matrix * world_position;

    // world normal
    Eigen::Vector3f input_normal = attribs.normal;
    Eigen::Vector3f normal = (normal_matrix * input_normal).normalized();

    // world tangent
    Eigen::Vector3f input_tangent = attribs.tangent.head<3>();
    Eigen::Vector3f tangent = (normal_matrix * input_tangent).normalized();
    // orthogonalization
    tangent = (tangent - normal * normal.dot(tangent)).normalized();    

    // world bitangent
    float handedness = attribs.tangent.w();
    Eigen::Vector3f bitangent = normal.cross(tangent) * handedness;

    varyings.world_position = world_position.head<3>();
    varyings.depth_position = depth_position.head<3>();
    varyings.texcoord = attribs.texcoord;
    varyings.normal = normal;
    varyings.tangent = tangent;
    varyings.bitangent = bitangent;

    return clip_position;
}

// 主顶点着色器入口
Eigen::Vector4f blinn_vertex_shader(const blinn_attribs& attribs, blinn_varyings& varyings, blinn_uniforms& uniforms){
    if (uniforms.shadow_pass) {
        return shadow_vertex_shader(attribs, varyings, uniforms);
    } else {
        return common_vertex_shader(attribs, varyings, uniforms);
    }
}


/* Fragment Shader */

// 仅计算深度图的片段着色器,丢弃透明片段
static Eigen::Vector4f shadow_fragment_shader(const blinn_varyings& varyings, blinn_uniforms& uniforms, bool* discard){
    if (uniforms.alpha_cutoff > 0) { // 透明度测试
        float alpha = uniforms.basecolor.w();
        if (uniforms.diffuse_map) {
            Eigen::Vector2f texcoord = varyings.texcoord;
            alpha *= uniforms.diffuse_map->sample(texcoord.x(), texcoord.y()).w();
        }
        if (alpha < uniforms.alpha_cutoff) {
            *discard = true;
        }
    }
    return Eigen::Vector4f(0, 0, 0, 0);
}

// 片元着色器材质属性结构体
struct Material {
    Eigen::Vector3f diffuse;  // 基础漫反射颜色
    Eigen::Vector3f specular; // 高光颜色
    float alpha;              // 透明度
    float shininess;          // 高光指数
    Eigen::Vector3f normal;   // 片元世界空间法线
    Eigen::Vector3f emission; // 自发光颜色
};

// 根据varyings和uniforms计算材质属性，backface表示是否为背面渲染
static Material get_material(const blinn_varyings &varyings,
                             const blinn_uniforms &uniforms,
                             bool backface)
{
    Material mat;
    Eigen::Vector2f uv = varyings.texcoord;

    auto compute_lod = [&](const std::shared_ptr<Texture>& tex) -> float{
        if(!tex) return 0.0f;

        Eigen::Vector2f ddx_texel(varyings.dUV_dx.x() * tex->width(), varyings.dUV_dx.y() * tex->height());
        Eigen::Vector2f ddy_texel(varyings.dUV_dy.x() * tex->width(), varyings.dUV_dy.y() * tex->height());

        float rho = std::max(ddx_texel.norm(), ddy_texel.norm());

        return (rho > 0.0f) ? std::max(0.0f, std::log2(rho)) : 0.0f;
    };
    
    // base color
    mat.diffuse = uniforms.basecolor.head<3>();
    mat.alpha = uniforms.basecolor.w();

    // diffuse map
    if (uniforms.diffuse_map) {
        float lod = compute_lod(uniforms.diffuse_map);
        Eigen::Vector4f sample = uniforms.diffuse_map->sample(uv.x(), uv.y(), WrapMode::Repeat, lod);
        mat.diffuse = mat.diffuse.cwiseProduct(sample.head<3>());
        mat.alpha *= sample.w();
    }

    // specular
    mat.specular = Eigen::Vector3f::Zero();
    if (uniforms.specular_map) {
        float lod = compute_lod(uniforms.specular_map);
        Eigen::Vector4f sample = uniforms.specular_map->sample(uv.x(), uv.y(), WrapMode::Repeat, lod);
        mat.specular = sample.head<3>();
    }

    mat.shininess = uniforms.shininess;

    // normal from varyings 
    if(uniforms.normal_map)
    {
        float lod = compute_lod(uniforms.normal_map);
        Eigen::Vector4f sample = uniforms.normal_map->sample(uv.x(), uv.y(), WrapMode::Repeat, lod);

        Eigen::Vector3f tangent_normal{
            sample.x() * 2.0f - 1.0f,
            sample.y() * 2.0f - 1.0f,
            sample.z() * 2.0f - 1.0f
        };
        tangent_normal.normalize();

        Eigen::Vector3f T = varyings.tangent;
        Eigen::Vector3f B = varyings.bitangent;
        Eigen::Vector3f N = varyings.normal;

        Eigen::Matrix3f TBN;
        TBN.col(0) = T;
        TBN.col(1) = B;
        TBN.col(2) = N;

        mat.normal = (TBN * tangent_normal).normalized();
    }
    else{
        mat.normal = varyings.normal.normalized();
    }
    
    if (backface) mat.normal = -mat.normal;

    // emission
    mat.emission = Eigen::Vector3f::Zero();
    if (uniforms.emission_map) {
        float lod = compute_lod(uniforms.emission_map);
        Eigen::Vector4f sample = uniforms.emission_map->sample(uv.x(), uv.y(), WrapMode::Repeat, lod);
        mat.emission = sample.head<3>();
    }

    return mat;
}

// 计算视线方向,view_dir指向摄像机
static Eigen::Vector3f get_view_dir(const blinn_varyings &varyings,
                                    const blinn_uniforms &uniforms)
{
    return (uniforms.camera_pos - varyings.world_position).normalized();
}

// 阴影判定
static bool is_in_shadow(const blinn_varyings &varyings,
                         const blinn_uniforms &uniforms,
                         float n_dot_l)
{
    if (!uniforms.shadow_map) return false;

    // 坐标转换到[0,1]
    float u = (varyings.depth_position.x() + 1.0f) * 0.5f;
    float v = (varyings.depth_position.y() + 1.0f) * 0.5f;
    float d = (varyings.depth_position.z() + 1.0f) * 0.5f;

    // 表面坡度自适应的深度偏移，用来减少阴影判定中的自遮挡伪影
    float depth_bias = std::max(0.05f * (1.0f - n_dot_l), 0.005f);
    float current_depth = d - depth_bias;

    Eigen::Vector4f sample = uniforms.shadow_map->sample(u, v);
    float closest_depth = sample.x();

    return current_depth > closest_depth;
}

// 计算Blinn-Phong模型的高光分量
static Eigen::Vector3f get_specular(const Eigen::Vector3f &light_dir,
                                    const Eigen::Vector3f &view_dir,
                                    const Material &material)
{
    if (!material.specular.isZero(1e-6f)) {
        Eigen::Vector3f half_dir = (light_dir + view_dir).normalized(); // 半程向量
        float n_dot_h = material.normal.dot(half_dir);
        if (n_dot_h > 0.0f) {
            float strength = std::pow(n_dot_h, material.shininess);
            return material.specular * strength;
        }
    }
    return Eigen::Vector3f::Zero();
}

// 常规片段着色器入口，discard用于输出是否丢弃该片段，backface表示是否为背面渲染
static Eigen::Vector4f common_fragment_shader(const blinn_varyings &varyings,
                                              blinn_uniforms &uniforms,
                                              bool *discard,
                                              bool backface)
{
    Material material = get_material(varyings, uniforms, backface);

    if (uniforms.alpha_cutoff > 0.0f && material.alpha < uniforms.alpha_cutoff) {
        if (discard) *discard = true;
        return Eigen::Vector4f(0,0,0,0);
    }

    Eigen::Vector3f color = material.emission;

    // ambient
    if (uniforms.ambient_intensity > 0.0f) {
        color += material.diffuse * uniforms.ambient_intensity;
    }

    // diffuse + specular from punctual light
    if (uniforms.punctual_intensity > 0.0f) {
        Eigen::Vector3f light_dir = -uniforms.light_dir.normalized(); // 使光照方向指向光源
        float n_dot_l = material.normal.dot(light_dir);
        if (n_dot_l > 0.0f && !is_in_shadow(varyings, uniforms, n_dot_l)) {
            Eigen::Vector3f view_dir = get_view_dir(varyings, uniforms);
            Eigen::Vector3f spec = get_specular(light_dir, view_dir, material);
            Eigen::Vector3f diff = material.diffuse * n_dot_l;                  
            Eigen::Vector3f punctual = diff + spec;
            color += punctual * uniforms.punctual_intensity;
        }
    }

    return Eigen::Vector4f(color.x(), color.y(), color.z(), material.alpha);
}

// 主片段着色器入口
Eigen::Vector4f blinn_fragment_shader(const blinn_varyings &varyings,
                                      blinn_uniforms &uniforms,
                                      bool *discard, bool backface)
{
    if (uniforms.shadow_pass) {
        return shadow_fragment_shader(varyings, uniforms, discard);
    } else {
        return common_fragment_shader(varyings, uniforms, discard, backface);
    }
}