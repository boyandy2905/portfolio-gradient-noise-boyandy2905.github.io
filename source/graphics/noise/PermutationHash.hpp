/**
 * \file
 * \author Sehoon Kim(sehoon.kim@digipen.edu)
 * \date 2026 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#pragma once


#include <algorithm> // std::copy
#include <numeric> // std::iota
#include "util/Random.hpp"
#include <utility> // std::swap
#include <vector>

#include "PeriodDimension.hpp"

namespace graphics::noise
{
    template <class RandomAccessIter>
    void my_random_shuffle(RandomAccessIter first, RandomAccessIter last)
    {
        int target_index = 1;
        for (auto target = first + 1; target != last; ++target_index, ++target)
        {
            const int offset = util::random(target_index + 1);
            if (offset != target_index)
            {
                std::swap(*target, *(first + offset));
            }
        }
    }

    class PermutationHash
    {

    public:
        PermutationHash() = default;

        explicit PermutationHash(PeriodDimension table_size)
        {
            const int size = static_cast<int>(table_size);
            mask           = period_dimension_mask(table_size);

            permutation_table.resize(static_cast<std::size_t>(size));

            std::iota(permutation_table.begin(), permutation_table.end(), 0);
            my_random_shuffle(permutation_table.begin(), permutation_table.end());
        }

        [[nodiscard]] int operator()(int x) const noexcept
        {
            return permutation_table[static_cast<std::size_t>(x & mask)];
        }

        [[nodiscard]] int operator()(int x, int y) const noexcept
        {
            const int hashed_y = permutation_table[static_cast<std::size_t>(y & mask)];
            return permutation_table[static_cast<std::size_t>((x + hashed_y) & mask)];
        }

        [[nodiscard]] int operator()(int x, int y, int z) const noexcept
        {
            const int hashed_z  = permutation_table[static_cast<std::size_t>(z & mask)];
            const int hashed_yz = permutation_table[static_cast<std::size_t>((y + hashed_z) & mask)];

            return permutation_table[static_cast<std::size_t>((x + hashed_yz) & mask)];
        }

    private:
        std::vector<int> permutation_table{};
        int              mask = 0;
    };

}
