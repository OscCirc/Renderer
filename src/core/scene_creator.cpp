#include "core/scene_creator.hpp"
#include "geometry/model.hpp" // 需要包含 Model 子类
#include "utils/resource_cache.hpp" 

std::map<std::string, SceneCreatorFunc>& get_scene_creators()
{
    static std::map<std::string, SceneCreatorFunc> creators;
    return creators;
}

void register_scene_creator(const std::string& name, SceneCreatorFunc func)
{
    get_scene_creators()[name] = func;
}

std::vector<std::string> get_available_scenes()
{
    std::vector<std::string> names;
    for (const auto& pair : get_scene_creators()) {
        names.push_back(pair.first);
    }
    return names;
}

// --- 实现具体的场景创建函数 ---

std::unique_ptr<Scene> create_azura_scene()
{

    Eigen::Affine3f translation(Eigen::Translation3f(-6.073f, -1.278f, 0.280f));
    Eigen::Affine3f scale(Eigen::Scaling(0.378f));


    Eigen::Matrix4f root_transform = (scale * translation).matrix();

    try {
        return std::make_unique<Scene>("assets/azura/azura.scn", root_transform);
    } catch (const std::exception& e) {
        std::cerr << "Failed to create azura scene: " << e.what() << std::endl;
        return nullptr;
    }
}