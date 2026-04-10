# Dear ImGui 集成指南（软件渲染后端）

> 目标：将 Dear ImGui 集成到你的软件光栅化渲染器中，通过自定义后端把 ImGui 的绘制命令光栅化到你自己的 Framebuffer 上。

---

## 为什么这件事有价值

通常 ImGui 的后端是 OpenGL / DirectX / Vulkan，但你是**软件渲染器**，没有 GPU API。你需要自己写一个**软件光栅化后端**——这意味着：

- 你要手动处理 ImGui 输出的顶点/索引缓冲
- 你要自己实现带纹理的 2D 三角形光栅化
- 你要自己处理裁剪矩形（scissor rect）
- 你要把 ImGui 的字体图集（font atlas）变成你自己的 Texture

这本身就是对你渲染管线的一次实战检验。

---

## 整体架构

```
┌──────────────────────────────────────────────┐
│                Application                    │
│                                              │
│  tick() {                                    │
│      input → camera/light/scene              │
│      renderer.render(scene, framebuffer)     │  ← 3D 场景渲染到 framebuffer
│      ui.begin()                              │
│          // 各种 ImGui 面板                    │
│      ui.end(framebuffer)                     │  ← ImGui 叠加渲染到同一个 framebuffer
│      window.present(framebuffer)             │  ← 最终显示
│  }                                           │
└──────────────────────────────────────────────┘
```

关键：ImGui **叠在** 3D 渲染结果之上，共享同一个 Framebuffer。

---

## 文件组织

```
include/ui/
├── imgui_context.hpp          ← ImGui 生命周期管理 + 面板绘制
├── imgui_backend_software.hpp ← 自定义软件光栅化后端（核心难点）
└── panels/                    ← 各功能面板（后续按需添加）
    ├── scene_panel.hpp
    ├── stats_panel.hpp
    └── ...

src/ui/
├── imgui_context.cpp
├── imgui_backend_software.cpp
└── panels/
    ├── scene_panel.cpp
    ├── stats_panel.cpp
    └── ...

third_party/
└── imgui/                     ← Dear ImGui 源码（直接编译进项目）
    ├── imgui.h
    ├── imgui.cpp
    ├── imgui_draw.cpp
    ├── imgui_tables.cpp
    ├── imgui_widgets.cpp
    └── imgui_demo.cpp         ← 可选，开发阶段有用
```

---

## 阶段一：引入 ImGui 源码 + 搭建骨架

### Step 1：获取 ImGui

```bash
# 在项目根目录
mkdir -p third_party
cd third_party
git clone https://github.com/ocornut/imgui.git --branch master --depth 1
```

或者手动下载，只需要这几个文件：

```
imgui.h / imgui.cpp
imgui_draw.cpp
imgui_tables.cpp
imgui_widgets.cpp
imgui_internal.h
imconfig.h
imstb_rectpack.h
imstb_textedit.h
imstb_truetype.h
```

### Step 2：修改 CMakeLists.txt

```cmake
# --- ImGui Source Files ---
set(IMGUI_DIR ${PROJECT_SOURCE_DIR}/third_party/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp     # 开发阶段保留，release 时可去掉
)

# 把 ImGui 源码加入主目标
target_sources(SoftRenderer PRIVATE ${IMGUI_SOURCES})
target_include_directories(SoftRenderer PRIVATE ${IMGUI_DIR})
```

> **编译验证** — 加上 ImGui 源码后编译通过（此时还没调用任何 ImGui 函数）

### Step 3：创建 ImGuiContext 骨架

**`include/ui/imgui_context.hpp`：**

```cpp
#pragma once

class Framebuffer;
namespace Platform { class Win32Window; }

class ImGuiContext {
public:
    void init(Platform::Win32Window& window);
    void shutdown();

    // 每帧调用
    void begin_frame(float delta_time);                 // ImGui::NewFrame()
    void end_frame_and_render(Framebuffer& target);     // ImGui::Render() + 光栅化

    // 输入转发（从 window 事件转给 ImGui IO）
    void on_key(int key, bool pressed);
    void on_mouse_button(int button, bool pressed);
    void on_mouse_pos(float x, float y);
    void on_scroll(float offset);
    void on_char(unsigned int c);

private:
    bool initialized_ = false;
};
```

