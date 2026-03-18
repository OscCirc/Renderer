# Renderer 项目架构文档

---

## 1. 模块依赖关系

```mermaid
graph TD
    subgraph Platform["🖥️ Platform 层"]
        WIN[Win32Window]
    end

    subgraph Core["⚙️ Core 层"]
        APP[Application]
        SCENE[Scene]
        FB[Framebuffer]
        GFX[graphics_draw_triangle]
        RAST[rasterize_polygon]
    end

    subgraph Geometry["📐 Geometry 层"]
        CAM[Camera]
        MODEL[Model_Base]
        MESH[Mesh]
        SKEL[Skeleton]
    end

    subgraph Shading["🎨 Shading 层"]
        PROG["Program&lt;A,V,U&gt;"]
        BLINN[Blinn Shader]
        SKYBOX[Skybox Shader]
        TEX[Texture]
        CUBE[Cubemap]
    end

    subgraph Utils["🔧 Utils / Math 层"]
        CACHE[ResourceCache]
        MATH[math]
        GLOBAL[global]
        IMG[Image / stb]
    end

    APP --> WIN
    APP --> SCENE
    APP --> FB
    APP --> CAM

    SCENE --> MODEL
    SCENE --> FB
    SCENE --> TEX

    MODEL --> MESH
    MODEL --> PROG
    MODEL --> SKEL

    GFX --> PROG
    GFX --> RAST
    RAST --> FB

    PROG --> BLINN
    PROG --> SKYBOX

    BLINN --> TEX
    SKYBOX --> CUBE
    CUBE --> TEX
    TEX --> IMG

    MODEL --> CACHE
    MESH --> CACHE
    SKEL --> CACHE
    TEX --> CACHE

    CAM --> MATH
    APP --> MATH
```

---

## 2. 类继承与组合关系

