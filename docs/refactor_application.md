# Application 拆分实操指南

> 目标：将 God Object `Application` 拆分为多个单一职责的子系统，同时建立 `app/` 和 `ui/` 目录。
> 原则：每一步都很小，做完就编译验证，确保不会翻车。

---

## 目录结构变更总览

### 拆分前

```
include/
├── core/
│   ├── application.hpp    ← God Object，7 种职责
│   ├── framebuffer.hpp
│   ├── graphics.hpp
│   ├── image.hpp
│   ├── rasterizer.hpp
│   ├── scene.hpp
│   └── scene_creator.hpp
├── geometry/
├── shading/
├── math/
├── platform/
└── utils/
```

### 拆分后

```
include/
├── app/                       ← 新增：应用层
│   ├── application.hpp        ← 精简后的主循环协调者
│   ├── input_manager.hpp      ← 鼠标/键盘输入 → InputState
│   ├── camera_controller.hpp  ← InputState → 驱动 Camera
│   ├── light_controller.hpp   ← 按键 → 光照方向
│   ├── scene_renderer.hpp     ← 阴影 + 主渲染通道调度
│   └── frame_timer.hpp        ← 帧率统计
├── core/                      ← 渲染管线核心（不再包含 application）
│   ├── framebuffer.hpp
│   ├── graphics.hpp
│   ├── image.hpp
│   ├── rasterizer.hpp
│   ├── scene.hpp
│   └── scene_creator.hpp
├── ui/                        ← 新增：预留给后续 UI 系统
│   └── ui_context.hpp         ← 占位
├── geometry/                  ← 不变
├── shading/                   ← 不变
├── math/                      ← 不变
├── platform/                  ← 不变
└── utils/                     ← 不变
```

### `app/` 和 `core/` 的区别

| | `core/` | `app/` |
|---|---|---|
| **职责** | 渲染管线的**底层机制** | 用户交互和**应用逻辑** |
| **依赖方向** | 不依赖 `app/` | 依赖 `core/` |
| **变化频率** | 低（光栅化算法稳定） | 高（输入方式、相机行为常改） |
| **举例** | Framebuffer、Rasterizer | InputManager、CameraController |

判断标准：**如果换一个完全不同的应用场景（比如从交互式查看器变成离线批渲染），还需要这个类吗？** 需要 → `core/`，不需要 → `app/`。

### 为什么现在就建 `ui/`

1. **防止 UI 代码被塞进 `app/`** — 后续加 ImGui 面板时，如果没有 `ui/` 目录，你大概率会把 200 行 ImGui 代码塞进 `Application`，它又变成 God Object
2. **UI 层有独立的依赖方向** — `ui/` 依赖 `app/`（读取状态）+ `platform/`（窗口句柄），但 `app/` 不应该依赖 `ui/`

### 依赖关系总览

```
         platform
            ^
    --------+--------
    |       |       |
  core   geometry  math
    ^       ^
    |       |
  shading---+
    ^
    |
   app  <-- 新增层，纯协调逻辑
    ^
    |
   ui   <-- 未来 UI 层，只读 app 的状态
```

**每层只向下依赖，不向上依赖。**

---

## 阶段一：搭骨架（建目录、移文件）

### Step 1：创建目录

创建以下 4 个新目录：

```
include/app/
src/app/
include/ui/
src/ui/
```

### Step 2：移动 application 到 app/

移动文件：

```
include/core/application.hpp  →  include/app/application.hpp
src/core/application.cpp      →  src/app/application.cpp
```

然后做两件事：

1. `application.hpp` 里的 `#include "core/scene.hpp"` 这类路径**不用改**（因为 `include/` 是根搜索路径）
2. 全局搜索 `#include "core/application.hpp"`，改成 `#include "app/application.hpp"`（应该只有 `main.cpp` 引用了它）

