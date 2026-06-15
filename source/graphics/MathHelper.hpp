/**
 * \file
 * \author Rudy Castan
 * \date 2024 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>

namespace graphics
{
    inline glm::mat4 euler_angle_xyz_matrix(float x_angle, float y_angle, float z_angle)
    {
        const float c1 = glm::cos(-x_angle);
        const float c2 = glm::cos(-y_angle);
        const float c3 = glm::cos(-z_angle);
        const float s1 = glm::sin(-x_angle);
        const float s2 = glm::sin(-y_angle);
        const float s3 = glm::sin(-z_angle);

        glm::mat4 Result;
        Result[0][0] = c2 * c3;
        Result[0][1] = -c1 * s3 + s1 * s2 * c3;
        Result[0][2] = s1 * s3 + c1 * s2 * c3;
        Result[0][3] = 0.0f;
        Result[1][0] = c2 * s3;
        Result[1][1] = c1 * c3 + s1 * s2 * s3;
        Result[1][2] = -s1 * c3 + c1 * s2 * s3;
        Result[1][3] = 0.0f;
        Result[2][0] = -s2;
        Result[2][1] = s1 * c2;
        Result[2][2] = c1 * c2;
        Result[2][3] = 0.0f;
        Result[3][0] = 0.0f;
        Result[3][1] = 0.0f;
        Result[3][2] = 0.0f;
        Result[3][3] = 1.0f;
        return Result;
    }
}