```mermaid
classDiagram

    %% ── Core ──────────────────────────────────────────
    class Application {
        -Win32Window window_
        -Scene scene_
        -TargetCamera camera_
        -Framebuffer framebuffer_
        -InputRecord input_record_
        -float delta_time_
        +run()
        -tick()
        -render_scene()
        -sort_models()
        -process_input()
    }

    class Scene {
        +Vector4f background_color
        +Model_Base skybox
        +Model_Base[] models
        +TargetCamera camera
        +perframe frame_data
        +Framebuffer shadow_buffer
        +Texture shadow_map
        +update_per_frame_data()
    }

    class Framebuffer {
        -int width
        -int height
        -uchar[] color_buffer
        -float[] depth_buffer
        +clear_color()
        +clear_depth()
        +get_color(index)
        +set_color(index, color)
        +get_depth(index)
        +set_depth(index, depth)
    }

    class perframe {
        +float frame_time
        +float delta_time
        +Vector3f light_dir
        +Vector3f camera_pos
        +Matrix4f light_view_matrix
        +Matrix4f light_proj_matrix
        +Matrix4f camera_view_matrix
        +Matrix4f camera_proj_matrix
        +float ambient_intensity
        +float punctual_intensity
        +Texture shadow_map
        +int layer_view
    }

    %% ── Geometry ──────────────────────────────────────
    class Camera_Base {
        #Vector3f position
        #Vector3f target
        #float aspect
        +get_view_matrix()
        +get_projection_matrix()
        +get_position()
    }

    class TargetCamera {
        +update_transform(motion)
        +camera_set_transform()
        -calculate_pan()
        -calculate_offset()
    }

    class Camera {
        -Vector3f front
        -float yaw
        -float pitch
        +rotate()
        +move_forward()
        +move_right()
        +move_up()
    }

    class Model_Base {
        <<abstract>>
        +Mesh mesh
        +IProgram program
        +Matrix4f transform
        +Skeleton skeleton
        +int attached
        +int opaque
        +float distance
        +update(perframe)*
        +draw(Framebuffer, pass)*
    }

    class Blinn_Phong_Model {
        +update(perframe)
        +draw(Framebuffer, pass)
    }

    class Skybox_Model {
        +update(perframe)
        +draw(Framebuffer, pass)
    }

    class Mesh {
        -vertex_attribs[] vertices
        -Vector3f center
        +load(filename)$
        +getNumFaces()
        +getVertices()
        +getCenter()
    }

    class vertex_attribs {
        +Vector3f position
        +Vector2f texcoord
        +Vector3f normal
        +Vector4f tangent
        +Vector4f joint
        +Vector4f weight
    }

    class Skeleton {
        -Joint[] joints
        -Matrix4f[] joint_matrices
        -Matrix3f[] normal_matrices
        +update(frame_time)
        +get_joint_matrices()
        +get_normal_matrices()
    }

    class Joint {
        +int parent_index
        +Matrix4f inverse_bind_matrix
        +Matrix4f global_transform
        +get_local_transform(time)
    }

    %% ── Shading ───────────────────────────────────────
    class IProgram {
        <<abstract>>
    }

    class Program~A_V_U~ {
        +vertex_shader vs
        +fragment_shader fs
        +Uniforms uniforms
        +bool is_double_sided
        +bool is_blend_enabled
        +Attribs[3] shader_attribs
        +Vector4f[10] in_coords
        +Vector4f[10] out_coords
        +clip_triangle()
    }

    class blinn_uniforms {
        +Vector3f light_dir
        +Vector3f camera_pos
        +Matrix4f model_matrix
        +Matrix3f normal_matrix
        +Matrix4f light_vp_matrix
        +Matrix4f camera_vp_matrix
        +Matrix4f[] joint_matrices
        +float ambient_intensity
        +float punctual_intensity
        +Texture shadow_map
        +Texture diffuse_map
        +Texture specular_map
        +Texture emission_map
        +float shininess
        +float alpha_cutoff
    }

    class skybox_uniforms {
        +Matrix4f vp_matrix
        +Cubemap skybox
    }

    %% ── Texture ───────────────────────────────────────
    class Texture {
        -int width
        -int height
        -Vector4f[] buffer
        +sample(u, v, mode)
        +update_from_depth_buffer()
        +update_from_color_buffer()
        +is_valid()
    }

    class Cubemap {
        -Texture[6] faces
        +sample(direction, mode)
        -select_face(direction)
    }

    class Image {
        -int width, height, channels
        -uchar[] ldr_buffer
        -float[] hdr_buffer
        +save(filename)
        +flip_h()
        +flip_v()
        +get_ldr_buffer()
        +get_hdr_buffer()
    }

    %% ── Cache ─────────────────────────────────────────
    class MeshCache {
        <<singleton>>
        +acquire(filename)
    }

    class SkeletonCache {
        <<singleton>>
        +acquire(filename)
    }

    class TextureCache {
        <<singleton>>
        +acquire(filename, usage)
    }

    class Cubemap_cache {
        <<singleton>>
        +acquire(name, blur_level)
    }

    %% ── Platform ──────────────────────────────────────
    class Win32Window {
        -HWND hwnd
        -bool should_close
        +poll_events()
        +present_framebuffer()
        +is_key_pressed()
        +get_cursor_position()
        +set_key_callback()
        +set_mouse_button_callback()
        +set_scroll_callback()
        +get_time()$
    }

    %% ── Relationships ─────────────────────────────────

    %% Application
    Application *-- Scene
    Application *-- Framebuffer
    Application *-- TargetCamera
    Application *-- Win32Window

    %% Scene
    Scene *-- "0..*" Model_Base
    Scene *-- perframe
    Scene o-- TargetCamera
    Scene *-- Framebuffer : shadow_buffer
    Scene *-- Texture : shadow_map

    %% Camera
    Camera_Base <|-- TargetCamera
    Camera_Base <|-- Camera

    %% Model
    Model_Base <|-- Blinn_Phong_Model
    Model_Base <|-- Skybox_Model
    Model_Base o-- Mesh
    Model_Base *-- IProgram
    Model_Base o-- Skeleton

    %% Program
    IProgram <|-- Program~A_V_U~
    Blinn_Phong_Model ..> blinn_uniforms : uses
    Skybox_Model ..> skybox_uniforms : uses

    %% Uniforms → Texture
    blinn_uniforms o-- Texture
    skybox_uniforms o-- Cubemap

    %% Mesh / Skeleton
    Mesh *-- "0..*" vertex_attribs
    Skeleton *-- "0..*" Joint

    %% Texture
    Texture ..> Image : loads via
    Cubemap *-- "6" Texture

    %% Cache
    MeshCache ..> Mesh : manages
    SkeletonCache ..> Skeleton : manages
    TextureCache ..> Texture : manages
    Cubemap_cache ..> Cubemap : manages
```

---

## 3. 渲染管线数据流

