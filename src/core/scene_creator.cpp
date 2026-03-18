#include "core/scene_creator.hpp"
#include "geometry/model.hpp" // 需要包含 Model 子类
#include "utils/resource_cache.hpp"

// 定义 float 精度的 PI，避免 EIGEN_PI (long double) 截断警告
static constexpr float PI_F = static_cast<float>(EIGEN_PI);

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

std::unique_ptr<Scene> create_blinn_azura_scene()
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
std::unique_ptr<Scene> create_blinn_centaur_scene()
{
    Eigen::Affine3f translation(Eigen::Translation3f(0.154f, -7.579f, -30.749f));
    Eigen::Affine3f rotation_x(Eigen::AngleAxisf(-PI_F / 2.0f, Eigen::Vector3f::UnitX()));
    Eigen::Affine3f rotation_y(Eigen::AngleAxisf(-PI_F / 2.0f, Eigen::Vector3f::UnitY()));
    Eigen::Affine3f rotation = rotation_y * rotation_x;
    Eigen::Affine3f scale(Eigen::Scaling(0.016f));

    Eigen::Matrix4f root_transform = (scale * rotation * translation).matrix();

    try {
        return std::make_unique<Scene>("assets/centaur/centaur.scn", root_transform);
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to create centaur scene: " << e.what() << std::endl;
        return nullptr;
    }
}

std::unique_ptr<Scene> create_blinn_craftsman_scene()
{
    Eigen::Affine3f translation(Eigen::Translation3f(-1.668f, -27.061f, -10.834f));
    Eigen::Affine3f scale(Eigen::Scaling(0.016f));

    Eigen::Matrix4f root_transform = (scale * translation).matrix();

    try {
        return std::make_unique<Scene>("assets/craftsman/craftsman.scn", root_transform);
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to create craftsman scene: " << e.what() << std::endl;
        return nullptr;
    }
}

std::unique_ptr<Scene> create_blinn_elfgirl_scene()
{
    Eigen::Affine3f translation(Eigen::Translation3f(2.449f, -2.472f, -20.907f));
    Eigen::Affine3f rotation(Eigen::AngleAxisf(-PI_F / 2.0f, Eigen::Vector3f::UnitX()));
    Eigen::Affine3f scale(Eigen::Scaling(0.023f));

    Eigen::Matrix4f root_transform = (scale * rotation * translation).matrix();

    try {
        return std::make_unique<Scene>("assets/elfgirl/elfgirl.scn", root_transform);
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to create elfgirl scene: " << e.what() << std::endl;
        return nullptr;
    }
}

std::unique_ptr<Scene> create_blinn_kgirl_scene()
{
    Eigen::Affine3f translation(Eigen::Translation3f(0, -4.937f, -96.547f));
    Eigen::Affine3f rotation_x(Eigen::AngleAxisf(-PI_F / 2.0f, Eigen::Vector3f::UnitX()));
    Eigen::Affine3f rotation_y(Eigen::AngleAxisf(PI_F / 2.0f, Eigen::Vector3f::UnitY()));
    Eigen::Affine3f rotation = rotation_y * rotation_x;
    Eigen::Affine3f scale(Eigen::Scaling(0.005f));

    Eigen::Matrix4f root_transform = (scale * rotation * translation).matrix();

    try {
        return std::make_unique<Scene>("assets/kgirl/kgirl.scn", root_transform);
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to create kgirl scene: " << e.what() << std::endl;
        return nullptr;
    }
}

std::unique_ptr<Scene> create_blinn_lighthouse_scene()
{
    Eigen::Affine3f translation(Eigen::Translation3f(-78.203f, -222.929f, 16.181f));
    Eigen::Affine3f rotation(Eigen::AngleAxisf(-3.0f * PI_F / 4.0f, Eigen::Vector3f::UnitY())); // -135度
    Eigen::Affine3f scale(Eigen::Scaling(0.0016f));

    Eigen::Matrix4f root_transform = (scale * rotation * translation).matrix();

    try {
        return std::make_unique<Scene>("assets/lighthouse/lighthouse.scn", root_transform);
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to create lighthouse scene: " << e.what() << std::endl;
        return nullptr;
    }
}

