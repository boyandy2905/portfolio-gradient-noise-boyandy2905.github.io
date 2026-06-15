/**
 * \file
 * \author Rudy Castan
 * \author Sehoon Kim(sehoon.kim@digipen.edu)
 * \date 2024 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#include "Mesh.hpp"
#include <cmath>
#include <glm/ext/matrix_transform.hpp>
#include <gsl/gsl>
#include <numbers>

namespace
{
    std::vector<graphics::MeshVertex> create_plane_vertices(int stacks, int slices);
    std::vector<unsigned>             build_index_buffer(int stacks, int slices);
    std::vector<unsigned>             convert_to_lines_pattern(const std::vector<unsigned>& indices);
    glm::vec3                         evaluate_clover(float row, float col, float side);

}

namespace graphics
{
    Geometry create_plane(int stacks, int slices)
    {
        auto vertices = create_plane_vertices(stacks, slices);
        auto indices  = build_index_buffer(stacks, slices);
        return Geometry{ std::move(vertices), std::move(indices) };
    }

    Geometry create_cube(int stacks, int slices)
    {
        const auto plane_vertices = create_plane_vertices(stacks, slices);
        const auto plane_indices  = build_index_buffer(stacks, slices);

        enum Axis
        {
            X,
            Y
        };

        constexpr glm::vec3 AxisVectors[2] = { glm::vec3(1, 0, 0), glm::vec3(0, 1, 0) };

        constexpr glm::vec3 translate_array[] = {
            glm::vec3(+0.0f, +0.0f, +0.5f), // Z+
            glm::vec3(+0.0f, +0.0f, -0.5f), // Z-
            glm::vec3(+0.5f, +0.0f, +0.0f), // X+
            glm::vec3(-0.5f, +0.0f, +0.0f), // X-
            glm::vec3(+0.0f, +0.5f, +0.0f), // Y+
            glm::vec3(+0.0f, -0.5f, +0.0f), // Y-
        };

        struct rotation
        {
            Axis  axis;
            float angle;
        };

        constexpr rotation rotate_array[] = {
            { Axis::Y,                 0.0f }, // Z+
            { Axis::Y, glm::radians(180.0f) }, // Z-
            { Axis::Y,  glm::radians(90.0f) }, // X+
            { Axis::Y, glm::radians(-90.0f) }, // X-
            { Axis::X, glm::radians(-90.0f) }, // Y+
            { Axis::X,  glm::radians(90.0f) }  // Y-
        };

        std::vector<MeshVertex> vertices;
        std::vector<unsigned>   indices;
        vertices.reserve(plane_vertices.size() * 6u);
        indices.reserve(plane_indices.size() * 6u);
        /*  Transform the plane to 6 positions to form the faces of the cube */
        for (unsigned i = 0; i < 6; ++i)
        {
            constexpr glm::mat4 identity(1.0f);
            const auto          axis             = AxisVectors[rotate_array[i].axis];
            const auto          angle            = rotate_array[i].angle;
            const auto          rotation_matrix  = glm::rotate(identity, angle, axis);
            const auto          transform_matrix = glm::translate(identity, translate_array[i]) * rotation_matrix;
            for (const auto plane_vertex : plane_vertices)
            {
                MeshVertex v;
                v.position = glm::vec3(transform_matrix * glm::vec4(plane_vertex.position, 1.0));
                v.normal   = glm::vec3(rotation_matrix * glm::vec4(plane_vertex.normal, 1.0));
                v.uv       = plane_vertex.uv;
                vertices.push_back(v);
            }
            for (const auto plane_index : plane_indices)
            {
                indices.push_back(plane_index + static_cast<unsigned>(plane_vertices.size()) * i);
            }
        }
        return Geometry{ std::move(vertices), std::move(indices) };
    }

    Geometry create_sphere(int stacks, int slices)
    {
        std::vector<MeshVertex> vertices;
        vertices.reserve(static_cast<std::size_t>((stacks + 1) * (slices + 1)));

        constexpr float radius = 0.5f;

        for (int stack = 0; stack <= stacks; ++stack)
        {
            const float row      = static_cast<float>(stack) / static_cast<float>(stacks);
            const float beta     = PI * (row - 0.5f);
            const float sin_beta = std::sin(beta);
            const float cos_beta = std::cos(beta);

            for (int slice = 0; slice <= slices; ++slice)
            {
                const float col   = static_cast<float>(slice) / static_cast<float>(slices);
                const float alpha = col * 2.0f * PI;

                const float sin_alpha = std::sin(alpha);
                const float cos_alpha = std::cos(alpha);

                MeshVertex vertex;
                vertex.position = glm::vec3(
                    radius * sin_alpha * cos_beta,
                    radius * sin_beta,
                    radius * cos_alpha * cos_beta
                );
                vertex.normal = vertex.position / radius;
                vertex.uv     = glm::vec2(col, row);

                vertices.push_back(vertex);
            }
        }

        auto indices = build_index_buffer(stacks, slices);
        return Geometry{ std::move(vertices), std::move(indices) };
    }

    Geometry create_torus(int stacks, int slices, float start_angle, float end_angle)
    {
        std::vector<MeshVertex> vertices;
        vertices.reserve(static_cast<std::size_t>((stacks + 1) * (slices + 1)));

        constexpr float major_radius = 0.35f;
        constexpr float minor_radius = 0.15f;

        for (int stack = 0; stack <= stacks; ++stack)
        {
            const float row   = static_cast<float>(stack) / static_cast<float>(stacks);
            const float alpha = std::lerp(start_angle, end_angle, row);

            const float sin_alpha = std::sin(alpha);
            const float cos_alpha = std::cos(alpha);

            for (int slice = 0; slice <= slices; ++slice)
            {
                const float col  = static_cast<float>(slice) / static_cast<float>(slices);
                const float beta = std::lerp(2.0f * PI, 0.0f, col);

                const float sin_beta = std::sin(beta);
                const float cos_beta = std::cos(beta);

                MeshVertex vertex;
                vertex.position.x = (major_radius + minor_radius * cos_beta) * sin_alpha;
                vertex.position.y = minor_radius * sin_beta;
                vertex.position.z = (major_radius + minor_radius * cos_beta) * cos_alpha;

                const glm::vec3 center{
                    major_radius * sin_alpha,
                    0.0f,
                    major_radius * cos_alpha
                };

                vertex.normal = (vertex.position - center) / minor_radius;
                vertex.uv     = glm::vec2(col, row);

                vertices.push_back(vertex);
            }
        }

        auto indices = build_index_buffer(stacks, slices);
        return Geometry{ std::move(vertices), std::move(indices) };
    }

    void add_cap(std::vector<MeshVertex>& vertices, std::vector<unsigned>& indices, float center_y, int slices)
    {
        constexpr float radius = 0.5f;

        const unsigned center_index = static_cast<unsigned>(vertices.size());

        MeshVertex vertex;
        vertex.position = glm::vec3(0.0f, center_y, 0.0f);
        vertex.normal   = center_y > 0.0f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(0.0f, -1.0f, 0.0f);
        vertex.uv       = glm::vec2(0.5f, 0.5f);
        vertices.push_back(vertex);

        const float texture_coord_scale = center_y > 0.0f ? 1.0f : -1.0f;

        for (int slice = 0; slice <= slices; ++slice)
        {
            const float col   = static_cast<float>(slice) / static_cast<float>(slices);
            const float alpha = col * 2.0f * PI;

            const float sin_alpha = std::sin(alpha);
            const float cos_alpha = std::cos(alpha);

            vertex.position = glm::vec3(
                radius * sin_alpha,
                center_y,
                radius * cos_alpha
            );
            vertex.normal = center_y > 0.0f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(0.0f, -1.0f, 0.0f);
            vertex.uv     = glm::vec2(
                texture_coord_scale * 0.5f * cos_alpha + 0.5f,
                0.5f * sin_alpha + 0.5f
            );

            vertices.push_back(vertex);
        }

        for (int slice = 0; slice < slices; ++slice)
        {
            const unsigned first_ring_index  = center_index + 1u + static_cast<unsigned>(slice);
            const unsigned second_ring_index = first_ring_index + 1u;

            if (center_y > 0.0f)
            {
                indices.push_back(center_index);
                indices.push_back(first_ring_index);
                indices.push_back(second_ring_index);
            }
            else
            {
                indices.push_back(center_index);
                indices.push_back(second_ring_index);
                indices.push_back(first_ring_index);
            }
        }
    }

    Geometry create_cylinder(int stacks, int slices)
    {
        std::vector<MeshVertex> vertices;
        vertices.reserve(static_cast<std::size_t>((stacks + 1) * (slices + 1) + (slices + 2) * 2));

        constexpr float radius = 0.5f;

        for (int stack = 0; stack <= stacks; ++stack)
        {
            const float row = static_cast<float>(stack) / static_cast<float>(stacks);

            for (int slice = 0; slice <= slices; ++slice)
            {
                const float col   = static_cast<float>(slice) / static_cast<float>(slices);
                const float alpha = col * 2.0f * PI;

                const float sin_alpha = std::sin(alpha);
                const float cos_alpha = std::cos(alpha);

                MeshVertex vertex;
                vertex.position = glm::vec3(
                    radius * sin_alpha,
                    row - 0.5f,
                    radius * cos_alpha
                );
                vertex.normal = glm::vec3(sin_alpha, 0.0f, cos_alpha);
                vertex.uv     = glm::vec2(col, row);

                vertices.push_back(vertex);
            }
        }

        auto indices = build_index_buffer(stacks, slices);

        add_cap(vertices, indices, 0.5f, slices);
        add_cap(vertices, indices, -0.5f, slices);

        return Geometry{ std::move(vertices), std::move(indices) };
    }

    Geometry create_cone(int stacks, int slices)
    {
        std::vector<MeshVertex> vertices;
        vertices.reserve(static_cast<std::size_t>((stacks + 1) * (slices + 1) + (slices + 2)));

        constexpr float radius        = 0.5f;
        constexpr float top_radius    = 0.0f;
        constexpr float bottom_radius = radius;
        constexpr float top_y         = 0.5f;
        constexpr float bottom_y      = -0.5f;

        constexpr float rise          = top_y - bottom_y;
        constexpr float run           = top_radius - bottom_radius;
        constexpr float slope         = rise / run;
        constexpr float tangent_slope = -1.0f / slope;

        for (int stack = 0; stack <= stacks; ++stack)
        {
            const float row = static_cast<float>(stack) / static_cast<float>(stacks);
            const float h   = row - 0.5f;

            for (int slice = 0; slice <= slices; ++slice)
            {
                const float col   = static_cast<float>(slice) / static_cast<float>(slices);
                const float alpha = col * 2.0f * PI;

                const float sin_alpha = std::sin(alpha);
                const float cos_alpha = std::cos(alpha);

                MeshVertex vertex;
                vertex.position = glm::vec3(
                    radius * (0.5f - h) * sin_alpha,
                    h,
                    radius * (0.5f - h) * cos_alpha
                );

                if (stack != stacks)
                {
                    vertex.normal = glm::normalize(glm::vec3(sin_alpha, tangent_slope, cos_alpha));
                }
                else
                {
                    vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                }

                vertex.uv = glm::vec2(col, row);

                vertices.push_back(vertex);
            }
        }

        auto indices = build_index_buffer(stacks, slices);

        add_cap(vertices, indices, -0.5f, slices);

        return Geometry{ std::move(vertices), std::move(indices) };
    }

    Geometry create_clover(int stacks, int slices)
    {
        std::vector<MeshVertex> vertices;
        std::vector<unsigned>   indices;

        const int stride      = slices + 1;
        const int vertex_grid = (stacks + 1) * (slices + 1);

        vertices.reserve(static_cast<std::size_t>(vertex_grid * 2 + (slices + 1) * 2));
        indices.reserve(static_cast<std::size_t>(stacks * slices * 12 + slices * 6));

        for (int stack = 0; stack <= stacks; ++stack)
        {
            const float row = static_cast<float>(stack) / static_cast<float>(stacks);

            for (int slice = 0; slice <= slices; ++slice)
            {
                const float col = static_cast<float>(slice) / static_cast<float>(slices);

                MeshVertex vertex;
                vertex.position = evaluate_clover(row, col, 1.0f);
                vertex.normal   = glm::normalize(glm::vec3(vertex.position.x, 0.45f, vertex.position.z));
                vertex.uv       = glm::vec2(col, row);

                vertices.push_back(vertex);
            }
        }

        const unsigned bottom_offset = static_cast<unsigned>(vertices.size());

        for (int stack = 0; stack <= stacks; ++stack)
        {
            const float row = static_cast<float>(stack) / static_cast<float>(stacks);

            for (int slice = 0; slice <= slices; ++slice)
            {
                const float col = static_cast<float>(slice) / static_cast<float>(slices);

                MeshVertex vertex;
                vertex.position = evaluate_clover(row, col, -1.0f);
                vertex.normal   = glm::normalize(glm::vec3(vertex.position.x, -0.45f, vertex.position.z));
                vertex.uv       = glm::vec2(col, row);

                vertices.push_back(vertex);
            }
        }

        auto surface_indices = build_index_buffer(stacks, slices);

        for (unsigned index : surface_indices)
        {
            indices.push_back(index);
        }

        for (std::size_t i = 0; i < surface_indices.size(); i += 3u)
        {
            indices.push_back(bottom_offset + surface_indices[i]);
            indices.push_back(bottom_offset + surface_indices[i + 2u]);
            indices.push_back(bottom_offset + surface_indices[i + 1u]);
        }

        const unsigned top_outer_start    = static_cast<unsigned>(stacks * stride);
        const unsigned bottom_outer_start = bottom_offset + top_outer_start;

        for (int slice = 0; slice < slices; ++slice)
        {
            const unsigned top0    = top_outer_start + static_cast<unsigned>(slice);
            const unsigned top1    = top0 + 1u;
            const unsigned bottom0 = bottom_outer_start + static_cast<unsigned>(slice);
            const unsigned bottom1 = bottom0 + 1u;

            indices.push_back(top0);
            indices.push_back(bottom0);
            indices.push_back(top1);

            indices.push_back(top1);
            indices.push_back(bottom0);
            indices.push_back(bottom1);
        }

        return Geometry{ std::move(vertices), std::move(indices) };
    }

    SubMesh to_submesh_as_triangles(const Geometry& geometry, Material* material)
    {
        SubMesh sub_mesh;
        sub_mesh.VertexArrayObj.SetPrimitivePattern(opengl::Primitive::Triangles);
        sub_mesh.VertexArrayObj.AddVertexBuffer(opengl::VertexBuffer(std::span{ geometry.Vertices }), get_meshvertex_layout());
        sub_mesh.VertexArrayObj.SetIndexBuffer(opengl::IndexBuffer(std::span{ geometry.Indicies }));
        sub_mesh.Material = material;
        return sub_mesh;
    }

    SubMesh to_submesh_as_lines(const Geometry& geometry, Material* material)
    {
        std::vector<unsigned> lines_indices = convert_to_lines_pattern(geometry.Indicies);
        SubMesh               sub_mesh;
        sub_mesh.VertexArrayObj.SetPrimitivePattern(opengl::Primitive::Lines);
        sub_mesh.VertexArrayObj.AddVertexBuffer(opengl::VertexBuffer(std::span{ geometry.Vertices }), get_meshvertex_layout());
        sub_mesh.VertexArrayObj.SetIndexBuffer(opengl::IndexBuffer(std::span{ lines_indices }));
        sub_mesh.Material = material;
        return sub_mesh;
    }
}

