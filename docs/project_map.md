# Project Map

> 这份文档用于重新接手项目时快速恢复上下文。它记录当前代码的真实状态、主要调用链、文档维护方式，以及下一步开发前值得优先整理的区域。

## 1. 快速恢复工作状态

### 构建

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

当前项目依赖：

- CMake + Visual Studio/MSVC
- Eigen3
- stb
- vcpkg toolchain，当前 `build/CMakeCache.txt` 指向本机 `C:/mqy/repo/vcpkg-2026.02.27/scripts/buildsystems/vcpkg.cmake`

### 运行

```powershell
.\build\bin\Debug\SoftRenderer.exe azura
.\build\bin\Debug\SoftRenderer.exe phoenix
.\build\bin\Debug\SoftRenderer.exe sampling_test
```

当前 `main.cpp` 的命令行只接收一个参数：`scene_name`。如果不传参数，会随机选择一个已注册场景。

### 交互

- 鼠标左键拖拽：轨道相机旋转
- 鼠标右键拖拽：相机平移
- 鼠标滚轮：推拉相机
- `Space`：重置相机和光照
- `W/A/S/D`：调整光照方向
- `1/2/3`：切换纹理采样模式，分别为 Nearest、Bilinear、Trilinear

## 2. 目录与职责

```text
include/
  app/        应用层：Application、InputManager、FrameTimer
  core/       渲染核心：Scene、Framebuffer、Graphics、Rasterizer、Image
  geometry/   几何数据：Camera、Model、Mesh、Skeleton、Triangle
  math/       矩阵和向量辅助函数
  platform/   Win32 窗口、输入事件、framebuffer 显示
  shading/    Shader、Blinn-Phong、Skybox、Texture
  utils/      ResourceCache、OBJ_Loader、全局常量

src/          与 include/ 对应的实现
assets/       场景、模型、贴图、HDR cubemap
docs/         维护文档与历史重构记录
outputs/      示例渲染结果
```

## 3. 当前主流程

```text
src/main.cpp
  -> 注册场景创建函数
  -> 根据 scene_name 创建 Scene
  -> Application app(...)
  -> app.run()

Application::run()
  -> timer_.begin_frame(...)
  -> tick()
  -> window_->present_framebuffer(...)
  -> timer_.maybe_print_fps()
  -> window_->poll_events()

Application::tick()
  -> input_manager_->poll(...)
  -> process_input()
  -> update_camera()
  -> update_light_direction(...)
  -> scene_->update_per_frame_data(...)
  -> render_scene()
```

当前 `Application` 已经拆出了 `FrameTimer` 和 `InputManager`，但相机控制、光照控制、渲染调度仍在 `Application` 内。`docs/refactor_application.md` 是后续继续拆分的历史计划，其中 `SceneRenderer`、`CameraController`、`LightController` 还没有完全落地。

## 4. 场景加载链路

```text
main.cpp
  -> REGISTER_SCENE(...)
  -> create_blinn_xxx_scene()
  -> Scene("assets/xxx/xxx.scn", root_transform)
  -> SceneLoader::read_light/read_blinn_materials/read_transforms/read_models
  -> Blinn_Phong_Model(...)
  -> cache_acquire_mesh/cache_acquire_skeleton/acquire_color_texture
```

`.scn` 文件目前支持 `type: blinn`，主要分区：

- `lighting`：背景色、环境贴图、天空盒、阴影、环境光强度、方向光强度
- `materials`：basecolor、shininess、diffuse/specular/emission/normal map、双面、透明混合、alpha cutoff
- `transforms`：每个模型的局部矩阵
- `models`：mesh、skeleton、attached、material、transform 索引

注意：`Scene::Scene` 会先按传入路径打开 `.scn`，之后调用 `SceneLoader::initialize_platform()` 切换工作目录到可执行文件旁边的 `assets` 目录。因此纹理、OBJ、HDR 的路径通常是相对 `assets/` 的路径。

## 5. 渲染链路

### 每帧场景更新

`Application::tick()` 计算当前帧时间、相机矩阵、光源矩阵，然后调用：

```cpp
scene_->update_per_frame_data(...)
```

这些数据会进入 `Scene::frame_data`，随后每个模型在 `Model_Base::update()` 中把 per-frame 数据转写到 shader uniforms。

### render_scene

