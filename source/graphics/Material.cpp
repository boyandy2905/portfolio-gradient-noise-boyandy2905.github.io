/**
 * \file
 * \author Rudy Castan
 * \date 2024 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#include "Material.hpp"

#include "opengl/GL.hpp"
#include "opengl/Shader.hpp"
#include "opengl/Texture.hpp"
#include "util/Logger.hpp"
#include <algorithm>
#include <gsl/gsl>

namespace
{
    void                                 apply_culling_settings(const graphics::Material& material);
    void                                 apply_depth_settings(const graphics::Material& material);
    graphics::Material::UniformValueType set_uniform(GLenum type, int& sampler_count);
}

namespace graphics
{
    Material::Material(gsl::not_null<opengl::Shader*> a_shader, std::string_view name)
    {
        using namespace std::string_literals;
        shaderPtr = a_shader;
        if (!name.empty())
            Name = name;
        else
            Name = shaderPtr->GetName() + "Material"s;
        findAndAddUniforms();
    }

    void Material::ForceApplyAllSettings() const
    {
        apply_culling_settings(*this);
        apply_depth_settings(*this);
        if (shaderPtr == nullptr)
            return;
        const auto& shader = *shaderPtr;
        shader.Use();
        for (const auto& uniform : uniformValues)
        {
            std::visit([&](auto&& value) { shader.SendUniform(uniform.Name, value); }, uniform.Value);
        }
        for (unsigned slot = 0; slot < textures.size(); ++slot)
        {
            const auto tex_ptr = textures[slot];
            tex_ptr->UseForSlot(slot);
        }
    }

    void Material::SetMaterialUniform(std::string_view name, float value)
    {
        setMaterialUniform(name, value);
    }

    void Material::SetMaterialUniform(std::string_view name, glm::vec2 value)
    {
        setMaterialUniform(name, value);
    }

    void Material::SetMaterialUniform(std::string_view name, glm::vec3 value)
    {
        setMaterialUniform(name, value);
    }

    void Material::SetMaterialUniform(std::string_view name, glm::vec4 value)
    {
        setMaterialUniform(name, value);
    }

    void Material::SetMaterialUniform(std::string_view name, int value)
    {
        setMaterialUniform(name, value);
    }

    void Material::SetMaterialUniform(std::string_view name, glm::ivec2 value)
    {
        setMaterialUniform(name, value);
    }

    void Material::SetMaterialUniform(std::string_view name, glm::ivec3 value)
    {
        setMaterialUniform(name, value);
    }

    void Material::SetMaterialUniform(std::string_view name, glm::ivec4 value)
    {
        setMaterialUniform(name, value);
    }

    void Material::SetMaterialUniform(std::string_view name, unsigned value)
    {
        setMaterialUniform(name, value);
    }

    void Material::SetMaterialUniform(std::string_view name, glm::uvec2 value)
    {
        setMaterialUniform(name, value);
    }

    void Material::SetMaterialUniform(std::string_view name, glm::uvec3 value)
    {
        setMaterialUniform(name, value);
    }

    void Material::SetMaterialUniform(std::string_view name, glm::uvec4 value)
    {
        setMaterialUniform(name, value);
    }

    void Material::SetMaterialUniform(std::string_view name, bool value)
    {
        setMaterialUniform(name, value);
    }

    void Material::SetMaterialUniform(std::string_view name, glm::bvec2 value)
    {
        setMaterialUniform(name, value);
    }

    void Material::SetMaterialUniform(std::string_view name, glm::bvec3 value)
    {
        setMaterialUniform(name, value);
    }

    void Material::SetMaterialUniform(std::string_view name, glm::bvec4 value)
    {
        setMaterialUniform(name, value);
    }

    void Material::SetMaterialUniform(std::string_view name, glm::mat2 value)
    {
        setMaterialUniform(name, value);
    }

    void Material::SetMaterialUniform(std::string_view name, const glm::mat3& value)
    {
        setMaterialUniform(name, value);
    }

    void Material::SetMaterialUniform(std::string_view name, const glm::mat4& value)
    {
        setMaterialUniform(name, value);
    }

    void Material::SetMaterialUniform(std::string_view name, const glm::mat2x3& value)
    {
        setMaterialUniform(name, value);
    }

    void Material::SetMaterialUniform(std::string_view name, const glm::mat2x4& value)
    {
        setMaterialUniform(name, value);
    }

    void Material::SetMaterialUniform(std::string_view name, const glm::mat3x2& value)
    {
        setMaterialUniform(name, value);
    }

    void Material::SetMaterialUniform(std::string_view name, const glm::mat3x4& value)
    {
        setMaterialUniform(name, value);
    }

    void Material::SetMaterialUniform(std::string_view name, const glm::mat4x2& value)
    {
        setMaterialUniform(name, value);
    }

    void Material::SetMaterialUniform(std::string_view name, const glm::mat4x3& value)
    {
        setMaterialUniform(name, value);
    }

    void Material::SetTextures(std::span<const opengl::Texture* const> the_textures)
    {
        using namespace std::string_literals;
        if (textures.size() != the_textures.size())
            throw std::runtime_error{ "Material "s + Name + " requires "s + std::to_string(textures.size()) + " textures but tried to set "s + std::to_string(the_textures.size()) };
        std::copy(std::begin(the_textures), std::end(the_textures), std::begin(textures));
    }

    void Material::SetTextures(const std::initializer_list<const opengl::Texture*>& the_textures)
    {
        SetTextures(std::span{ the_textures.begin(), the_textures.end() });
    }

    void Material::findAndAddUniforms()
    {
        // https://docs.gl/es3/glGetActiveUniform - reference
        GLint      max_name_length = 0;
        const auto program_handle  = shaderPtr->GetHandle();
        GL::GetProgramiv(program_handle, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_name_length);
        GLint num_uniforms = 0;
        GL::GetProgramiv(program_handle, GL_ACTIVE_UNIFORMS, &num_uniforms);
        const GLuint uniform_count = gsl::narrow<GLuint>(num_uniforms);
        std::string  name;
        int          sampler_count = 0;
        for (GLuint i = 0; i < uniform_count; ++i)
        {
            GLsizei written;
            GLint   size;
            GLenum  type;
            name.clear();
            name.resize(static_cast<unsigned>(max_name_length) + 1);
            GL::GetActiveUniform(program_handle, i, max_name_length, &written, &size, &type, name.data());
            name.resize(static_cast<std::string::size_type>(written));
            // skip failures, skip built in uniforms that start with gl_, skip arrays (for now)
            if (name.empty() || (name.size() > 3 && name[0] == 'g' && name[1] == 'l' && name[2] == '_') || size != 1)
                continue;
            uniformValues.push_back({ name, set_uniform(type, sampler_count) });
        }
        textures.resize(static_cast<size_t>(sampler_count));
    }
}

namespace
{
    void apply_culling_settings(const graphics::Material& material)
    {
        const auto enable_culling     = material.Culling.Enabled;
        const auto which_face_to_cull = material.Culling.Faces;
        const auto winding_order      = material.Culling.Winding;

        if (enable_culling)
        {
            GL::Enable(GL_CULL_FACE);
            GL::CullFace(static_cast<GLenum>(which_face_to_cull));
            GL::FrontFace(static_cast<GLenum>(winding_order));
        }
        else
        {
            GL::Disable(GL_CULL_FACE);
        }
    }

    void apply_depth_settings(const graphics::Material& material)
    {
        const auto enable_depth_writing = material.Depth.EnableWriting;
        if (const auto enable_depth_testing = material.Depth.EnableTesting; enable_depth_testing)
        {
            GL::Enable(GL_DEPTH_TEST);
            GL::DepthMask((enable_depth_writing) ? GL_TRUE : GL_FALSE);
        }
        else
        {
            GL::Disable(GL_DEPTH_TEST);
        }
    }

    graphics::Material::UniformValueType set_uniform(GLenum type, int& sampler_count)
    {
        graphics::Material::UniformValueType uniform;
        switch (type)
        {
            case GL_FLOAT: uniform = 0.0f; break;
            case GL_FLOAT_VEC2: uniform = glm::vec2(0.0f); break;
            case GL_FLOAT_VEC3: uniform = glm::vec3(0.0f); break;
            case GL_FLOAT_VEC4: uniform = glm::vec4(0.0f); break;
            case GL_INT: uniform = 0; break;
            case GL_INT_VEC2: uniform = glm::ivec2(0); break;
            case GL_INT_VEC3: uniform = glm::ivec3(0); break;
            case GL_INT_VEC4: uniform = glm::ivec4(0); break;
            case GL_UNSIGNED_INT: uniform = 0u; break;
            case GL_UNSIGNED_INT_VEC2: uniform = glm::uvec2(0u); break;
            case GL_UNSIGNED_INT_VEC3: uniform = glm::uvec3(0u); break;
            case GL_UNSIGNED_INT_VEC4: uniform = glm::uvec4(0u); break;
            case GL_BOOL: uniform = false; break;
            case GL_BOOL_VEC2: uniform = glm::bvec2(false); break;
            case GL_BOOL_VEC3: uniform = glm::bvec3(false); break;
            case GL_BOOL_VEC4: uniform = glm::bvec4(false); break;
            case GL_FLOAT_MAT2:
                {
                    uniform = glm::mat2(1.0f);
                }
                break;
            case GL_FLOAT_MAT3:
                {
                    uniform = glm::mat3(1.0f);
                }
                break;
            case GL_FLOAT_MAT4:
                {
                    uniform = glm::mat4(1.0f);
                }
                break;
            case GL_FLOAT_MAT2x3:
                {
                    uniform = glm::mat2x3(1.0f);
                }
                break;
            case GL_FLOAT_MAT2x4:
                {
                    uniform = glm::mat2x4(1.0f);
                }
                break;
            case GL_FLOAT_MAT3x2:
                {
                    uniform = glm::mat3x2(1.0f);
                }
                break;
            case GL_FLOAT_MAT3x4:
                {
                    uniform = glm::mat3x4(1.0f);
                }
                break;
            case GL_FLOAT_MAT4x2:
                {
                    uniform = glm::mat4x2(1.0f);
                }
                break;
            case GL_FLOAT_MAT4x3:
                {
                    uniform = glm::mat4x3(1.0f);
                }
                break;
            // Samplers
            case GL_SAMPLER_2D:
            case GL_SAMPLER_3D:
            case GL_SAMPLER_CUBE:
            case GL_SAMPLER_2D_SHADOW:
            case GL_SAMPLER_2D_ARRAY:
            case GL_SAMPLER_2D_ARRAY_SHADOW:
            case GL_SAMPLER_CUBE_SHADOW:
            case GL_INT_SAMPLER_2D:
            case GL_INT_SAMPLER_3D:
            case GL_INT_SAMPLER_CUBE:
            case GL_INT_SAMPLER_2D_ARRAY:
            case GL_UNSIGNED_INT_SAMPLER_2D:
            case GL_UNSIGNED_INT_SAMPLER_3D:
            case GL_UNSIGNED_INT_SAMPLER_CUBE:
            case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
                {
                    uniform = sampler_count;
                    ++sampler_count;
                }
                break;

            default:
                // Handle other cases or throw an exception for unsupported types
                util::log_error("Unsupported GLenum: {}", type);
                assert(false);
                break;
        }

        return uniform;
    }


}
