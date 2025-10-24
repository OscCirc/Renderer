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

std::unique_ptr<Scene> create_azura_scene();