```text
render_scene()
  -> Model::update(perframe)
  -> Skybox::update(perframe)
  -> shadow pass
     -> sort_models(light_view_matrix)
     -> shadow_buffer clear_depth
     -> opaque model draw(shadow_pass = 1)
     -> shadow_map update_from_depth_buffer
  -> main pass
     -> sort_models(camera_view_matrix)
     -> framebuffer clear_color + clear_depth
     -> opaque models
     -> skybox
     -> transparent models
```

透明模型依赖排序：不透明模型先按近到远，透明模型按远到近。`Model_Base::opaque` 由材质的 `enable_blend` 决定。

### 单个三角形

```text
Blinn_Phong_Model::draw
  -> 逐 face 填充 shader_attribs
  -> graphics_draw_triangle
     -> vertex shader
     -> clip_triangle，7 平面裁剪
     -> rasterize_polygon，裁剪后多边形拆成三角扇
     -> rasterize_triangle
        -> NDC / viewport transform
        -> backface culling
        -> bounding box
        -> barycentric coverage
        -> depth test
        -> perspective-correct interpolation
        -> mipmap LOD 计算
        -> fragment shader
        -> alpha blend 或写入 color/depth
```

## 6. 着色和纹理

当前主要 shader 是 Blinn-Phong：

- 顶点阶段支持骨骼蒙皮和附着到骨骼节点
- 法线贴图通过 TBN 把切线空间法线转换到世界空间
- diffuse/specular/emission/normal 贴图按材质配置加载
- alpha cutoff 在阴影 pass 和主 pass 都会参与丢弃片段
- shadow map 使用深度比较和 slope bias

纹理系统：

- `TextureUsage::Color` / `Linear` 用于区分颜色贴图和线性数据
- 全局采样模式是 `g_sample_mode`
- `Texture` 构造时生成 mipmaps
- `rasterizer.cpp` 对 `blinn_varyings` 计算 `dUV_dx/dUV_dy`，fragment shader 根据纹理尺寸计算 LOD

## 7. 资源缓存

资源缓存基于 `std::weak_ptr`：

- `MeshCache`
- `SkeletonCache`
- `TextureCache`
- `Cubemap_cache`

模型、骨骼、贴图由 `shared_ptr` 持有；缓存层只保留弱引用。这样重复场景对象可以复用资源，但最后一个使用者释放后资源也能自然释放。

## 8. 当前已知文档和代码漂移

- `README.md` 中部分命令看起来是旧格式，例如 `SoftRenderer blinn azura`；当前入口实际是 `SoftRenderer.exe azura`。
- `docs/refactor_application.md` 是“拆分 Application”的计划和历史记录，不完全等于当前代码状态。
- `include/app/application.hpp` 仍声明了 `init_window()`，但当前代码没有实现和调用。
- `docs/Geometry.md` 和部分平台层注释显示为乱码，后续可以统一确认编码并转换为 UTF-8。
- `TODO.md` 适合作为路线图，但每次勾选任务后需要同步更新底部进度统计。

## 9. 推荐的下一步维护顺序

1. 固化运行基线  
   每次开始较大改动前，先确认 `azura`、`phoenix`、`sampling_test` 至少能构建和启动。

2. 清理文档入口  
   让 `README.md` 只承担构建、运行、文档索引；架构细节放 `ARCHITECTURE.md` 和本文件。

3. 继续拆分 Application  
   优先从 `render_scene()` 拆出 `SceneRenderer`，再拆 `LightController`、`CameraController`。每拆一步都构建一次。

4. 建立开发日志  
   重大设计决定写到 `docs/dev_notes.md`，格式可以保持：

   ```md
   ## YYYY-MM-DD - 标题

   背景：
   决定：
   影响：
   后续：
   ```

5. 规划 PBR 前先整理材质边界  
   当前 `.scn`、`blinn_material`、`Blinn_Phong_Model` 绑定较紧。做 PBR 前，建议先定义材质类型如何选择 shader、如何加载贴图、如何和旧场景兼容。

## 10. 修改模块时同步更新哪些文档

| 修改内容 | 同步文档 |
| --- | --- |
| 构建依赖、CMake、vcpkg | `README.md`、`docs/project_map.md` |
| 主循环、模块职责、渲染管线 | `ARCHITECTURE.md`、`docs/project_map.md` |
| Application 拆分 | `docs/refactor_application.md`、`docs/project_map.md` |
| 新 shader、新材质、新场景格式 | `ARCHITECTURE.md`、`docs/project_map.md`、`TODO.md` |
| 下一步开发计划 | `TODO.md`、必要时新增 `docs/dev_notes.md` |

