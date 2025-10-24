#pragma once

#include <eigen3/Eigen/Eigen>
#include "texture.hpp"


template <typename A, typename V, typename U>
class Program;

template <typename A, typename V, typename U>
using vertex_shader_t = Eigen::Vector4f (*)(const A &attribs, V &varyings, U &uniforms);

template <typename V, typename U>
using fragment_shader_t = Eigen::Vector4f (*)(const V &varyings, U &uniforms, bool *discard, bool backface);