> **编译验证** — 行为应该完全不变
>
> **git commit**: `refactor: move application from core/ to app/`

---

## 阶段二：逐步拆分（由易到难）

### Step 3：拆出 FrameTimer（最简单，练手）

**目标：** 把 `run()` 里的帧率统计逻辑搬出去。

#### 3a. 新建 `include/app/frame_timer.hpp`

```cpp
#pragma once
#include <iostream>

class FrameTimer {
public:
    void begin_frame(float current_time);
    void maybe_print_fps();

    float current_time() const;
    float delta_time() const;

private:
    float last_time_ = 0.0f;
    float print_time_ = 0.0f;
    float delta_time_ = 0.0f;
    int frame_count_ = 0;
};
```

#### 3b. 新建 `src/app/frame_timer.cpp`

从 `application.cpp` 的 `run()` 中找到以下逻辑，搬到对应方法里：

```cpp
void FrameTimer::begin_frame(float current_time) {
    delta_time_ = current_time - last_time_;
    last_time_ = current_time;
    frame_count_++;
}

void FrameTimer::maybe_print_fps() {
    if (last_time_ - print_time_ >= 1.0f) {
        int sum_millis = static_cast<int>((last_time_ - print_time_) * 1000);
        int avg_millis = sum_millis / frame_count_;
        std::cout << "fps: " << frame_count_
                  << ", avg: " << avg_millis << " ms" << std::endl;
        frame_count_ = 0;
        print_time_ = last_time_;
    }
}

float FrameTimer::current_time() const { return last_time_; }
float FrameTimer::delta_time() const { return delta_time_; }
```

#### 3c. 修改 Application

**application.hpp：**

```cpp
// 删掉这三个成员变量：
//   float last_frame_time_;
//   float delta_time_;
//   float print_time_;

// 换成：
#include "app/frame_timer.hpp"
// ... 在 private 区域添加：
FrameTimer timer_;
```

**application.cpp — `run()`：**

```cpp
void Application::run() {
    while (!window_->should_close()) {
        timer_.begin_frame(Platform::Win32Window::get_time());
        tick();
        window_->present_framebuffer(*framebuffer_);
        timer_.maybe_print_fps();
        window_->poll_events();
    }
}
```

**application.cpp — `tick()`：**

```cpp
void Application::tick() {
    float current_time = timer_.current_time();
    float delta_time = timer_.delta_time();
    // 后续用这两个局部变量替代原来的 last_frame_time_ 和 delta_time_
    // ...
}
```

> **编译验证** — fps 打印行为和之前一模一样
>
> **git commit**: `refactor: extract FrameTimer from Application`

---

### Step 4：拆出 InputManager（核心，最重要）

这是最大的一块，分三个小步骤。

#### 4a. 定义 InputState 结构体

**新建 `include/app/input_manager.hpp`，先只写数据结构：**

```cpp
#pragma once
#include <Eigen/Core>

// 每帧的输入快照，只读数据，供其他系统消费
struct InputState {
    Eigen::Vector2f orbit_delta{ 0, 0 };
    Eigen::Vector2f pan_delta{ 0, 0 };
    float dolly_delta = 0.0f;

    bool single_click = false;
    bool double_click = false;
    Eigen::Vector2f click_pos{ 0, 0 };
};
```

**验证点：** 在 `application.cpp` 里 `#include "app/input_manager.hpp"`，创建一个 `InputState state;`，编译通过就行，不用实际使用。

> **编译验证**

#### 4b. 搬移回调和输入状态

在 `input_manager.hpp` 里补全 `InputManager` 类声明：

