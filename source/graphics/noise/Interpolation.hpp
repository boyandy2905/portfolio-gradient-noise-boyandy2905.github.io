/**
 * \file
 * \author Sehoon Kim(sehoon.kim@digipen.edu)
 * \date 2026 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#pragma once

namespace graphics::noise
{

    template <typename T>
    struct LinearValues
    {
        T Left{};
        T Right{};
    };

    template <typename T, typename U>
    constexpr auto linear_mix(const LinearValues<T>& values, U s) noexcept
    {
        return values.Left * (static_cast<U>(1) - s) + values.Right * s;
    }

    template <typename T>
    struct BiLinearValues
    {
        LinearValues<T> Bottom{};
        LinearValues<T> Top{};
    };

    template <typename T, typename U>
    constexpr auto bilinear_mix(const BiLinearValues<T>& values, U s, U t) noexcept
    {
        const auto bottom = linear_mix(values.Bottom, s);
        const auto top    = linear_mix(values.Top, s);

        return linear_mix(LinearValues<decltype(bottom)>{ bottom, top }, t);
    }

    template <typename T>
    struct TriLinearValues
    {
        BiLinearValues<T> Near{};
        BiLinearValues<T> Far{};
    };

    template <typename T, typename U>
    constexpr auto trilinear_mix(const TriLinearValues<T>& values, U s, U t, U p) noexcept
    {
        const auto near_value = bilinear_mix(values.Near, s, t);
        const auto far_value  = bilinear_mix(values.Far, s, t);

        return linear_mix(LinearValues<decltype(near_value)>{ near_value, far_value }, p);
    }
}
