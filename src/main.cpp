#include "core/application.hpp"
#include "core/scene_creator.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>

#define REGISTER_SCENE(name, func) \
    static SceneAutoRegister auto_register_##name(#name, func)

// 自动注册场景的辅助类
struct SceneAutoRegister {
    SceneAutoRegister(const std::string& name, SceneCreatorFunc func) {
        register_scene_creator(name, func);
    }
};

// 使用宏来简化注册过程，静态存储期（包括全局变量和 static 全局变量）的对象，其初始化（即构造函数的调用）必须在 main 函数执行之前完成。


REGISTER_SCENE(azura, create_blinn_azura_scene);
REGISTER_SCENE(centaur, create_blinn_centaur_scene);
REGISTER_SCENE(craftsman, create_blinn_craftsman_scene);
REGISTER_SCENE(elfgirl, create_blinn_elfgirl_scene);
REGISTER_SCENE(kgirl, create_blinn_kgirl_scene);
REGISTER_SCENE(lighthouse, create_blinn_lighthouse_scene);
REGISTER_SCENE(mccree, create_blinn_mccree_scene);
REGISTER_SCENE(nier2b, create_blinn_nier2b_scene);
REGISTER_SCENE(phoenix, create_blinn_phoenix_scene);
REGISTER_SCENE(vivi, create_blinn_vivi_scene);
REGISTER_SCENE(whip, create_blinn_whip_scene);
REGISTER_SCENE(witch, create_blinn_witch_scene);
REGISTER_SCENE(sampling_test, create_sampling_test_scene);

void print_usage() {
    std::cout << "Usage: SoftRenderer.exe [scene_name]" << std::endl;
    std::cout << "Available scenes: " << std::endl;
    for (const auto& name : get_available_scenes()) {
        std::cout << "  - " << name << std::endl;
    }
}

int main(int argc, char** argv)
{
    // 获取所有已注册的场景创建函数
    auto& creators = get_scene_creators();
    if (creators.empty()) {
        std::cerr << "No scenes have been registered!" << std::endl;
        return EXIT_FAILURE;
    }

    std::string scene_name;
    if (argc > 1) {
        scene_name = argv[1];
    } else {
        // 如果没有提供场景名，随机选择一个
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        int index = std::rand() % creators.size();
        auto it = creators.begin();
        std::advance(it, index);
        scene_name = it->first;
    }

    // 查找并创建场景
    auto it = creators.find(scene_name);
    if (it == creators.end()) {
        std::cerr << "Scene not found: " << scene_name << std::endl;
        print_usage();
        return EXIT_FAILURE;
    }

    std::cout << "Loading scene: " << scene_name << std::endl;
    std::unique_ptr<Scene> scene = it->second();
    if (!scene) {
        std::cerr << "Failed to create scene: " << scene_name << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Running" << std::endl;
    // 运行应用
    try {
        Application app(800, 600, "Software Renderer - " + scene_name, std::move(scene));
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}