```cpp
// 在 InputState 结构体下方添加：

namespace Platform { class Win32Window; }

class InputManager {
public:
    // 构造时注册回调到 window
    InputManager(Platform::Win32Window& window, int screen_height);

    // 每帧调用一次，返回本帧输入快照，内部自动重置增量
    InputState poll(float current_time);

private:
    void on_mouse_button(Platform::MouseButton button, bool pressed);
    void on_scroll(float offset);
    void on_cursor_pos(double xpos, double ypos);
    Eigen::Vector2f get_pos_delta(const Eigen::Vector2f& old_pos,
                                   const Eigen::Vector2f& new_pos) const;

    Platform::Win32Window& window_;
    int screen_height_;

    // —— 以下全部从 Application::InputRecord 搬过来 ——
    bool is_orbiting_ = false;
    Eigen::Vector2f orbit_pos_{ 0, 0 };
    Eigen::Vector2f orbit_delta_{ 0, 0 };

    bool is_panning_ = false;
    Eigen::Vector2f pan_pos_{ 0, 0 };
    Eigen::Vector2f pan_delta_{ 0, 0 };

    float dolly_delta_ = 0.0f;

    // 点击检测
    static constexpr float CLICK_DELAY = 0.25f;
    static constexpr float CLICK_DISTANCE_THRESHOLD = 5.0f;
    float press_time_ = 0.0f;
    float release_time_ = 0.0f;
    Eigen::Vector2f press_pos_{ 0, 0 };
    Eigen::Vector2f release_pos_{ 0, 0 };
    bool single_click_ = false;
    bool double_click_ = false;
    Eigen::Vector2f click_pos_{ 0, 0 };
};
```

**新建 `src/app/input_manager.cpp`：**

关键操作就是 **剪切-粘贴**：

| 从 `application.cpp` 剪切 | 粘贴到 `input_manager.cpp` | 注意事项 |
|---|---|---|
| `Application::on_mouse_button_pressed()` | `InputManager::on_mouse_button()` | 把 `input_record_.xxx` 改成 `xxx_` |
| `Application::on_scroll()` | `InputManager::on_scroll()` | 一行代码 |
| `Application::on_cursor_pos()` | `InputManager::on_cursor_pos()` | 把 `input_record_.xxx` 改成 `xxx_` |
| `Application::get_pos_delta()` | `InputManager::get_pos_delta()` | `height_` 改成 `screen_height_` |
| `Application::update_click()` | 合并进 `InputManager::poll()` 内部 | 见下方 |

**构造函数（把 `init_window()` 里的回调注册搬过来）：**

```cpp
InputManager::InputManager(Platform::Win32Window& window, int screen_height)
    : window_(window), screen_height_(screen_height)
{
    window_.set_mouse_button_callback([this](auto btn, bool p) {
        on_mouse_button(btn, p);
    });
    window_.set_scroll_callback([this](float off) {
        on_scroll(off);
    });
    window_.set_cursor_pos_callback([this](double x, double y) {
        on_cursor_pos(x, y);
    });
}
```

**`poll()` 方法（核心逻辑）：**

```cpp
InputState InputManager::poll(float current_time) {
    // 1. 点击检测（原 update_click 的逻辑）
    if (release_time_ > 0 && current_time - release_time_ > CLICK_DELAY) {
        Eigen::Vector2f pos_delta = release_pos_ - press_pos_;
        if (pos_delta.norm() < CLICK_DISTANCE_THRESHOLD) {
            single_click_ = true;
        }
        release_time_ = 0;
    }

    if (single_click_ || double_click_) {
        int width = /* 需要宽度，考虑构造时传入或从 window_ 获取 */;
        float click_x = release_pos_.x() / static_cast<float>(width);
        float click_y = release_pos_.y() / static_cast<float>(screen_height_);
        click_pos_ = Eigen::Vector2f(click_x, 1.0f - click_y);
    }

    // 2. 组装只读快照
    InputState state;
    state.orbit_delta  = orbit_delta_;
    state.pan_delta    = pan_delta_;
    state.dolly_delta  = dolly_delta_;
    state.single_click = single_click_;
    state.double_click = double_click_;
    state.click_pos    = click_pos_;

    // 3. 重置每帧增量
    orbit_delta_  = { 0, 0 };
    pan_delta_    = { 0, 0 };
    dolly_delta_  = 0;
    single_click_ = false;
    double_click_ = false;

    return state;
}
```