**`src/ui/imgui_context.cpp`** — 先写空实现，确保编译通过。

---

## 阶段二：实现软件光栅化后端（核心难点）

这是整个集成中最有技术价值的部分。

### ImGui 的渲染流程

```
ImGui::Render()
    │
    ▼
ImDrawData* draw_data = ImGui::GetDrawData()
    │
    ├── draw_data->CmdListsCount   ← 有多少个绘制列表
    │
    └── 每个 ImDrawList:
        ├── VtxBuffer[]   ← 顶点数组 (pos, uv, color)
        ├── IdxBuffer[]   ← 索引数组 (unsigned short)
        └── CmdBuffer[]   ← 绘制命令
            ├── ElemCount      ← 这条命令用多少个索引
            ├── ClipRect       ← 裁剪矩形 (x1, y1, x2, y2)
            └── TextureId      ← 纹理指针（通常是字体图集）
```

### 你需要实现的核心函数

**`include/ui/imgui_backend_software.hpp`：**

```cpp
#pragma once

class Framebuffer;
struct ImDrawData;

namespace ImGuiBackend {

    // 初始化：创建字体纹理
    void init();

    // 关闭：释放字体纹理
    void shutdown();

    // 核心：把 ImGui 的绘制数据光栅化到 framebuffer
    void render_draw_data(ImDrawData* draw_data, Framebuffer& target);

}
```

**`src/ui/imgui_backend_software.cpp` — 实现要点：**

#### 2a. 字体图集初始化

```cpp
#include "imgui.h"

// ImGui 会生成一张字体位图（Font Atlas）
// 你需要把它转成你自己的 Texture 格式

static std::unique_ptr<Texture> g_font_texture;

void ImGuiBackend::init() {
    ImGuiIO& io = ImGui::GetIO();

    // 获取字体图集的像素数据
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    // 创建你自己的 Texture 对象
    g_font_texture = std::make_unique<Texture>(width, height);

    // 把像素数据填入你的 Texture
    // pixels 是 RGBA 格式，每像素 4 字节
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int i = (y * width + x) * 4;
            float r = pixels[i + 0] / 255.0f;
            float g = pixels[i + 1] / 255.0f;
            float b = pixels[i + 2] / 255.0f;
            float a = pixels[i + 3] / 255.0f;
            // 写入你的 texture buffer...
        }
    }

    // 告诉 ImGui 用这个纹理
    io.Fonts->SetTexID(static_cast<ImTextureID>(g_font_texture.get()));
}
```

#### 2b. 核心光栅化循环

```cpp
void ImGuiBackend::render_draw_data(ImDrawData* draw_data, Framebuffer& fb) {
    if (!draw_data) return;

    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;
        const ImDrawIdx*  idx_buffer = cmd_list->IdxBuffer.Data;

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd& pcmd = cmd_list->CmdBuffer[cmd_i];

            // 用户回调（罕见，可先跳过）
            if (pcmd.UserCallback) {
                pcmd.UserCallback(cmd_list, &pcmd);
                continue;
            }

            // 裁剪矩形
            int clip_x1 = static_cast<int>(pcmd.ClipRect.x);
            int clip_y1 = static_cast<int>(pcmd.ClipRect.y);
            int clip_x2 = static_cast<int>(pcmd.ClipRect.z);
            int clip_y2 = static_cast<int>(pcmd.ClipRect.w);

            // 获取纹理（通常是字体图集）
            Texture* texture = static_cast<Texture*>(pcmd.TextureId);

            // 逐三角形光栅化
            for (unsigned int i = 0; i < pcmd.ElemCount; i += 3) {
                const ImDrawVert& v0 = vtx_buffer[idx_buffer[pcmd.IdxOffset + i + 0]];
                const ImDrawVert& v1 = vtx_buffer[idx_buffer[pcmd.IdxOffset + i + 1]];
                const ImDrawVert& v2 = vtx_buffer[idx_buffer[pcmd.IdxOffset + i + 2]];

                // v0.pos  → 屏幕坐标 (float x, y)
                // v0.uv   → 纹理坐标 (float u, v)
                // v0.col  → 顶点颜色 (ImU32, ABGR packed)

                rasterize_imgui_triangle(fb, texture,
                    v0, v1, v2,
                    clip_x1, clip_y1, clip_x2, clip_y2);
            }
        }
    }
}
```

