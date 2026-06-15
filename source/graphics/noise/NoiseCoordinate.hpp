/**
 * \file
 * \author Sehoon Kim(sehoon.kim@digipen.edu)
 * \date 2026 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include <cmath>

namespace graphics::noise
{
    struct [[nodiscard]] NoiseCoordinate
    {
        int   base        = 0;
        int   next        = 0;
        float interpolant = 0.0f;
    };

    constexpr NoiseCoordinate make_noise_coord(float input) noexcept
    {
        const auto base_float = std::floor(input);
        const int  base       = static_cast<int>(base_float);
        const int  next       = base + 1;
        const auto interpolant = input - static_cast<float>(base);

        return NoiseCoordinate{ base, next, interpolant };
    }

}