> **注意**：`poll()` 里需要窗口宽度来计算 `click_pos`。推荐在构造函数里多传一个 `screen_width` 参数。

#### 4c. 在 Application 中使用 InputManager

**application.hpp：**

```cpp
// 删掉整个 InputRecord 结构体
// 删掉所有回调声明：on_key_pressed, on_mouse_button_pressed, on_scroll, on_cursor_pos
// 删掉 get_pos_delta, get_cursor_pos, update_click
// 删掉常量 CLICK_DELAY, CLICK_DISTANCE_THRESHOLD

// 添加：
#include "app/input_manager.hpp"
std::unique_ptr<InputManager> input_;
```

**application.cpp 构造函数：**

```cpp
Application::Application(int width, int height, const std::string& title,
                         std::unique_ptr<Scene> scene)
    : width_(width), height_(height), title_(title), scene_(std::move(scene))
{
    window_ = std::make_unique<Platform::Win32Window>(title_, width_, height_);
    input_ = std::make_unique<InputManager>(*window_, height_);

    camera_ = std::make_shared<TargetCamera>(...);
    framebuffer_ = std::make_unique<Framebuffer>(width_, height_);

    if (scene_) scene_->camera = camera_;

    // 不再需要 init_window() — 回调注册已在 InputManager 构造函数里
}
```

**application.cpp — `tick()`：**

```cpp
void Application::tick() {
    // ...
    InputState input = input_->poll(timer_.current_time());
    // 用 input.orbit_delta 替代原来的 input_record_.orbit_delta
    // 用 input.dolly_delta 替代原来的 input_record_.dolly_delta
    // ...
}
```

> **编译验证** — 鼠标拖拽旋转、平移、缩放行为和之前一模一样
>
> **git commit**: `refactor: extract InputManager from Application`

---

### Step 5：拆出 LightController

**新建 `include/app/light_controller.hpp`：**

```cpp
#pragma once
#include <Eigen/Core>

namespace Platform { class Win32Window; }

class LightController {
public:
    LightController(float initial_theta, float initial_phi, float speed);

    // 根据按键状态更新光照角度
    void update(Platform::Win32Window& window, float delta_time);

    // 重置到初始状态
    void reset();

    // 获取光照方向向量
    Eigen::Vector3f get_direction() const;

private:
    float theta_, phi_;
    float default_theta_, default_phi_;
    float speed_;
};
```

**新建 `src/app/light_controller.cpp`：**

从 Application 搬移的对应关系：

| 从 `application.cpp` 搬走 | 搬到 `LightController` |
|---|---|
| `input_record_.light_theta` / `light_phi` | 成员变量 `theta_` / `phi_` |
| `update_light_direction()` 方法体 | `LightController::update()` |
| 自由函数 `get_light_dir()` | `LightController::get_direction()` |
| 常量 `LIGHT_THETA` / `LIGHT_PHI` / `LIGHT_SPEED` | 构造参数 / `default_xxx_` |

```cpp
#include "app/light_controller.hpp"
#include "platform/window.hpp"
#include <cmath>

// EPSILON 从 utils/global.hpp 获取，或者在此定义局部常量

LightController::LightController(float initial_theta, float initial_phi, float speed)
    : theta_(initial_theta), phi_(initial_phi)
    , default_theta_(initial_theta), default_phi_(initial_phi)
    , speed_(speed) {}

void LightController::update(Platform::Win32Window& window, float delta_time) {
    float angle = speed_ * delta_time;

    if (window.is_key_pressed(Platform::KeyCode::A))
        theta_ -= angle;
    if (window.is_key_pressed(Platform::KeyCode::D))
        theta_ += angle;
    if (window.is_key_pressed(Platform::KeyCode::S))
        phi_ = std::min(phi_ + angle, static_cast<float>(M_PI) - EPSILON);
    if (window.is_key_pressed(Platform::KeyCode::W))
        phi_ = std::max(phi_ - angle, EPSILON);
}

void LightController::reset() {
    theta_ = default_theta_;
    phi_ = default_phi_;
}

Eigen::Vector3f LightController::get_direction() const {
    float x = std::sin(phi_) * std::sin(theta_);
    float y = std::cos(phi_);
    float z = std::sin(phi_) * std::cos(theta_);
    return -Eigen::Vector3f(x, y, z).normalized();
}
```