#### 2c. 2D 三角形光栅化器

这是你要自己写的核心函数。和你已有的 3D 光栅化器不同，这个更简单：

- **没有深度测试**（UI 总是在最前面）
- **没有透视校正**（已经是屏幕空间坐标）
- **需要 Alpha 混合**（文字、半透明面板）
- **需要裁剪矩形**（ImGui 用 scissor rect 做面板裁剪）

```
对每个三角形:
  1. 计算 bounding box，与 clip rect 取交集
  2. 对 bounding box 内每个像素：
     a. 计算重心坐标 (w0, w1, w2)
     b. 如果不在三角形内 → 跳过
     c. 插值 uv 和 color
     d. 采样纹理 → texel
     e. 最终颜色 = texel * vertex_color（ImGui 顶点颜色包含 alpha）
     f. Alpha 混合写入 framebuffer
```

Alpha 混合公式：

```
out.rgb = src.rgb * src.a + dst.rgb * (1 - src.a)
out.a   = src.a + dst.a * (1 - src.a)
```

> **提示**：你可以复用已有 `rasterizer.cpp` 的扫描线逻辑作为参考，但建议为 ImGui 单独写一个简化版本，避免污染 3D 渲染管线。

---

## 阶段三：输入系统对接

ImGui 需要知道键鼠状态。你需要在 Win32 消息处理中把事件同时转发给 ImGui。

### 修改思路

**不要修改 `window.cpp`。** 而是在 `ImGuiContext` 层面拦截：

```cpp
// imgui_context.cpp

void ImGuiContext::begin_frame(float delta_time) {
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = delta_time;
    io.DisplaySize = ImVec2(width, height);
    ImGui::NewFrame();
}

void ImGuiContext::on_mouse_pos(float x, float y) {
    ImGui::GetIO().AddMousePosEvent(x, y);
}

void ImGuiContext::on_mouse_button(int button, bool pressed) {
    ImGui::GetIO().AddMouseButtonEvent(button, pressed);
}

void ImGuiContext::on_scroll(float offset) {
    ImGui::GetIO().AddMouseWheelEvent(0, offset);
}

void ImGuiContext::on_key(int key, bool pressed) {
    // 映射你的 KeyCode → ImGuiKey
    ImGuiKey imgui_key = map_keycode(key);
    ImGui::GetIO().AddKeyEvent(imgui_key, pressed);
}

void ImGuiContext::on_char(unsigned int c) {
    ImGui::GetIO().AddInputCharacter(c);
}
```

### 关键：输入消费判断

ImGui 会告诉你它是否要"吃掉"输入事件：

```cpp
// 在 Application::tick() 中
InputState input = input_->poll(timer_.current_time());

// 如果 ImGui 正在处理鼠标（比如拖动滑条），不要让 3D 相机响应
ImGuiIO& io = ImGui::GetIO();
if (io.WantCaptureMouse) {
    input.orbit_delta = {0, 0};
    input.pan_delta = {0, 0};
    input.dolly_delta = 0;
}
if (io.WantCaptureKeyboard) {
    // 屏蔽键盘输入对光照/相机的影响
}
```

这样就能避免"拖动 UI 滑条时场景跟着转"的问题。

---

## 阶段四：实用面板设计

UI 集成完成后，按照你 TODO 路线图的需求，建议实现以下面板：

### 面板 1：渲染统计 Stats Panel（最先做）

```
┌─ Stats ──────────────────┐
│ FPS: 24.3                │
│ Frame: 41.2 ms           │
│ Triangles: 12,847        │
│ Draw Calls: 6            │
│ Resolution: 800 x 600    │
└──────────────────────────┘
```

替代现在控制台的 `std::cout << "fps: ..."` 打印。

