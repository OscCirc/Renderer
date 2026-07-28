#pragma once

#include <algorithm>
#include <cmath>

namespace ColorSpace
{
    inline float srgb_to_linear(float c){
        return (c <= 0.04045f) ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
    }

    inline float linear_to_srgb(float c)
    {
        c = std::max(c, 0.0f);
        return c <= 0.0031308f
            ? 12.92f * c
            : 1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f;
    }
}