**修改 Application：**

```cpp
// application.hpp：
//   删掉 update_light_direction() 声明
//   从 InputRecord 里删掉 light_theta, light_phi（如果还没删干净的话）
//   添加：
#include "app/light_controller.hpp"
std::unique_ptr<LightController> light_ctrl_;

// application.cpp 构造函数：
light_ctrl_ = std::make_unique<LightController>(
    LIGHT_THETA, LIGHT_PHI, LIGHT_SPEED);
// 然后删掉 LIGHT_THETA, LIGHT_PHI, LIGHT_SPEED 这三个 static const

// application.cpp — tick()：
light_ctrl_->update(*window_, timer_.delta_time());
if (window_->is_key_pressed(Platform::KeyCode::Space))
    light_ctrl_->reset();

Eigen::Vector3f light_dir = light_ctrl_->get_direction();
// 替代原来的 get_light_dir(input_record_.light_theta, input_record_.light_phi)
```

> **编译验证** — WASD 控制光照方向行为不变
>
> **git commit**: `refactor: extract LightController from Application`

---

### Step 6：拆出 CameraController

**新建 `include/app/camera_controller.hpp`：**

```cpp
#pragma once
#include "geometry/camera.hpp"
#include <memory>

struct InputState; // 前向声明

class CameraController {
public:
    CameraController(std::shared_ptr<TargetCamera> camera,
                     const Eigen::Vector3f& default_pos,
                     const Eigen::Vector3f& default_target);

    // 根据输入更新相机
    void update(const InputState& input, bool reset_requested);

    TargetCamera& camera();
    std::shared_ptr<TargetCamera> camera_ptr();

private:
    std::shared_ptr<TargetCamera> camera_;
    Eigen::Vector3f default_pos_;
    Eigen::Vector3f default_target_;
};
```

**新建 `src/app/camera_controller.cpp`：**

```cpp
#include "app/camera_controller.hpp"
#include "app/input_manager.hpp"  // InputState 的完整定义

CameraController::CameraController(std::shared_ptr<TargetCamera> camera,
                                   const Eigen::Vector3f& default_pos,
                                   const Eigen::Vector3f& default_target)
    : camera_(std::move(camera))
    , default_pos_(default_pos)
    , default_target_(default_target) {}

void CameraController::update(const InputState& input, bool reset_requested) {
    if (reset_requested) {
        camera_->camera_set_transform(default_pos_, default_target_);
        return;
    }
    motion m;
    m.orbit = input.orbit_delta;
    m.pan   = input.pan_delta;
    m.dolly = input.dolly_delta;
    camera_->update_transform(m);
}

TargetCamera& CameraController::camera() { return *camera_; }
std::shared_ptr<TargetCamera> CameraController::camera_ptr() { return camera_; }
```

**修改 Application：**