实现要点：
- 从 `FrameTimer` 读取 fps / delta_time
- 从 `Scene` 统计模型数和三角面数
- 纯只读，不需要任何新的数据结构

### 面板 2：场景切换 Scene Panel

```
┌─ Scene ──────────────────┐
│ ● azura                  │
│ ○ centaur                │
│ ○ craftsman              │
│ ○ elfgirl                │
│ ...                      │
│ [Load Scene]             │
└──────────────────────────┘
```

替代命令行参数切换场景。

实现要点：
- 从 `get_available_scenes()` 获取场景列表
- 选中后调用场景工厂创建新 Scene
- 需要让 Application 支持运行时替换 Scene（目前只有构造时传入）

### 面板 3：渲染设置 Settings Panel

```
┌─ Settings ───────────────┐
│ Sample Mode: [Bilinear▾] │
│ Light θ: [====●===] 0.78 │
│ Light φ: [===●====] 0.78 │
│ Ambient:  [=●======] 0.2 │
│ Punctual: [=====●==] 0.8 │
│ □ Show Wireframe         │
│ □ Show Normals           │
│ □ Shadow Map View        │
└──────────────────────────┘
```

替代键盘快捷键（1/2/3 切换采样模式、WASD 控制光照）。

实现要点：
- `SampleMode` 从全局变量改为通过 UI 控制
- 光照角度直接用 `ImGui::SliderAngle`
- 调试开关控制 `layer_view`

### 面板 4：材质编辑器 Material Panel（后期 PBR 阶段再做）

```
┌─ Material ───────────────┐
│ Model: [body         ▾]  │
│ Albedo:    ■ (1,0.8,0.6) │
│ Metallic:  [===●====] 0.5│
│ Roughness: [=====●==] 0.8│
│ [Diffuse Map]  ✓ loaded  │
│ [Normal Map]   ✗ none    │
└──────────────────────────┘
```

等 PBR 材质系统（TODO 第三阶段）完成后再做。

### 建议的面板实现顺序

```
① Stats Panel        ← 最简单，替代 console 输出，立即可用
② Settings Panel     ← 替代键盘快捷键，消除 g_sample_mode 全局变量
③ Scene Panel        ← 运行时切换场景，需要小幅重构 Application
④ Material Panel     ← 等 PBR 做完后再加
```

---

## 阶段五：需要配合修改的现有代码

集成 ImGui 后，有些现有代码需要调整，这里提前列出来：

### 5a. 消除 g_sample_mode 全局变量

```cpp
// 现状（global.hpp 里的全局变量）：
// SampleMode g_sample_mode;                    ← 删掉

// 改为：在 Settings Panel 中通过 ImGui::Combo 选择
// 选择结果存储在某个 RenderSettings 结构体中
struct RenderSettings {
    SampleMode sample_mode = SampleMode::Bilinear;
    bool show_wireframe = false;
    bool show_normals = false;
    int  debug_view = -1;   // layer_view
};

// Application 持有这个结构体，每帧传递给需要的子系统
```

### 5b. 支持运行时场景切换

```cpp
// 现状：Scene 在 main() 里创建，一次性传入 Application 构造函数

// 改为：Application 提供 load_scene() 方法
void Application::load_scene(const std::string& name) {
    auto& creators = get_scene_creators();
    auto it = creators.find(name);
    if (it != creators.end()) {
        scene_ = it->second();
        if (scene_) {
            scene_->camera = camera_ctrl_->camera_ptr();
        }
    }
}
```

### 5c. Framebuffer 需要支持 Alpha 混合写入

当前 `set_color()` 是直接覆盖写入。ImGui 渲染需要 Alpha 混合。

两种做法：
- **方案 A**：在 ImGui 后端的光栅化函数内自己做混合（推荐，不侵入 Framebuffer）
- **方案 B**：给 Framebuffer 加 `blend_color()` 方法

推荐方案 A：

