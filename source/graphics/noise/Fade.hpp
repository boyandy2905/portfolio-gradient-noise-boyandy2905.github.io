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
    enum class SmoothMethod
    {
        Linear,
        Cosine,
        Smoothstep,
        Quintic
    };

    struct Fade2
    {
        float x = 0.0f;
        float y = 0.0f;
    };

    struct Fade3
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    constexpr float fade(float x, SmoothMethod smoothing) noexcept
    {
        constexpr float PI = 3.14159265358979323846f;

        switch (smoothing)
        {
            case SmoothMethod::Linear:
                return x;

            case SmoothMethod::Cosine:
                return (1.0f - std::cos(x * PI)) * 0.5f;

            case SmoothMethod::Smoothstep:
                return x * x * (3.0f - 2.0f * x);

            case SmoothMethod::Quintic:
                return x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f);
        }

        return x;
    }

    constexpr Fade2 fade(float x, float y, SmoothMethod smoothing) noexcept
    {
        return Fade2{ fade(x, smoothing), fade(y, smoothing) };
    }

    constexpr Fade3 fade(float x, float y, float z, SmoothMethod smoothing) noexcept
    {
        return Fade3{ fade(x, smoothing), fade(y, smoothing), fade(z, smoothing) };
    }
}
