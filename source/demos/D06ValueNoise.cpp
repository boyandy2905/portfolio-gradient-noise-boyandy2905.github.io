/**
 * \file
 * \author Rudy Castan
 * \date 2024 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#include "D06ValueNoise.hpp"

#include "opengl/Environment.hpp"
#include "opengl/GL.hpp"
#include "util/Random.hpp"
#include "window/Environment.hpp"
#include <glm/ext/matrix_clip_space.hpp> // perspective
#include <imgui.h>

namespace
{
    using namespace std::string_literals;

    namespace Uniforms
    {
        const auto Projection = "uProjection"s;
        const auto TileScale  = "uTileScale"s;

    }

    namespace asset_paths
    {
        constexpr auto SimpleVertexPath   = "D06ValueNoise/simple.vert";
        constexpr auto SimpleFragmentPath = "D06ValueNoise/simple.frag";

    }

    opengl::Texture::RGBA vec4_to_rgba(const glm::vec4& color);

    bool is_close(float a, float b, float tolerance)
    {
        return std::fabs(a - b) <= tolerance;
    }

    void override_if_close(float& target, float number, float tolerance = 1e-1f)
    {
        if (is_close(target, number, tolerance))
        {
            target = number;
        }
    }
}

namespace demos
{
    D06ValueNoise::D06ValueNoise()
    {
        GL::ClearColor(0.392f, 0.584f, 0.929f, 1.0f);

        assetReloader.SetAndAutoReloadShader(displayTextureShader, "Display Texture Shader", { asset_paths::SimpleVertexPath, asset_paths::SimpleFragmentPath });
        const auto texture_size = static_cast<size_t>(textureSize);
        colors.resize(texture_size * texture_size);

        if (const bool loaded = generatedTexture.LoadFromMemory(textureSize, textureSize, colors.data()); !loaded)
        {
            throw std::runtime_error{ "Failed to create 2D texture image in D06ValueNoise" };
        }
        generatedTexture.SetFiltering(opengl::Texture::NearestPixel);
        generatedTexture.SetWrapping(opengl::Texture::Repeat);

        displayTextureMaterial = graphics::Material(&displayTextureShader, "Full Screen Material");
        displayTextureMaterial.SetTextures({ &generatedTexture });

        graphics::Geometry full_screen_geometry = graphics::create_plane(1, 1);
        for (auto& vert : full_screen_geometry.Vertices)
        {
            vert.position *= 2.0f;
        }
        quadMesh = graphics::to_submesh_as_triangles(full_screen_geometry, &displayTextureMaterial);

        D06ValueNoise::SetDisplaySize(window::DisplayWidth, window::DisplayHeight);
        generation.state = Generation::Setup;
    }

    void D06ValueNoise::Update()
    {
        switch (generation.state)
        {
            case Generation::Setup: generation.setup(*this); break;
            case Generation::Working: generation.update(*this); break;
            case Generation::UploadNewTexture:
                generatedTexture.UploadAsRGBA(colors.data());
                generation.state = Generation::Done;
                break;
            case Generation::Done: break;
        }

        constexpr float FUDGE_FACTOR = 1.25;
        auto            easing       = std::min(window::DeltaTime * FUDGE_FACTOR, 1.0f);
        tileScale += easing * (targetTileScale - tileScale);
        if (generation.z != targetZ && generation.state == Generation::Done)
        {
            easing = std::min(window::DeltaTime * 8.0f, 1.0f);
            generation.z += easing * (targetZ - generation.z);
            override_if_close(generation.z, targetZ);
            generation.state = Generation::Setup;
        }

        assetReloader.Update();
        displayTextureMaterial.SetMaterialUniform(Uniforms::Projection, orthoProjectionMatrix);
        displayTextureMaterial.SetMaterialUniform(Uniforms::TileScale, tileScale);
    }

    void D06ValueNoise::Draw() const
    {
        GL::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        displayTextureMaterial.ForceApplyAllSettings();
        quadMesh.VertexArrayObj.Use();
        opengl::DrawIndexed(quadMesh.VertexArrayObj);
    }

    void D06ValueNoise::ImGuiDraw()
    {
        if (generation.state != Generation::Done)
        {
            // ImGui::Spinner("##spinner", 32, 6, IM_COL32_WHITE);
            ImGui::Text("Generating noise %c", "|/-\\"[static_cast<int>(ImGui::GetTime() * 3.5) & 3]);
        }
        else
        {
            bool reset_values = false;
            {
                using namespace graphics::noise;
                constexpr auto            period_dimension_names = " 2\0 4\0 8\0 16\0 32\0 64\0 128\0 256\0 512\0 1024\0 2048\0 4096\0 8192\0 16384\0\0";
                constexpr PeriodDimension TableSizes[]           = { PeriodDimension::_2,    PeriodDimension::_4,    PeriodDimension::_8,    PeriodDimension::_16,   PeriodDimension::_32,
                                                                     PeriodDimension::_64,   PeriodDimension::_128,  PeriodDimension::_256,  PeriodDimension::_512,  PeriodDimension::_1024,
                                                                     PeriodDimension::_2048, PeriodDimension::_4096, PeriodDimension::_8192, PeriodDimension::_16384

                };
                const auto size_itr   = std::find(std::begin(TableSizes), std::end(TableSizes), noisePeriod);
                int        size_index = (size_itr != std::end(TableSizes)) ? static_cast<int>(std::distance(std::begin(TableSizes), size_itr)) : 0;
                if (ImGui::Combo("Period Dimension", &size_index, period_dimension_names))
                {
                    noisePeriod = TableSizes[size_index];
                    noise.SetPeriod(noisePeriod);
                    generation.state = Generation::Setup;
                    reset_values     = true;
                }
            }

            constexpr auto smooth_method_names = "Linear\0Cosine\0Smoothstep\0Quintic\0\0";
            if (ImGui::Combo("Smooth Method", reinterpret_cast<int*>(&noiseSmoothMethod), smooth_method_names))
            {
                noise.SetSmoothing(noiseSmoothMethod);
                generation.state = Generation::Setup;
            }

            static const auto texture_sizes_info = []
            {
                std::vector<int> sizes;
                std::string      names;
                int              size = opengl::MaxTextureSize;
                while (size >= 64)
                {
                    sizes.push_back(size);
                    names += std::to_string(size);
                    names += '\0';
                    size >>= 1;
                }
                names += '\0';
                return std::make_tuple(sizes, names);
            }();
            const auto& [texture_sizes, texture_sizes_string] = texture_sizes_info;
            {
                const auto size_itr       = std::find(std::begin(texture_sizes), std::end(texture_sizes), textureSize);
                int        width_location = (size_itr != std::end(texture_sizes)) ? static_cast<int>(std::distance(std::begin(texture_sizes), size_itr)) : 0;
                if (ImGui::Combo("Texture Dimensions", &width_location, texture_sizes_string.c_str()))
                {
                    const int new_size = texture_sizes[static_cast<size_t>(width_location)];
                    if (opengl::Texture new_one; new_one.LoadAsRGBA(new_size, new_size))
                    {
                        new_one.SetFiltering(opengl::Texture::NearestPixel);
                        new_one.SetWrapping(opengl::Texture::Repeat);

                        textureSize      = new_size;
                        generatedTexture = std::move(new_one);
                    }
                    generation.state = Generation::Setup;
                }
            }


            ImGui::SliderFloat("Tiling Scale", &targetTileScale.x, 0.1f, 3.0f);
            switch (generation.noiseDimension)
            {
                case Dimension::_1D: targetTileScale.y = 0.0f; break;
                case Dimension::_2D:
                case Dimension::_3D: targetTileScale.y = targetTileScale.x; break;
            }

            constexpr auto dimension_type_names = " 1D\0 2D\0 3D\0\0";
            if (ImGui::Combo("Noise Dimension", reinterpret_cast<int*>(&generation.noiseDimension), dimension_type_names))
            {
                generation.state = Generation::Setup;
                if (opengl::Texture new_one; new_one.LoadAsRGBA(textureSize, textureSize))
                {
                    new_one.SetFiltering(opengl::Texture::NearestPixel);
                    new_one.SetWrapping(opengl::Texture::Repeat);

                    generatedTexture = std::move(new_one);
                }
            }

            if (generation.noiseDimension == Dimension::_3D)
            {
                ImGui::SliderFloat("Z Input", &targetZ, 0.0f, static_cast<float>(noisePeriod));
            }

            constexpr auto pattern_type_names = "Value\0Fractal Sum\0Turbulence\0Marble\0Wood\0\0";
            if (ImGui::Combo("Pattern Type", reinterpret_cast<int*>(&generation.pattern), pattern_type_names))
            {
                generation.state = Generation::Setup;
            }


            if (ImGui::Button("Use Random Grey Values") || (reset_values && lastPickedValueSet == ValueSet::RandomGrey))
            {
                auto values = noise.GetValues();
                for (auto& value : values)
                {
                    value = glm::vec4(glm::vec3(util::random()), 1.0f);
                }
                generation.state   = Generation::Setup;
                lastPickedValueSet = ValueSet::RandomGrey;
            }

            if (ImGui::Button("Use Random Color Values") || (reset_values && lastPickedValueSet == ValueSet::RandomColor))
            {
                auto values = noise.GetValues();
                for (auto& value : values)
                {
                    value = glm::vec4(glm::vec3(util::random(), util::random(), util::random()), 1.0f);
                }
                generation.state   = Generation::Setup;
                lastPickedValueSet = ValueSet::RandomColor;
            }

            if (ImGui::Button("Use Uniform Grey Values") || (reset_values && lastPickedValueSet == ValueSet::UniformGrey))
            {
                auto        values        = noise.GetValues();
                float       current_value = 0;
                const float inc           = 1.0f / float(values.size() - 1);
                for (auto& value : values)
                {
                    value = glm::vec4(glm::vec3(current_value), 1.0f);
                    current_value += inc;
                }
                generation.state   = Generation::Setup;
                lastPickedValueSet = ValueSet::UniformGrey;
            }
        }
    }

    void D06ValueNoise::SetDisplaySize(int width, int height)
    {
        if (width >= height)
        {
            const float aspect    = static_cast<float>(width) / static_cast<float>(height);
            orthoProjectionMatrix = glm::ortho(-aspect, aspect, -1.0f, 1.0f);
        }
        else
        {
            const float aspect    = static_cast<float>(height) / static_cast<float>(width);
            orthoProjectionMatrix = glm::ortho(-1.0f, 1.0f, -aspect, aspect);
        }
    }

    void D06ValueNoise::Generation::setup(D06ValueNoise& demo)
    {
        const auto texture_size = static_cast<size_t>(demo.textureSize);
        demo.colors.resize(texture_size * texture_size);
        frequency = static_cast<float>(demo.noisePeriod) / static_cast<float>(demo.textureSize);
        xyInputValues.resize(texture_size);
        for (column = 0; column < demo.textureSize; ++column)
        {
            const float x                              = static_cast<float>(column) * frequency;
            xyInputValues[static_cast<size_t>(column)] = x;
        }
        the_colors = demo.colors.data();
        row = 0, column = 0;
        y = 0;
        switch (noiseDimension)
        {
            case Dimension::_1D:
                width  = demo.textureSize;
                height = 1;
                break;
            case Dimension::_2D:
            case Dimension::_3D: width = height = demo.textureSize; break;
        }
        state = Generation::Working;
        if constexpr (window::CanUseThreads)
        {
            jobSystem.DoJobs(
                width * height,
                [this, &demo](int index)
                {
                    const int   r     = index / width;
                    const int   c     = index % width;
                    const float the_x = xyInputValues[static_cast<size_t>(c)];
                    const float the_y = xyInputValues[static_cast<size_t>(r)];
                    the_colors[index] = get_color(demo.noise, the_x, the_y, z);
                    //
                });
        }
    }

    void D06ValueNoise::Generation::update([[maybe_unused]] const D06ValueNoise& demo)
    {
        if constexpr (window::CanUseThreads)
        {
            if (jobSystem.IsDone())
            {
                state = Generation::UploadNewTexture;
            }
        }
        else
        {
            timer.ResetTimeStamp();
            while (timer.GetElapsedSeconds() < 1.0f / 32.0f)
            {
                const float x = xyInputValues[static_cast<size_t>(column)];
                *the_colors   = get_color(demo.noise, x, y, z);
                ++the_colors;
                ++column;
                if (column >= width)
                {
                    ++row;
                    column = 0;
                    if (row >= height)
                    {
                        state = Generation::UploadNewTexture;
                        break;
                    }
                    y = xyInputValues[static_cast<size_t>(row)];
                }
            }
        }
    }

    opengl::Texture::RGBA D06ValueNoise::Generation::get_color(const graphics::noise::ValueNoise<glm::vec4>& the_noise, float the_x, float the_y, float the_z) const
    {
        const auto eval = [&]()
        {
            switch (noiseDimension)
            {
                case Dimension::_1D: return the_noise.Evaluate(the_x);
                case Dimension::_2D: return the_noise.Evaluate(the_x, the_y);
                default: return the_noise.Evaluate(the_x, the_y, the_z);
            }
        };

        constexpr float lacunarity = 2.0f;
        constexpr float gain       = 0.5f;
        constexpr int   num_layers = 5;

        glm::vec4 noise_result{ 0.0f };
        switch (pattern)
        {
            case Pattern::FractalSum:
                {
                    float amplitude = 0.5f;
                    for (int layer = 0; layer < num_layers; ++layer)
                    {
                        const auto n = amplitude * eval();
                        noise_result += n;
                        the_x *= lacunarity;
                        the_y *= lacunarity;
                        the_z *= lacunarity;
                        amplitude *= gain;
                    }
                }
                return vec4_to_rgba(noise_result);
            case Pattern::Turbulence:
                {
                    float amplitude = 0.5f;
                    for (int layer = 0; layer < num_layers; ++layer)
                    {
                        const auto n = amplitude * glm::abs(2.0f * eval() - 1.0f);
                        noise_result += n;
                        the_x *= lacunarity;
                        the_y *= lacunarity;
                        the_z *= lacunarity;
                        amplitude *= gain;
                    }
                }
                return vec4_to_rgba(noise_result);
            case Pattern::Marble:
                {
                    float       amplitude  = 0.5f;
                    const float the_column = the_x / frequency;
                    for (int layer = 0; layer < num_layers; ++layer)
                    {
                        const auto n = amplitude * eval();
                        noise_result += n;
                        the_x *= lacunarity;
                        the_y *= lacunarity;
                        the_z *= lacunarity;
                        amplitude *= gain;
                    }
                    constexpr float MY_PI = 3.1415926535897932384626433832795028f;
                    noise_result          = (glm::sin((the_column + noise_result * 100.f) * 2.f * MY_PI / 200.f) + 1.f) / 2.f;
                }
                return vec4_to_rgba(noise_result);
            case Pattern::Wood:
                {
                    noise_result = 10.f * eval();
                    noise_result -= glm::floor(noise_result);
                }
                return vec4_to_rgba(noise_result);
            case Pattern::PlainValue:
            default: return vec4_to_rgba(eval());
        }
    }
}

namespace
{
    opengl::Texture::RGBA vec4_to_rgba(const glm::vec4& color)
    {
        const auto                  r    = static_cast<unsigned int>(std::clamp(color.r, 0.0f, 1.0f) * 255.0f);
        const auto                  g    = static_cast<unsigned int>(std::clamp(color.g, 0.0f, 1.0f) * 255.0f);
        const auto                  b    = static_cast<unsigned int>(std::clamp(color.b, 0.0f, 1.0f) * 255.0f);
        const auto                  a    = static_cast<unsigned int>(std::clamp(color.a, 0.0f, 1.0f) * 255.0f);
        const opengl::Texture::RGBA rgba = (a << 24) | (b << 16) | (g << 8) | r;
        return rgba;
    }
}
