# pragma once

#include <cmath>
#include <cstddef> // for size_t
#include <algorithm> // for std::size (C++17)

// 1. 数学常量和精度
constexpr float EPSILON = 1e-6f;
constexpr float PI = 3.14159265359f;

// 2. 缓冲区大小
constexpr size_t LINE_SIZE = 256;
constexpr size_t PATH_SIZE = 256;

// 3. 角度转换 (模板函数替代宏)
template<typename T>
constexpr float to_radians(T degrees) {
    // 使用 180.0f 确保浮点运算
    return degrees * (PI / 180.0f);
}

template<typename T>
constexpr float to_degrees(T radians) {
    return radians * (180.0f / PI);
}


