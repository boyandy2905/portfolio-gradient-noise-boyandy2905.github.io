/**
 * \file
 * \author Rudy Castan
 * \date 2024 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#include "D07GradientNoise.hpp"

#include "graphics/MathHelper.hpp"
#include "opengl/Environment.hpp"
#include "opengl/GL.hpp"
#include "opengl/Texture.hpp"
#include "window/Environment.hpp"
#include "window/input/Environment.hpp"
#include <SDL.h>
#include <glm/ext/matrix_clip_space.hpp> // perspective
#include <imgui.h>
#include <span>

namespace
{
    using namespace std::string_literals;

    namespace Uniforms
    {
        const auto Projection  = "uProjection"s;
        const auto TileScale   = "uTileScale"s;
        const auto Z           = "uZ"s;
        const auto Pattern     = "uPattern"s;
        const auto ModelMatrix = "uModelMatrix"s;
        const auto ViewMatrix  = "uViewMatrix"s;

    }

    namespace asset_paths
    {
        constexpr auto SimpleVertexPath   = "D07GradientNoise/simple.vert";
        constexpr auto SimpleFragmentPath = "D07GradientNoise/simple.frag";

        constexpr auto GenGradientNoiseVertexPath   = "D07GradientNoise/gen_gradient_noise.vert";
        constexpr auto GenGradientNoiseFragmentPath = "D07GradientNoise/gen_gradient_noise.frag";

        constexpr auto SurfaceVertexPath   = "D07GradientNoise/surface.vert";
        constexpr auto SurfaceFragmentPath = "D07GradientNoise/simple.frag";
    }

    namespace camera
    {
        constexpr float FOV          = glm::radians(60.0f);
        constexpr float NearDistance = 0.05f;
        constexpr float FarDistance  = 100.0f;
    }
}

namespace demos
{
    D07GradientNoise::D07GradientNoise()
    {
        GL::ClearColor(0.392f, 0.584f, 0.929f, 1.0f);
        assetReloader.SetAndAutoReloadShader(generateGradientNoiseShader, "Generate Gradient Noise Shader", { asset_paths::GenGradientNoiseVertexPath, asset_paths::GenGradientNoiseFragmentPath });
        assetReloader.SetAndAutoReloadShader(displayTextureShader, "Display Texture Shader", { asset_paths::SimpleVertexPath, asset_paths::SimpleFragmentPath });
        assetReloader.SetAndAutoReloadShader(surfaceShader, "Surface Shader", { asset_paths::SurfaceVertexPath, asset_paths::SurfaceFragmentPath });

        setupNoiseFrameBuffer();

        buildSurfaceMesh();

        graphics::Geometry full_screen_geometry = graphics::create_plane(1, 1);
        for (auto& vert : full_screen_geometry.Vertices)
        {
            vert.position *= 2.0f;
        }
        quadMesh = graphics::to_submesh_as_triangles(full_screen_geometry);

        D07GradientNoise::SetDisplaySize(window::DisplayWidth, window::DisplayHeight);

        constexpr glm::vec3 eye_position{ -5, 5, 5 };
        constexpr glm::vec3 target_position{ 0, 0, 0 };
        camera = graphics::Camera(eye_position, target_position - eye_position, graphics::Camera::WORLD_UP);
    }

    void D07GradientNoise::Update()
    {
        assetReloader.Update();
        constexpr float FUDGE_FACTOR = 1.25;
        const auto      easing       = std::min(window::DeltaTime * FUDGE_FACTOR, 1.0f);
        tileScale += easing * (targetTileScale - tileScale);
        z += std::min(easing, 1.0f / 2048.0f) * (targetZ - z);
        if (viewNoise == ViewNoise::Surface)
            updateSpectatorCamera();
    }

    void D07GradientNoise::Draw() const
    {
        graphics::DEFAULT_MATERIAL.ForceApplyAllSettings();
        generateGradientNoiseShader.Use();
        generateGradientNoiseShader.SendUniform(Uniforms::Z, z);
        generateGradientNoiseShader.SendUniform(Uniforms::Pattern, static_cast<int>(pattern));
        noiseFrameBuffer.Use();
        GL::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        GL::Viewport(0, 0, textureSize, textureSize);
        quadMesh.VertexArrayObj.Use();
        opengl::DrawIndexed(quadMesh.VertexArrayObj);
        noiseFrameBuffer.Use(false);

        if (viewNoise != ViewNoise::Surface)
        {
            displayTextureShader.Use();
            displayTextureShader.SendUniform(Uniforms::Projection, orthoProjectionMatrix);
            displayTextureShader.SendUniform(Uniforms::TileScale, tileScale);
            noiseFrameBuffer.ColorTexture().UseForSlot(0);
            GL::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            GL::Viewport(viewport.x, viewport.y, viewport.width, viewport.height);
            quadMesh.VertexArrayObj.Use();
            opengl::DrawIndexed(quadMesh.VertexArrayObj);
        }
        else
        {
            const glm::mat4 s          = glm::scale(glm::mat4(1.0f), glm::vec3(surfaceScale, surfaceScale, heightScale));
            const glm::mat4 r          = graphics::euler_angle_xyz_matrix(glm::radians(-90.0f), 0, 0);
            const auto      ViewMatrix = camera.ViewMatrix();
            noiseFrameBuffer.ColorTexture().UseForSlot(0);
            GL::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            GL::Viewport(viewport.x, viewport.y, viewport.width, viewport.height);
            surfaceShader.Use();
            surfaceShader.SendUniform(Uniforms::TileScale, tileScale);
            surfaceShader.SendUniform(Uniforms::ModelMatrix, r * s);
            surfaceShader.SendUniform(Uniforms::ViewMatrix, ViewMatrix);
            surfaceShader.SendUniform(Uniforms::Projection, projectionMatrix);
            surfaceMesh.VertexArrayObj.Use();
            opengl::DrawIndexed(surfaceMesh.VertexArrayObj);
        }
    }

    void D07GradientNoise::ImGuiDraw()
    {
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
                textureSize = texture_sizes[static_cast<size_t>(width_location)];
                setupNoiseFrameBuffer();
            }
        }

        ImGui::SliderFloat("Tiling Scale", &targetTileScale, 0.1f, 3.0f);


        ImGui::SliderFloat("Z Input", &targetZ, 0.0f, 64.0f);
        ImGui::SameLine();
        ImGui::Text("(%.3f)", static_cast<double>(z));

        constexpr auto view_mode_names = "Texture\0Surface\0\0";
        ImGui::Combo("View Noise", reinterpret_cast<int*>(&viewNoise), view_mode_names);

        constexpr auto pattern_type_names = "Gradient\0Fractal Sum\0Turbulence\0Marble\0Wood\0\0";
        ImGui::Combo("Pattern Type", reinterpret_cast<int*>(&pattern), pattern_type_names);

        if (viewNoise == ViewNoise::Surface)
        {
            ImGui::SliderFloat("Area Scale", &surfaceScale, 1.0f, 100.0f);
            ImGui::SliderFloat("Height Scale", &heightScale, 0.01f, 10.0f);
            bool rebuild_shapes = ImGui::SliderInt("Stacks", &stacks, 1, 500);
            rebuild_shapes      = ImGui::SliderInt("Slices", &slices, 1, 500) || rebuild_shapes;
            if (rebuild_shapes)
            {
                buildSurfaceMesh();
            }
        }
    }

    void D07GradientNoise::SetDisplaySize(int width, int height)
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

        const float aspect = static_cast<float>(width) / static_cast<float>(height);
        projectionMatrix   = glm::perspective(camera::FOV, aspect, camera::NearDistance, camera::FarDistance);

        GL::GetIntegerv(GL_VIEWPORT, &viewport.x);
    }

    void D07GradientNoise::setupNoiseFrameBuffer()
    {
        opengl::FrameBuffer::Specification spec;
        spec.Width       = textureSize;
        spec.Height      = textureSize;
        spec.DepthFormat = opengl::Texture::None;
        spec.ColorFormat = opengl::FrameBuffer::ColorComponent::R32F;
        noiseFrameBuffer.LoadWithSpecification(spec);
        auto& texture = noiseFrameBuffer.ColorTexture();
        texture.SetFiltering(opengl::Texture::NearestPixel); // FIXED
        texture.SetWrapping(opengl::Texture::Repeat);
    }

    void D07GradientNoise::buildSurfaceMesh()
    {
        const auto g = graphics::create_plane(stacks, slices);
        surfaceMesh  = graphics::to_submesh_as_triangles(g);
    }

    void D07GradientNoise::updateSpectatorCamera()
    {
        using namespace window;
        using namespace window::input;
        spectatorCamera.MoveDirection = glm::vec3(0);

        if (contains(PressedKeyboardButtons, KeyboardButtons::W, KeyboardButtons::Up, KeyboardButtons::NumPad_8))
        {
            spectatorCamera.MoveDirection.z += 1.0f;
        }

        if (contains(PressedKeyboardButtons, KeyboardButtons::S, KeyboardButtons::Down, KeyboardButtons::NumPad_2))
        {
            spectatorCamera.MoveDirection.z -= 1.0f;
        }

        if (contains(PressedKeyboardButtons, KeyboardButtons::A, KeyboardButtons::Left, KeyboardButtons::NumPad_4))
        {
            spectatorCamera.MoveDirection.x -= 1.0f;
        }

        if (contains(PressedKeyboardButtons, KeyboardButtons::D, KeyboardButtons::Right, KeyboardButtons::NumPad_6))
        {
            spectatorCamera.MoveDirection.x += 1.0f;
        }

        if (contains(PressedKeyboardButtons, KeyboardButtons::Q, KeyboardButtons::NumPad_7))
        {
            spectatorCamera.MoveDirection.y -= 1.0f;
        }

        if (contains(PressedKeyboardButtons, KeyboardButtons::E, KeyboardButtons::NumPad_9))
        {
            spectatorCamera.MoveDirection.y += 1.0f;
        }


        spectatorCamera.Yaw                     = 0.0f;
        spectatorCamera.Pitch                   = 0.0f;
        const auto mouse_is_inside_display_area = []()
        {
            constexpr float border_width = 32.0f;
            return input::MouseDisplayX >= border_width && input::MouseDisplayY >= border_width && input::MouseDisplayX < static_cast<float>(DisplayWidth) - border_width &&
                   input::MouseDisplayY < static_cast<float>(DisplayHeight) - border_width;
        };
        if (!input::PressedMouseButtons.empty())
        {
            if (spectatorCamera.IsLookingAround)
            {
                const float speed     = DeltaTime * spectatorCamera.radiansPerSecond;
                spectatorCamera.Yaw   = speed * glm::radians(-input::MouseVelocityX);
                spectatorCamera.Pitch = speed * glm::radians(input::MouseVelocityY);
            }
            else if (mouse_is_inside_display_area())
            {
                spectatorCamera.IsLookingAround = true;
                SDL_SetRelativeMouseMode(SDL_TRUE);
            }
        }
        else if (spectatorCamera.IsLookingAround)
        {
            spectatorCamera.IsLookingAround = false;
            SDL_SetRelativeMouseMode(SDL_FALSE);
        }

        if (float length = glm::dot(spectatorCamera.MoveDirection, spectatorCamera.MoveDirection); length > 0)
        {
            length                 = std::sqrt(length);
            const float move_speed = static_cast<float>(DeltaTime) * spectatorCamera.unitsPerSecond;
            spectatorCamera.MoveDirection *= move_speed / length;
            camera.Move(spectatorCamera.MoveDirection);
        }

        if (spectatorCamera.Yaw + spectatorCamera.Pitch != 0)
        {
            camera.Rotate(spectatorCamera.Yaw, spectatorCamera.Pitch);
        }
    }
}
