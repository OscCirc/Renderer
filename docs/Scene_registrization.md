我们采取了如下方法进行场景注册和管理：

`get_scene_creators`里注册了一个静态`map`，这样`main.cpp`中可以通过一个注册辅助类和其静态声明来实现在`main`执行之前初始化这个`map`。