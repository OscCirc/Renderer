# 源码阅读顺序

> 按列表顺序阅读，自底向上：从基础数据结构 → 着色器 → 渲染管线 → 场景/模型 → 平台层 → 应用层 → 入口。

## 第一阶段：基础

- [x] `include/utils/global.hpp`          — 全局常量 (EPSILON, PI, to_radians, to_degrees)
- [x] `include/math/math.hpp`             — mat4_lookat, mat4_orthographic, float_clamp
- [x] `src/math/math.cpp`                 — 矩阵函数实现
- [x] `include/utils/resource_cache.hpp`  — ResourceCache 模板基类 (weak_ptr 缓存)
- [x] `include/core/framebuffer.hpp`      — 颜色缓冲 + 深度缓冲
- [x] `src/core/framebuffer.cpp`          — clear/set/get 实现
- [x] `include/core/image.hpp`            — stb_image 封装, LDR/HDR 加载保存
- [x] `src/core/image.cpp`                — 图像加载/翻转/格式转换实现

## 第二阶段：几何数据

- [x] `include/geometry/mesh.hpp`         — vertex_attribs, Mesh, MeshCache
- [x] `src/geometry/mesh.cpp`             — OBJ 加载, buildMesh
- [x] `include/geometry/skeleton.hpp`     — Joint, Skeleton, SkeletonCache, 骨骼动画
- [x] `src/geometry/skeleton.cpp`         — 骨骼加载, update, 关节矩阵计算
- [x] `include/geometry/camera.hpp`       — Camera_Base, Camera, TargetCamera
- [x] `src/geometry/camera.cpp`           — view/projection 矩阵实现

## 第三阶段：着色与纹理

- [x] `include/shading/shader.hpp`        — vertex_shader_t, fragment_shader_t 类型定义
- [x] `include/shading/light.hpp`         — light 结构体
- [x] `include/shading/texture.hpp`       — Texture, Cubemap, TextureCache, Cubemap_cache
- [x] `src/shading/texture.cpp`           — 纹理加载, 采样 (Nearest/Bilinear/Trilinear), mipmap
- [ ] `include/shading/Blinn_shader.hpp`  — blinn_attribs/varyings/uniforms, shader 函数声明
- [ ] `src/shading/Blinn_shader.cpp`      — Blinn-Phong 顶点/片段着色器实现
- [ ] `include/shading/skybox_shader.hpp` — skybox_attribs/varyings/uniforms, shader 函数声明
- [ ] `src/shading/skybox_shader.cpp`     — 天空盒顶点/片段着色器实现

## 第四阶段：渲染管线

- [ ] `include/core/graphics.hpp`         — Program<A,V,U> 模板, clip_triangle (7平面裁剪), graphics_draw_triangle
- [ ] `include/core/rasterizer.hpp`       — rasterize_polygon, rasterize_triangle 声明
- [ ] `src/core/rasterizer.cpp`           — 扫描线光栅化, 重心插值, 深度测试, 透视校正, mipmap LOD, alpha混合

## 第五阶段：场景与模型

- [ ] `include/geometry/model.hpp`        — Model_Base, Blinn_Phong_Model, Skybox_Model
- [ ] `src/geometry/model.cpp`            — update(perframe), draw(逐面遍历 → graphics_draw_triangle)
- [ ] `include/core/scene.hpp`            — Scene, perframe, 场景对象容器
- [ ] `src/core/scene.cpp`                — SceneLoader, .scn 文件解析
- [ ] `include/core/scene_creator.hpp`    — 场景工厂函数注册表声明
- [ ] `src/core/scene_creator.cpp`        — 13 个 create_blinn_*_scene() 实现

## 第六阶段：平台与应用层

- [ ] `include/platform/window.hpp`       — Win32Window, 窗口创建, 事件, GDI blit
- [ ] `src/platform/window.cpp`           — WndProc, 消息泵, present_framebuffer
- [ ] `include/app/input_manager.hpp`     — InputManager, InputState, 鼠标/键盘回调
- [ ] `src/app/input_manager.cpp`         — 输入轮询, 轨道/平移/推拉计算
- [ ] `include/app/frame_timer.hpp`       — FrameTimer, 帧计时, FPS 打印
- [ ] `src/app/frame_timer.cpp`           — begin_frame, maybe_print_fps
- [ ] `include/app/application.hpp`       — Application 主循环, tick, render_scene
- [ ] `src/app/application.cpp`           — run(), process_input, shadow/main pass 调度

## 第七阶段：入口

- [ ] `src/main.cpp`                      — 场景注册, 命令行分派, Application 生命周期

## 附录（可选）

- [ ] `include/utils/OBJ_Loader.h`        — 第三方 OBJ 解析库
- [ ] `CMakeLists.txt`                    — 构建配置
