#include "core/scene.hpp"
#include "geometry/model.hpp"
#include "shading/blinn_shader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>
#include <windows.h>

namespace SceneLoader
{
    void initialize_platform() {
        // 设置工作目录到assets文件夹
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        *wcsrchr(path, L'\\') = L'\0';
        SetCurrentDirectoryW(path);

        // 尝试进入assets目录
        std::cout << "Trying to enter assets directory" << std::endl;

        if (SetCurrentDirectoryW(L"assets") == 0) {
            // 如果失败，可能是在不同的目录结构中，尝试其他路径
            SetCurrentDirectoryW(L"../assets");
        }
    }
    struct LightData
    {
        Eigen::Vector3f background;
        std::string environment;
        std::string skybox;
        std::string shadow;
        float ambient;
        float punctual;
    };

    struct BlinnMaterialData
    {
        int index;
        Eigen::Vector4f basecolor;
        float shininess;
        std::string diffuse_map;
        std::string specular_map;
        std::string emission_map;   // 自发光贴图
        std::string normal_map;     // 法线贴图
        std::string double_sided;   // 背面剔除
        std::string enable_blend;   // 透明混合
        float alpha_cutoff;         // 透明度裁剪阈值  
    };

    struct TransformData
    {
        int index;
        Eigen::Matrix4f matrix;
    };

    struct ModelData
    {
        int index;
        std::string mesh;
        std::string skeleton;
        int attached;
        int material;
        int transform;
    };

    // 辅助函数
    static std::string wrap_path(const std::string &path)
    {
        return (path == "null") ? "" : path;
    }

    static bool wrap_knob(const std::string &knob)
    {
        if (knob == "on") return true;
        if (knob == "off") return false;
        throw std::runtime_error("Invalid knob value: " + knob);
    }

    // 使用 C++ iostream 进行解析的辅助函数
    void read_light(std::ifstream &file, LightData &light)
    {
        std::string key;
        file >> key; // "lighting:"
        file >> key >> light.background.x() >> light.background.y() >> light.background.z(); // "background:"
        file >> key >> light.environment; // "environment:"
        file >> key >> light.skybox;      // "skybox:"
        file >> key >> light.shadow;      // "shadow:"
        file >> key >> light.ambient;     // "ambient:"
        file >> key >> light.punctual;    // "punctual:"
    }

    void read_blinn_materials(std::ifstream &file, std::vector<BlinnMaterialData> &materials)
    {
        std::string key;
        char delimiter;
        int count;
        file >> key >> count >> delimiter; // "materials:"
        materials.resize(count);
        for (int i = 0; i < count; ++i)
        {
            auto &mat = materials[i];
            file >> key >> mat.index >> delimiter; // "material"
            file >> key >> mat.basecolor.x() >> mat.basecolor.y() >> mat.basecolor.z() >> mat.basecolor.w();
            file >> key >> mat.shininess;
            file >> key >> mat.diffuse_map;
            file >> key >> mat.specular_map;
            file >> key >> mat.emission_map;
            file >> key >> mat.normal_map;
            file >> key >> mat.double_sided;
            file >> key >> mat.enable_blend;
            file >> key >> mat.alpha_cutoff;
        }
    }

    void read_transforms(std::ifstream &file, std::vector<TransformData> &transforms)
    {
        std::string key;
        int count;
        char delimiter;
        file >> key >> count >> delimiter; // "transforms:"
        transforms.resize(count);
        for (int i = 0; i < count; ++i)
        {
            auto &trans = transforms[i];
            file >> key >> trans.index >> delimiter; // "transform"
            for (int row = 0; row < 4; ++row)
                for (int col = 0; col < 4; ++col)
                    file >> trans.matrix(row, col);
        }
    }

    void read_models(std::ifstream &file, std::vector<ModelData> &models)
    {
        std::string key;
        int count;
        char delimiter;
        file >> key >> count >> delimiter;
        models.resize(count);
        for (int i = 0; i < count; ++i)
        {
            auto &model = models[i];
            file >> key >> model.index >> delimiter;
            file >> key >> model.mesh;
            file >> key >> model.skeleton;
            file >> key >> model.attached;
            file >> key >> model.material;
            file >> key >> model.transform;
        }
    }
    
