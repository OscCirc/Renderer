# Renderer 技术路线 TODO List

> 在每条任务前的 `[ ]` 改为 `[x]` 标记完成状态

---

## 🟢 第一阶段：基础质量提升

- [x] **双线性插值**
  - 修改 `src/shading/texture.cpp` 的采样函数
  - 解决近距离纹理像素块问题

- [x] **Mipmap**
  - 修改 `include/shading/texture.hpp` 和 `src/shading/texture.cpp`
  - 解决远处纹理摩尔纹和闪烁问题
  - 依赖：建议先完成双线性插值

- [ ] **法线贴图（Normal Mapping）**
  - 修改 `include/shading/Blinn_shader.hpp` 和 `src/shading/Blinn_shader.cpp`
  - 修改 `include/geometry/mesh.hpp` 和 `src/geometry/mesh.cpp`
  - 实现 TBN 矩阵变换，切线空间光照计算

---

## 🟡 第二阶段：抗锯齿

- [ ] **SSAA（超采样抗锯齿）**
  - 修改 `include/core/framebuffer.hpp` 和 `src/core/framebuffer.cpp`
  - 修改 `src/core/application.cpp`
  - 以 2x/4x 分辨率渲染后 box filter 降采样输出

- [ ] **MSAA（多重采样抗锯齿）**
  - 修改 `src/core/rasterizer.cpp`
  - 修改 `include/core/framebuffer.hpp`
  - 光栅化阶段记录多个覆盖样本点，按覆盖率混合着色结果
  - 依赖：建议先完成 SSAA 理解原理

- [ ] **FXAA（快速近似抗锯齿）**
  - 新增 `src/core/post_process.cpp`
  - 后处理阶段边缘检测 + 模糊混合

---

## 🔴 第三阶段：PBR 物理渲染

- [ ] **PBR 材质系统（Cook-Torrance BRDF）**
  - 新增 `include/shading/pbr_shader.hpp` 和 `src/shading/pbr_shader.cpp`
  - 修改 `include/geometry/model.hpp`（新增 PBR 材质类型）
  - 实现 GGX 法线分布函数（D）
  - 实现 Schlick 菲涅耳近似（F）
  - 实现 Smith 几何遮蔽函数（G）
  - 支持 Albedo / Metallic / Roughness / AO 材质参数

- [ ] **IBL（基于图像的光照）**
  - 新增 `src/core/ibl_precompute.cpp`
  - 修改 `src/shading/pbr_shader.cpp`
  - 预计算漫反射辐照度图
  - 预计算镜面反射预滤波图
  - 预计算 BRDF LUT
  - 依赖：先完成 PBR 材质系统

---

## 🟣 第四阶段：高级效果

- [ ] **PCF 软阴影**
  - 修改阴影采样逻辑
  - 对 Shadow Map 周围多个样本取均值，软化阴影边缘

- [ ] **PCSS（百分比近似软阴影）**
  - 在 PCF 基础上，根据遮挡距离动态调整滤波核大小
  - 模拟真实半影效果
  - 依赖：先完成 PCF

- [ ] **HDR & Tone Mapping**
  - 颜色缓冲改为 float（HDR）
  - 实现 Reinhard 色调映射
  - 实现 ACES Filmic 色调映射

- [ ] **SSAO（屏幕空间环境光遮蔽）**
  - 新增后处理 pass
  - 屏幕空间深度采样估算环境光遮蔽
  - 缝隙、凹陷处产生自然变暗效果

---

## 🟤 第五阶段：光线追踪与全局光照 (Ray Tracing)

- [ ] **基础光线追踪**
  - 实现光线生成（Ray Generation）
  - 实现光线与三角形/包围盒的相交检测算法（例如 Möller–Trumbore）

- [ ] **空间加速结构 (BVH)**
  - 实现层次包围盒（Bounding Volume Hierarchy）加速光线求交
  - 优化树的构建（例如 SAH 表面积启发式算法）

- [ ] **基于物理的路径追踪 (Path Tracing)**
  - 根据渲染方程实现蒙特卡洛光线追踪
  - 实现真正的多次反射全局光照（Global Illumination）
  - 实现光源采样（Next Event Estimation）减少噪点

---

## ⚪ 第六阶段：系统框架与优化

- [ ] **骨骼动画系统**
  - 解析 `.ani` 等动画文件
  - 实现骨骼层级变换与双线性权重的顶点蒙皮（Skinning）计算

- [ ] **渲染性能优化**
  - CPU 多线程渲染（Tile-based 多线程光栅化或光线追踪）
  - 视锥体剔除 (Frustum Culling) 减少多余的 Draw Call 计算

---

## 进度总览

| 阶段 | 任务数 | 已完成 |
|------|--------|--------|
| 🟢 基础质量提升 | 3 | 0 |
| 🟡 抗锯齿 | 3 | 0 |
| 🔴 PBR 物理渲染 | 2 | 0 |
| 🟣 高级效果 | 4 | 0 |
| 🟤 光线追踪与全局光照 | 3 | 0 |
| ⚪ 系统框架与优化 | 2 | 0 |
| **合计** | **17** | **0** |
