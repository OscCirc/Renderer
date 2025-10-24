#include "math/math.hpp"

float float_clamp(float f, float min, float max) {
    return f < min ? min : (f > max ? max : f);
}