namespace
{
    std::vector<graphics::MeshVertex> create_plane_vertices(int stacks, int slices)
    {
        std::vector<graphics::MeshVertex> vertices;
        vertices.reserve(static_cast<std::size_t>((stacks + 1) * (slices + 1)));

        for (int stack = 0; stack <= stacks; ++stack)
        {
            const float row = static_cast<float>(stack) / static_cast<float>(stacks);

            for (int slice = 0; slice <= slices; ++slice)
            {
                const float col = static_cast<float>(slice) / static_cast<float>(slices);

                graphics::MeshVertex vertex;
                vertex.position = glm::vec3(col - 0.5f, row - 0.5f, 0.0f);
                vertex.normal   = glm::vec3(0.0f, 0.0f, 1.0f);
                vertex.uv       = glm::vec2(col, row);

                vertices.push_back(vertex);
            }
        }

        return vertices;
    }

    std::vector<unsigned> build_index_buffer(int stacks, int slices)
    {
        std::vector<unsigned> indices;

        const auto num_triangles = static_cast<std::size_t>(stacks * slices * 2);
        indices.reserve(num_triangles * 3u);

        const int stride = slices + 1;

        for (int stack = 0; stack < stacks; ++stack)
        {
            const int curr_row = stack * stride;

            for (int slice = 0; slice < slices; ++slice)
            {
                const unsigned p0 = static_cast<unsigned>(curr_row + slice);
                const unsigned p1 = p0 + 1u;
                const unsigned p2 = p1 + static_cast<unsigned>(stride);

                indices.push_back(p0);
                indices.push_back(p1);
                indices.push_back(p2);

                const unsigned p3 = p2;
                const unsigned p4 = p3 - 1u;
                const unsigned p5 = p0;

                indices.push_back(p3);
                indices.push_back(p4);
                indices.push_back(p5);
            }
        }

        return indices;
    }

