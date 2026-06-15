/**
 * \file
 * \author Rudy Castan
 * \date 2024 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#pragma once

namespace graphics::noise
{
    enum class PeriodDimension : int
    {
        _2     = 1 << 1,  // 2^1 = 2
        _4     = 1 << 2,  // 2^2 = 4
        _8     = 1 << 3,  // 2^3 = 8
        _16    = 1 << 4,  // 2^4 = 16
        _32    = 1 << 5,  // 2^5 = 32
        _64    = 1 << 6,  // 2^6 = 64
        _128   = 1 << 7,  // 2^7 = 128
        _256   = 1 << 8,  // 2^8 = 256
        _512   = 1 << 9,  // 2^9 = 512
        _1024  = 1 << 10, // 2^10 = 1024
        _2048  = 1 << 11, // 2^11 = 2048
        _4096  = 1 << 12, // 2^12 = 4096
        _8192  = 1 << 13, // 2^13 = 8192
        _16384 = 1 << 14, // 2^14 = 16384

    };

    [[nodiscard]] constexpr int period_dimension_mask(PeriodDimension period_dimension) noexcept
    {
        return static_cast<int>(period_dimension) - 1;
    }
}