```cpp
// 在 imgui_backend_software.cpp 内部
void write_pixel_blended(Framebuffer& fb, int x, int y,
                          float r, float g, float b, float a) {
    // 读取当前颜色
    auto dst = fb.get_color(x, y);      // 假设返回 (r,g,b,a)

    // Alpha 混合
    float out_r = r * a + dst.r * (1 - a);
    float out_g = g * a + dst.g * (1 - a);
    float out_b = b * a + dst.b * (1 - a);
    float out_a = a + dst.a * (1 - a);

    fb.set_color(x, y, out_r, out_g, out_b, out_a);
}
```

### 5d. 输入回调需要分流

```
Win32Window 消息
    ├──→ ImGuiContext（更新 ImGui IO）
    └──→ InputManager（更新 InputState）

每帧检查 io.WantCaptureMouse / io.WantCaptureKeyboard
    → 决定是否屏蔽 3D 交互
```

需要确保 ImGui 先收到事件、再判断是否消费。建议的回调注册顺序：

```cpp
// Application 构造函数中
// 1. 先创建 ImGuiContext 并注册回调
ui_ = std::make_unique<ImGuiContext>();
ui_->init(*window_);

// 2. 再创建 InputManager（它也注册回调）
// 注意：Win32Window 的回调是 std::function，后注册会覆盖前一个
// 所以需要修改为支持多个监听者，或者改成一个统一的分发器
```

**这里有个设计决策**：当前 `Win32Window` 每种事件只支持一个 callback。你有两个选择：

```
选项 A（简单）：让 InputManager 同时转发给 ImGui
    - InputManager 持有 ImGuiContext 的引用
    - 在自己的回调中先调 imgui_context->on_xxx()，再处理自己的逻辑

选项 B（干净）：在 Win32Window 中支持多监听者
    - callback 改为 std::vector<std::function<...>>
    - 或者改为事件队列模式
```

**建议先用选项 A 快速跑通，后续有需要再改为选项 B。**

---

## 实施总路线

```
Phase 1 — 骨架
    Step 1: 引入 ImGui 源码，CMake 编译通过
    Step 2: 创建 ImGuiContext 空壳，调用 ImGui::CreateContext()
    Step 3: 字体图集 → 你的 Texture 格式

Phase 2 — 后端核心
    Step 4: 实现 2D 三角形光栅化器（不带纹理，先画纯色）
    Step 5: 加上纹理采样（字体渲染）
    Step 6: 加上 Alpha 混合
    Step 7: 加上裁剪矩形

Phase 3 — 输入对接
    Step 8: 鼠标事件转发给 ImGui
    Step 9: 键盘事件转发给 ImGui
    Step 10: WantCaptureMouse / WantCaptureKeyboard 判断

Phase 4 — 面板
    Step 11: ImGui::ShowDemoWindow() 验证一切工作正常
    Step 12: Stats Panel（替代 console fps 输出）
    Step 13: Settings Panel（替代键盘快捷键）
    Step 14: Scene Panel（运行时切换场景）
```

### 每步预期耗时

| Step | 内容 | 难度 | 预期耗时 |
|------|------|------|---------|
| 1-3 | 引入 ImGui + 字体图集 | 简单 | 1-2 小时 |
| 4-7 | 2D 光栅化后端 | **核心难点** | 4-8 小时 |
| 8-10 | 输入对接 | 中等 | 2-3 小时 |
| 11 | Demo Window 验证 | 简单 | 30 分钟 |
| 12-14 | 各功能面板 | 简单 | 每个 1-2 小时 |

**总计约 2-3 天集中开发。** Step 4-7 是最难的部分，但也是学习收获最大的地方。

---

## 自检清单

每完成一个 Phase，检查：

- [ ] 编译通过，零新 warning
- [ ] 3D 渲染结果不受 ImGui 集成影响（没有 UI 面板时画面不变）
- [ ] ImGui 面板正确叠加在 3D 场景上
- [ ] 半透明面板能看到背后的 3D 场景
- [ ] 文字清晰可读（字体图集采样正确）
- [ ] 鼠标拖动 UI 面板时场景不会跟着转动
- [ ] UI 面板外的区域仍然可以正常操控场景
- [ ] 新代码全部在 `ui/` 目录和 `third_party/imgui/` 内，没有侵入其他模块