```mermaid
flowchart TD
    START([Application::run]) --> TICK

    TICK[tick 每帧] --> INPUT[process_input<br/>键鼠输入处理]
    TICK --> UCAM[update_camera<br/>轨道相机更新]
    TICK --> ULIGHT[update_light_direction<br/>光源方向更新]
    TICK --> UPF[Scene::update_per_frame_data<br/>更新 perframe uniform]
    TICK --> RENDER

    subgraph RENDER[render_scene]
        SORT[sort_models<br/>按深度排序不透明/透明]

        subgraph SHADOW[阴影通道 Shadow Pass]
            SCLEAR[shadow_buffer clear_depth]
            SDRAW[Opaque Models draw<br/>shadow_pass = 1]
            SUPDATE[shadow_map update_from_depth_buffer]
            SCLEAR --> SDRAW --> SUPDATE
        end

        subgraph MAIN[主渲染通道 Main Pass]
            MCLEAR[framebuffer clear_color + clear_depth]

            subgraph DRAWLOOP[逐模型绘制]
                UPDATE[Model::update<br/>写入 Uniforms]
                VERTS[Mesh::getVertices<br/>逐三角面]
                GDT["graphics_draw_triangle()"]

                subgraph PIPELINE[着色器管线]
                    VS[Vertex Shader<br/>模型→裁剪空间<br/>骨骼动画蒙皮]
                    CLIP[clip_triangle<br/>Sutherland-Hodgman<br/>7平面裁剪]
                    ASSEMBLE[三角形组装<br/>多边形→三角扇]
                    RAST[rasterize_polygon<br/>扫描线光栅化]

                    subgraph FRAG[逐像素]
                        DT{深度测试<br/>Z-buffer}
                        INTERP[透视校正插值<br/>1/w 权重]
                        FS[Fragment Shader<br/>Blinn-Phong 光照<br/>阴影采样<br/>Alpha 测试]
                        BLEND[Alpha 混合]
                        WRITE[写入 Framebuffer]
                    end

                    VS --> CLIP --> ASSEMBLE --> RAST --> FRAG
                    DT -->|通过| INTERP --> FS --> BLEND --> WRITE
                    DT -->|丢弃| DISCARD([discard])
                end

                UPDATE --> VERTS --> GDT --> PIPELINE
            end

            MCLEAR --> DRAWLOOP
        end

        SORT --> SHADOW --> MAIN
    end

    RENDER --> PRESENT[Win32Window::present_framebuffer<br/>BitBlt 到屏幕]
    PRESENT --> POLL[poll_events<br/>Win32 消息泵]
    POLL --> TICK
```

---

## 4. 资源缓存架构

```mermaid
flowchart LR
    subgraph Loaders["加载请求方"]
        BPM[Blinn_Phong_Model]
        SKY[Skybox_Model]
    end

    subgraph Caches["单例缓存层（weak_ptr）"]
        MC[MeshCache]
        SC[SkeletonCache]
        TC[TextureCache]
        CC[Cubemap_cache]
    end

    subgraph Resources["资源对象（shared_ptr）"]
        MESH[Mesh]
        SKEL[Skeleton]
        TEX[Texture]
        CUBE[Cubemap]
        IMG[Image<br/>stb_image]
    end

    BPM -->|cache_acquire_mesh| MC
    BPM -->|cache_acquire_skeleton| SC
    BPM -->|acquire_color_texture| TC
    SKY -->|acquire_cubemap| CC
    SKY -->|cache_acquire_mesh| MC

    MC -->|hit / miss| MESH
    SC -->|hit / miss| SKEL
    TC -->|hit / miss| TEX
    CC -->|hit / miss| CUBE

    CUBE -->|6 faces| TC
    TEX -->|load| IMG
```

---

## 5. 目录结构速览

```
Renderer/
├── include/
│   ├── core/          ← Application · Scene · Framebuffer · Graphics · Rasterizer
│   ├── geometry/      ← Camera · Model · Mesh · Skeleton · Triangle
│   ├── shading/       ← Shader · Blinn · Skybox · Texture · Light
│   ├── math/          ← mat4_lookat · mat4_orthographic
│   ├── platform/      ← Win32Window
│   └── utils/         ← global · resource_cache · OBJ_Loader
├── src/               ← 对应实现文件（同目录结构）
├── assets/            ← 3D 模型 · 贴图 · 场景描述文件(.scn)
├── TODO.md            ← 技术路线 TODO
└── ARCHITECTURE.md    ← 本文件
```
