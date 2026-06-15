/**
 * \file
 * \author Rudy Castan
 * \date 2024 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include <GL/glew.h>
#include <glm/mat2x2.hpp> // mat2, dmat2
#include <glm/mat3x3.hpp> // mat3, dmat3
#include <glm/mat4x4.hpp> // mat4, dmat4
#include <glm/vec2.hpp>   // vec2, bvec2, dvec2, ivec2 and uvec2
#include <glm/vec3.hpp>   // vec3, bvec3, dvec3, ivec3 and uvec3
#include <glm/vec4.hpp>   // vec4, bvec4, dvec4, ivec4 and uvec4
#include <gsl/gsl>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace opengl
{
    class Texture;
    class Shader;
}

namespace graphics
{
    enum class CullFace : GLenum
    {
        Front        = GL_FRONT,
        Back         = GL_BACK,
        FrontAndBack = GL_FRONT_AND_BACK
    };

    enum class WindingOrder : GLenum
    {
        Clockwise        = GL_CW,
        CounterClockwise = GL_CCW
    };

    class Material
    {
    public:
        Material() = default;
        explicit Material(gsl::not_null<opengl::Shader*> a_shader, std::string_view name = "");

        using UniformValueType = std::variant<
            float, glm::vec2, glm::vec3, glm::vec4,                                      // float vectors
            int, glm::ivec2, glm::ivec3, glm::ivec4,                                     // int vectors, texture samplers
            unsigned, glm::uvec2, glm::uvec3, glm::uvec4,                                // unsigned vectors
            bool, glm::bvec2, glm::bvec3, glm::bvec4,                                    // bool vectors
            glm::mat2, glm::mat3, glm::mat4,                                             // matrices
            glm::mat2x3, glm::mat2x4, glm::mat3x2, glm::mat3x4, glm::mat4x2, glm::mat4x3 // non-square matrices
            >;
        void ForceApplyAllSettings() const;

        void SetMaterialUniform(std::string_view name, float value);
        void SetMaterialUniform(std::string_view name, glm::vec2 value);
        void SetMaterialUniform(std::string_view name, glm::vec3 value);
        void SetMaterialUniform(std::string_view name, glm::vec4 value);
        void SetMaterialUniform(std::string_view name, int value);
        void SetMaterialUniform(std::string_view name, glm::ivec2 value);
        void SetMaterialUniform(std::string_view name, glm::ivec3 value);
        void SetMaterialUniform(std::string_view name, glm::ivec4 value);
        void SetMaterialUniform(std::string_view name, unsigned value);
        void SetMaterialUniform(std::string_view name, glm::uvec2 value);
        void SetMaterialUniform(std::string_view name, glm::uvec3 value);
        void SetMaterialUniform(std::string_view name, glm::uvec4 value);
        void SetMaterialUniform(std::string_view name, bool value);
        void SetMaterialUniform(std::string_view name, glm::bvec2 value);
        void SetMaterialUniform(std::string_view name, glm::bvec3 value);
        void SetMaterialUniform(std::string_view name, glm::bvec4 value);
        void SetMaterialUniform(std::string_view name, glm::mat2 value);
        void SetMaterialUniform(std::string_view name, const glm::mat3& value);
        void SetMaterialUniform(std::string_view name, const glm::mat4& value);
        void SetMaterialUniform(std::string_view name, const glm::mat2x3& value);
        void SetMaterialUniform(std::string_view name, const glm::mat2x4& value);
        void SetMaterialUniform(std::string_view name, const glm::mat3x2& value);
        void SetMaterialUniform(std::string_view name, const glm::mat3x4& value);
        void SetMaterialUniform(std::string_view name, const glm::mat4x2& value);
        void SetMaterialUniform(std::string_view name, const glm::mat4x3& value);

        void SetTextures(std::span<const opengl::Texture* const> the_textures);
        void SetTextures(const std::initializer_list<const opengl::Texture*>& the_textures);

    private:
        void findAndAddUniforms();

        template <typename ValueType>
        void setMaterialUniform(std::string_view name, const ValueType& value)
        {
            for (auto& uniform : uniformValues)
            {
                if (uniform.Name == name)
                {
                    std::get<ValueType>(uniform.Value) = value;
                    break;
                }
            }
        }

    public:
        std::string Name{ "unnamed_material" };

        struct
        {
            bool         Enabled = true; // If double-sided then we do not do culling
            CullFace     Faces   = CullFace::Back;
            WindingOrder Winding = WindingOrder::CounterClockwise;
        } Culling;

        struct
        {
            bool EnableWriting = true;
            bool EnableTesting = true;
        } Depth;

    private:
        struct Uniform
        {
            std::string      Name;
            UniformValueType Value;
        };

        opengl::Shader*                     shaderPtr{ nullptr };
        std::vector<Uniform>                uniformValues{};
        std::vector<const opengl::Texture*> textures{};
    };

    static inline const Material DEFAULT_MATERIAL{};
}