    std::vector<unsigned> convert_to_lines_pattern(const std::vector<unsigned>& indices)
    {
        std::vector<unsigned> lines_indices;
        lines_indices.reserve(indices.size() * 2u);

        std::size_t i = 0;

        if (indices.size() > 6u)
        {
            const std::size_t limit = indices.size() % 6u == 0u ? indices.size() : indices.size() - 5u;

            while (i < limit)
            {
                const unsigned p0 = indices[i];
                const unsigned p1 = indices[i + 1u];
                const unsigned p2 = indices[i + 2u];
                const unsigned p3 = indices[i + 3u];
                const unsigned p4 = indices[i + 4u];
                const unsigned p5 = indices[i + 5u];

                if (p1 != p0 + 1u || p3 != p2 || p4 != p3 - 1u || p5 != p0)
                {
                    break;
                }

                lines_indices.push_back(p0);
                lines_indices.push_back(p1);

                lines_indices.push_back(p1);
                lines_indices.push_back(p2);

                lines_indices.push_back(p2);
                lines_indices.push_back(p4);

                lines_indices.push_back(p4);
                lines_indices.push_back(p0);

                i += 6u;
            }
        }

        if (i < indices.size() && indices.size() - i >= 3u)
        {
            const std::size_t limit = indices.size() % 3u == 0u ? indices.size() : indices.size() - 2u;

            while (i < limit)
            {
                const unsigned p0 = indices[i];
                const unsigned p1 = indices[i + 1u];
                const unsigned p2 = indices[i + 2u];

                lines_indices.push_back(p0);
                lines_indices.push_back(p1);

                lines_indices.push_back(p1);
                lines_indices.push_back(p2);

                lines_indices.push_back(p2);
                lines_indices.push_back(p0);

                i += 3u;
            }
        }

        return lines_indices;
    }

    glm::vec3 evaluate_clover(float row, float col, float side)
    {
        constexpr float PI = graphics::PI;

        const float theta = col * 2.0f * PI;

        const float petal_shape = std::pow(std::abs(std::cos(2.0f * theta)), 0.65f);

        constexpr float max_radius = 0.5f;
        const float     radius     = row * max_radius * (0.12f + 0.88f * petal_shape);

        constexpr float base_thickness = 0.0f;
        constexpr float dome_thickness = 0.075f;
        const float     dome           = std::sin(row * PI);
        const float     thickness      = base_thickness + dome_thickness * dome * petal_shape;

        glm::vec3 position;
        position.x = radius * std::cos(theta);
        position.y = side * thickness;
        position.z = radius * std::sin(theta);

        return position;
    }
}
