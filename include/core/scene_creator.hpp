#pragma once

#include "core/scene.hpp"
#include <memory>
#include <string>
#include <functional>
#include <map>
#include <vector>

// 使用std::function 定义场景创建器的类型
using SceneCreatorFunc = std::function<std::unique_ptr<Scene>()>;

// 声明一个函数来获取所有场景创建者的映射
std::map<std::string, SceneCreatorFunc>& get_scene_creators();

// 声明一个函数来注册场景创建者
void register_scene_creator(const std::string& name, SceneCreatorFunc func);

// 声明一个函数来获取所有场景名称
std::vector<std::string> get_available_scenes();

// --- 声明具体的场景创建函数 ---

std::unique_ptr<Scene> create_blinn_azura_scene();
std::unique_ptr<Scene> create_blinn_centaur_scene();
std::unique_ptr<Scene> create_blinn_craftsman_scene();
std::unique_ptr<Scene> create_blinn_elfgirl_scene();
std::unique_ptr<Scene> create_blinn_kgirl_scene();
std::unique_ptr<Scene> create_blinn_lighthouse_scene();
std::unique_ptr<Scene> create_blinn_mccree_scene();
std::unique_ptr<Scene> create_blinn_nier2b_scene();
std::unique_ptr<Scene> create_blinn_phoenix_scene();
std::unique_ptr<Scene> create_blinn_vivi_scene();
std::unique_ptr<Scene> create_blinn_whip_scene();
std::unique_ptr<Scene> create_blinn_witch_scene();
std::unique_ptr<Scene> create_sampling_test_scene();

