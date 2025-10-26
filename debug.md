

# 初始图像渲染错误

- 禁用所有回调、相机和光照更新 (Application::tick())
- 禁用阴影通道(Application::render_scene())
- 仅渲染一个物体测试(Application::render_scene())

检查windows文件是否出错：手动设置Framebuffer并修改Application::run函数，添加Framebuffer测试函数test()
    未发现问题

Camera类检查
    copilot未发现问题

检查Application::render_scene函数
    检查Blinn_Phong_Model::draw函数
        检查graphics_draw_triangle函数，禁用裁剪，尝试仅渲染一个三角形
            检查rasterize_triangle函数，禁用背面剔除

发现问题：
- rasterize_triangle函数参数varyings各点深度值错误
    溯源到graphics_draw_triangle函数
        检查vertex_shader结果：经过vertex_shader后,in_varyings深度值错误；输入的shader_attribs未发现问题；
            发现uniforms的light_vp矩阵错误。已修复
    interpolate_varyings结果错误
        检查输入参数：varyings无错；weights无错；recip_w无错。确定是函数体出错。
        检查函数体：发现blinn_varyings结构体运算符重载错误。已修复

- draw_fragment函数color计算错误
    检查输入参数，均无误，确认函数体问题。
    检查函数体
        检查fragment_shader
            检查get_material，确定sample函数存在问题
                检查sample函数体，确定是v_image取值使用了错误的翻转。