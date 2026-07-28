#pragma once

#include <Eigen/Core>

namespace ToneMapping
{
    inline Eigen::Vector3f reinhard(const Eigen::Vector3f& linear_hdr)
    {
        const Eigen::Vector3f non_negative =
            linear_hdr.cwiseMax(0.0f);

        return non_negative.cwiseQuotient(
            non_negative + Eigen::Vector3f::Ones());
    }
}