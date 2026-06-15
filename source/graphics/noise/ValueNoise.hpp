/**
 * \file
 * \author Sehoon Kim(sehoon.kim@digipen.edu)
 * \date 2024 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include "Fade.hpp"
#include "Interpolation.hpp"
#include "NoiseCoordinate.hpp"
#include "PeriodDimension.hpp"
#include "PermutationHash.hpp"

#include "util/Random.hpp"

#include <span>
#include <vector>

namespace graphics::noise
{
    template <typename T>
    class [[nodiscard]] ValueNoise
    {
    public:
        explicit ValueNoise(PeriodDimension period = PeriodDimension::_256, SmoothMethod smooth_method = SmoothMethod::Quintic);

        [[nodiscard]] T Evaluate(float x) const noexcept;
        [[nodiscard]] T Evaluate(float x, float y) const noexcept;
        [[nodiscard]] T Evaluate(float x, float y, float z) const noexcept;

        [[nodiscard]] constexpr PeriodDimension GetPeriodDimension() const noexcept;
        void                                    SetPeriod(PeriodDimension period);

        [[nodiscard]] constexpr SmoothMethod GetSmoothing() const noexcept;
        constexpr void                       SetSmoothing(SmoothMethod smooth_method);

        [[nodiscard]] std::span<T> GetValues() noexcept;

    private:
        PeriodDimension period_dimension = PeriodDimension::_256;
        SmoothMethod    smoothing        = SmoothMethod::Quintic;
        PermutationHash hash{};
        std::vector<T>  values{};
    };

    template <typename T>
    ValueNoise<T>::ValueNoise(PeriodDimension period, SmoothMethod smooth_method)
    {
        smoothing = smooth_method;
        SetPeriod(period);
    }

    template <typename T>
    T ValueNoise<T>::Evaluate(float x) const noexcept
    {
        const auto x_coord = make_noise_coord(x);
        const auto s       = fade(x_coord.interpolant, smoothing);

        const LinearValues<T> mixed_values{
            values[static_cast<std::size_t>(hash(x_coord.base))],
            values[static_cast<std::size_t>(hash(x_coord.next))]
        };

        return linear_mix(mixed_values, s);
    }

    template <typename T>
    T ValueNoise<T>::Evaluate(float x, float y) const noexcept
    {
        const auto x_coord = make_noise_coord(x);
        const auto y_coord = make_noise_coord(y);
        const auto faded   = fade(x_coord.interpolant, y_coord.interpolant, smoothing);

        const BiLinearValues<T> mixed_values{
            LinearValues<T>{
                values[static_cast<std::size_t>(hash(x_coord.base, y_coord.base))],
                values[static_cast<std::size_t>(hash(x_coord.next, y_coord.base))]
            },
            LinearValues<T>{
                values[static_cast<std::size_t>(hash(x_coord.base, y_coord.next))],
                values[static_cast<std::size_t>(hash(x_coord.next, y_coord.next))]
            }
        };

        return bilinear_mix(mixed_values, faded.x, faded.y);
    }

    template <typename T>
    T ValueNoise<T>::Evaluate(float x, float y, float z) const noexcept
    {
        const auto x_coord = make_noise_coord(x);
        const auto y_coord = make_noise_coord(y);
        const auto z_coord = make_noise_coord(z);
        const auto faded   = fade(x_coord.interpolant, y_coord.interpolant, z_coord.interpolant, smoothing);

        const TriLinearValues<T> mixed_values{
            BiLinearValues<T>{
                LinearValues<T>{
                    values[static_cast<std::size_t>(hash(x_coord.base, y_coord.base, z_coord.base))],
                    values[static_cast<std::size_t>(hash(x_coord.next, y_coord.base, z_coord.base))]
                },
                LinearValues<T>{
                    values[static_cast<std::size_t>(hash(x_coord.base, y_coord.next, z_coord.base))],
                    values[static_cast<std::size_t>(hash(x_coord.next, y_coord.next, z_coord.base))]
                }
            },
            BiLinearValues<T>{
                LinearValues<T>{
                    values[static_cast<std::size_t>(hash(x_coord.base, y_coord.base, z_coord.next))],
                    values[static_cast<std::size_t>(hash(x_coord.next, y_coord.base, z_coord.next))]
                },
                LinearValues<T>{
                    values[static_cast<std::size_t>(hash(x_coord.base, y_coord.next, z_coord.next))],
                    values[static_cast<std::size_t>(hash(x_coord.next, y_coord.next, z_coord.next))]
                }
            }
        };

        return trilinear_mix(mixed_values, faded.x, faded.y, faded.z);
    }

    template <typename T>
    constexpr PeriodDimension ValueNoise<T>::GetPeriodDimension() const noexcept
    {
        return period_dimension;
    }

    template <typename T>
    void ValueNoise<T>::SetPeriod(PeriodDimension period)
    {
        period_dimension = period;
        hash             = PermutationHash{ period_dimension };

        const auto period_size = static_cast<std::size_t>(period_dimension);
        values.resize(period_size);

        for (auto& value : values)
        {
            value = T{ util::random() };
        }
    }

    template <typename T>
    constexpr SmoothMethod ValueNoise<T>::GetSmoothing() const noexcept
    {
        return smoothing;
    }

    template <typename T>
    constexpr void ValueNoise<T>::SetSmoothing(SmoothMethod smooth_method)
    {
        smoothing = smooth_method;
    }

    template <typename T>
    std::span<T> ValueNoise<T>::GetValues() noexcept
    {
        return std::span<T>{ values };
    }


}