std::unique_ptr<Scene> create_blinn_mccree_scene()
{
    Eigen::Affine3f translation(Eigen::Translation3f(0.108f, -1.479f, 0.034f));
    Eigen::Affine3f scale(Eigen::Scaling(0.337f));

    Eigen::Matrix4f root_transform = (scale * translation).matrix();

    try {
        return std::make_unique<Scene>("assets/mccree/mccree.scn", root_transform);
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to create mccree scene: " << e.what() << std::endl;
        return nullptr;
    }
}

std::unique_ptr<Scene> create_blinn_nier2b_scene()
{
    Eigen::Affine3f translation(Eigen::Translation3f(4.785f, -105.275f, -23.067f));
    Eigen::Affine3f rotation(Eigen::AngleAxisf(PI_F / 2.0f, Eigen::Vector3f::UnitY()));
    Eigen::Affine3f scale(Eigen::Scaling(0.004f));

    Eigen::Matrix4f root_transform = (scale * rotation * translation).matrix();

    try {
        return std::make_unique<Scene>("assets/nier2b/nier2b.scn", root_transform);
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to create nier2b scene: " << e.what() << std::endl;
        return nullptr;
    }
}

std::unique_ptr<Scene> create_blinn_phoenix_scene()
{
    Eigen::Affine3f translation(Eigen::Translation3f(376.905f, -169.495f, 0));
    Eigen::Affine3f rotation(Eigen::AngleAxisf(PI_F, Eigen::Vector3f::UnitY()));
    Eigen::Affine3f scale(Eigen::Scaling(0.001f));

    Eigen::Matrix4f root_transform = (scale * rotation * translation).matrix();

    try {
        return std::make_unique<Scene>("assets/phoenix/phoenix.scn", root_transform);
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to create phoenix scene: " << e.what() << std::endl;
        return nullptr;
    }
}

std::unique_ptr<Scene> create_blinn_vivi_scene()
{
    Eigen::Affine3f translation(Eigen::Translation3f(0, 0, -1.369f));
    Eigen::Affine3f rotation(Eigen::AngleAxisf(-PI_F / 2.0f, Eigen::Vector3f::UnitX()));
    Eigen::Affine3f scale(Eigen::Scaling(0.331f));

    Eigen::Matrix4f root_transform = (scale * rotation * translation).matrix();

    try {
        return std::make_unique<Scene>("assets/vivi/vivi.scn", root_transform);
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to create vivi scene: " << e.what() << std::endl;
        return nullptr;
    }
}

std::unique_ptr<Scene> create_blinn_whip_scene()
{
    Eigen::Affine3f translation(Eigen::Translation3f(-3732.619f, -93.643f, -1561.663f));
    Eigen::Affine3f rotation(Eigen::AngleAxisf(PI_F / 2.0f, Eigen::Vector3f::UnitX()));
    Eigen::Affine3f scale(Eigen::Scaling(0.0004f));

    Eigen::Matrix4f root_transform = (scale * rotation * translation).matrix();

    try {
        return std::make_unique<Scene>("assets/whip/whip.scn", root_transform);
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to create whip scene: " << e.what() << std::endl;
        return nullptr;
    }
}

std::unique_ptr<Scene> create_blinn_witch_scene()
{
    Eigen::Affine3f translation(Eigen::Translation3f(-17.924f, -16.974f, -32.691f));
    Eigen::Affine3f rotation(Eigen::AngleAxisf(-PI_F / 2.0f, Eigen::Vector3f::UnitX()));
    Eigen::Affine3f scale(Eigen::Scaling(0.02f));

    Eigen::Matrix4f root_transform = (scale * rotation * translation).matrix();

    try {
        return std::make_unique<Scene>("assets/witch/witch.scn", root_transform);
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to create witch scene: " << e.what() << std::endl;
        return nullptr;
    }
}