```cpp
// application.hpp：
//   删掉 update_camera() 声明
//   删掉 std::shared_ptr<TargetCamera> camera_;
//   添加：
#include "app/camera_controller.hpp"
std::unique_ptr<CameraController> camera_ctrl_;

// application.cpp 构造函数：
auto camera = std::make_shared<TargetCamera>(
    CAMERA_POSITION, CAMERA_TARGET,
    static_cast<float>(width_) / height_);
camera_ctrl_ = std::make_unique<CameraController>(
    camera, CAMERA_POSITION, CAMERA_TARGET);
if (scene_) scene_->camera = camera_ctrl_->camera_ptr();

// application.cpp — tick()：
bool reset = window_->is_key_pressed(Platform::KeyCode::Space);
camera_ctrl_->update(input, reset);
// 替代原来的 update_camera()

// 需要用相机的地方：
auto& cam = camera_ctrl_->camera();
scene_->update_per_frame_data(
    ...,
    cam.get_position(),
    ...,
    cam.get_view_matrix(),
    cam.get_projection_matrix(), -1);
```

> **编译验证** — 相机旋转、平移、缩放行为不变
>
> **git commit**: `refactor: extract CameraController from Application`

---

### Step 7：拆出 SceneRenderer

**新建 `include/app/scene_renderer.hpp`：**

```cpp
#pragma once

#include <vector>
#include <memory>
#include <Eigen/Core>

class Scene;
class Framebuffer;
class Model_Base;

class SceneRenderer {
public:
    void render(Scene& scene, Framebuffer& framebuffer);

private:
    void shadow_pass(Scene& scene);
    void main_pass(Scene& scene, Framebuffer& framebuffer);
    void sort_models(std::vector<std::shared_ptr<Model_Base>>& models,
                     const Eigen::Matrix4f& view_matrix);
};
```

**新建 `src/app/scene_renderer.cpp`：**

从 `application.cpp` 搬走 `render_scene()` 和 `sort_models()`。

**重要：清理测试代码！** `application.cpp` 第 321-340 行的 `//test code ... //test end` 整块删掉 — 它和下面 343-370 行的正式逻辑是重复的，会导致模型被画两遍。

整理后的 `main_pass` 参考实现：

```cpp
void SceneRenderer::main_pass(Scene& scene, Framebuffer& fb) {
    sort_models(scene.models, scene.frame_data.camera_view_matrix);
    fb.clear_color(scene.background_color);
    fb.clear_depth(1.0f);

    if (!scene.skybox || scene.frame_data.layer_view >= 0) {
        // 无天空盒：直接渲染所有模型
        for (const auto& model : scene.models) {
            model->draw(&fb, 0);
        }
    } else {
        // 有天空盒：不透明物体 → 天空盒 → 透明物体
        // 找到第一个非不透明模型的位置
        size_t num_opaques = 0;
        for (const auto& model : scene.models) {
            if (model->opaque) num_opaques++;
            else break;
        }

        for (size_t i = 0; i < num_opaques; ++i)
            scene.models[i]->draw(&fb, 0);

        scene.skybox->draw(&fb, 0);

        for (size_t i = num_opaques; i < scene.models.size(); ++i)
            scene.models[i]->draw(&fb, 0);
    }
}
```

**修改 Application：**

```cpp
// application.hpp：
//   删掉 render_scene() 和 sort_models() 声明
//   添加：
#include "app/scene_renderer.hpp"
std::unique_ptr<SceneRenderer> renderer_;

// application.cpp 构造函数：
renderer_ = std::make_unique<SceneRenderer>();

// application.cpp — tick()：
renderer_->render(*scene_, *framebuffer_);
// 替代原来的 render_scene()
```

> **编译验证** — 渲染结果和之前一致（画面正常、阴影正常）
>
> **git commit**: `refactor: extract SceneRenderer from Application`

---

### Step 8：审视最终的 Application

做完所有步骤后，检查 `application.hpp`，它应该只剩：