    // 底层辅助函数，负责组装场景的全局属性
    void create_scene(Scene &scene,
                      const LightData &light_data,
                      std::vector<std::shared_ptr<Model_Base>> &&models)
    {
        // 1. 将模型列表移动到 Scene 对象中
        scene.models = std::move(models);

        // 2. 创建天空盒
        if (light_data.skybox != "off")
        {
            std::string env_name = wrap_path(light_data.environment);
            int blur_level = 0;
            if (light_data.skybox == "ambient") blur_level = -1;
            else if (light_data.skybox == "blurred") blur_level = 1;
            
            if (!env_name.empty())
            {
                scene.skybox = std::make_shared<Skybox_Model>(env_name, blur_level);
            }
        }

        // 3. 设置阴影
        if (light_data.shadow != "off")
        {
            int shadow_width = 512, shadow_height = 512;
            if (light_data.shadow != "on")
            {
                sscanf(light_data.shadow.c_str(), "%dx%d", &shadow_width, &shadow_height);
            }
            scene.shadow_buffer = std::make_unique<Framebuffer>(shadow_width, shadow_height);
            scene.shadow_map = std::make_unique<Texture>(shadow_width, shadow_height);
        }

        // 4. 设置其他场景属性
        scene.background_color = Eigen::Vector4f(light_data.background.x(), light_data.background.y(), light_data.background.z(), 1.0f);
        scene.frame_data.ambient_intensity = light_data.ambient;
        scene.frame_data.punctual_intensity = light_data.punctual;
    }

    // 中层辅助函数，负责创建特定类型的模型
    void create_blinn_scene(Scene &scene,
                            const LightData &light_data,
                            const std::vector<BlinnMaterialData> &materials,
                            const std::vector<TransformData> &transforms,
                            const std::vector<ModelData> &model_data_list,
                            const Eigen::Matrix4f &root_transform)
    {
        std::cout << "Initializing models" << std::endl;
        std::vector<std::shared_ptr<Model_Base>> models;
        for (const auto &model_data : model_data_list)
        {
            const auto &material_data = materials.at(model_data.material);
            const auto &transform_data = transforms.at(model_data.transform);

            Eigen::Matrix4f transform = root_transform * transform_data.matrix;

            blinn_material cpp_material;
            cpp_material.basecolor = material_data.basecolor;
            cpp_material.shininess = material_data.shininess;
            cpp_material.diffuse_map = wrap_path(material_data.diffuse_map);
            cpp_material.specular_map = wrap_path(material_data.specular_map);
            cpp_material.emission_map = wrap_path(material_data.emission_map);
            cpp_material.normal_map = wrap_path(material_data.normal_map);
            cpp_material.double_sided = wrap_knob(material_data.double_sided);
            cpp_material.enable_blend = wrap_knob(material_data.enable_blend);
            cpp_material.alpha_cutoff = material_data.alpha_cutoff;

            auto model = std::make_shared<Blinn_Phong_Model>(
                wrap_path(model_data.mesh), transform,
                wrap_path(model_data.skeleton), model_data.attached,
                cpp_material);
            models.push_back(model);
        }

        create_scene(scene, light_data, std::move(models));
    }

} // namespace SceneLoader


Scene::Scene(const std::string &filename, const Eigen::Matrix4f &root_transform)
{
    std::cout << "Current directory: " << std::filesystem::current_path() << std::endl;
    std::ifstream file(filename);
    if (!file.is_open())
    {
        throw std::runtime_error("Error: Could not open scene file: " + filename);
    }

    std::string key, scene_type;
    file >> key >> scene_type;

    if (scene_type == "blinn")
    {
        std::cout << "Scene using Blinn Phong" << std::endl;
        std::cout << "Reading light" << std::endl;
        SceneLoader::LightData light_data;
        SceneLoader::read_light(file, light_data);

        std::cout << "Reading blinn materials" << std::endl;
        std::vector<SceneLoader::BlinnMaterialData> materials;
        SceneLoader::read_blinn_materials(file, materials);

        std::cout << "Reading transforms" << std::endl;
        std::vector<SceneLoader::TransformData> transforms;
        SceneLoader::read_transforms(file, transforms);

        std::cout << "Reading models" << std::endl;
        std::vector<SceneLoader::ModelData> model_data_list;
        SceneLoader::read_models(file, model_data_list);

        SceneLoader::initialize_platform();
        std::cout << "Current directory: " << std::filesystem::current_path() << std::endl;
        // 调用中层辅助函数来创建 Blinn 场景
        SceneLoader::create_blinn_scene(*this, light_data, materials, transforms, model_data_list, root_transform);
    }
    else
    {
        // TODO: 实现 PBRM 和 PBRS 场景的加载
        throw std::runtime_error("Unsupported scene type: " + scene_type);
    }

    this->camera = nullptr; // Camera 在外部设置
}