```cpp
#pragma once

#include <memory>
#include <string>

// 前向声明 — 不再需要 include 一大堆头文件
namespace Platform { class Win32Window; }
class Scene;
class Framebuffer;
class InputManager;
class CameraController;
class LightController;
class SceneRenderer;
class FrameTimer;

class Application {
public:
    Application(int width, int height, const std::string& title,
                std::unique_ptr<Scene> scene);
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void run();

private:
    void tick();

    // 窗口
    std::unique_ptr<Platform::Win32Window> window_;
    int width_, height_;
    std::string title_;

    // 子系统 — 每个只做一件事
    std::unique_ptr<InputManager>     input_;
    std::unique_ptr<CameraController> camera_ctrl_;
    std::unique_ptr<LightController>  light_ctrl_;
    std::unique_ptr<SceneRenderer>    renderer_;
    FrameTimer                        timer_;

    // 数据
    std::unique_ptr<Scene>       scene_;
    std::unique_ptr<Framebuffer> framebuffer_;
};
```

精简后的 `tick()` 只有协调逻辑：

```cpp
void Application::tick() {
    timer_.begin_frame(Platform::Win32Window::get_time());

    // 1. 收集输入
    InputState input = input_->poll(timer_.current_time());
    bool reset = window_->is_key_pressed(Platform::KeyCode::Space);

    // 2. 更新子系统
    camera_ctrl_->update(input, reset);
    light_ctrl_->update(*window_, timer_.delta_time());
    if (reset) light_ctrl_->reset();

    // 3. 更新场景 per-frame 数据
    auto& cam = camera_ctrl_->camera();
    Eigen::Vector3f light_dir = light_ctrl_->get_direction();
    scene_->update_per_frame_data(
        timer_.current_time(), timer_.delta_time(),
        light_dir, cam.get_position(),
        mat4_lookat(-light_dir, Eigen::Vector3f(0,0,0), Eigen::Vector3f(0,1,0)),
        mat4_orthographic(1, 1, 0, 2),
        cam.get_view_matrix(), cam.get_projection_matrix(), -1);

    // 4. 渲染
    renderer_->render(*scene_, *framebuffer_);

    timer_.maybe_print_fps();
}
```

### 拆分前后对比

| 指标 | 拆分前 | 拆分后 |
|------|--------|--------|
| `application.hpp` include 数 | 7 | 0（全部前向声明） |
| `Application` 成员变量 | ~20 | 7 |
| `Application` 方法数 | 14 | 3（`run`, `tick`, 构造/析构） |
| `application.cpp` 行数 | 371 | ~60 |
| 最大单一方法行数 | 78（`render_scene`） | ~20（`tick`） |
| 可独立测试的组件 | 0 | 5 |

如果还有多余的东西，思考一下它属于哪个子系统，继续搬。

---

## 阶段三：建立 UI 占位

### Step 9：创建 UI 层骨架

**新建 `include/ui/ui_context.hpp`：**

```cpp
#pragma once

// UI 系统入口，后续集成 ImGui 时在此扩展
class UIContext {
public:
    void init();      // 未来：初始化 ImGui
    void begin();     // 未来：ImGui::NewFrame()
    void end();       // 未来：ImGui::Render()
    void shutdown();  // 未来：ImGui::DestroyContext()
};
```

**新建 `src/ui/ui_context.cpp`：**

```cpp
#include "ui/ui_context.hpp"

void UIContext::init() {}
void UIContext::begin() {}
void UIContext::end() {}
void UIContext::shutdown() {}
```

现在不用实现，只是**占位**，确保目录存在、CMake 能扫到。

> **git commit**: `chore: add ui/ directory with UIContext placeholder`

---

## 每步完成后的自检清单

每完成一个 Step，用这个清单核查：

- [ ] 编译通过，零新 warning
- [ ] 运行行为和拆分前完全一致
- [ ] `application.hpp` 的 `#include` 数量没有增加（最好减少）
- [ ] 新类不依赖 Application（依赖方向：Application → 新类，不能反过来）
- [ ] 新类可以独立理解（不需要看 Application 的代码就能看懂新类）
- [ ] git commit（每个 Step 一个 commit，方便回退）
