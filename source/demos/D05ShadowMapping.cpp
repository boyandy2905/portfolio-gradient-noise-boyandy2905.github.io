/**
 * \file
 * \author Rudy Castan
 * \author Sehoon Kim(sehoon.kim@digipen.edu)
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#include "D05ShadowMapping.hpp"

#include "graphics/MathHelper.hpp"
#include "opengl/Environment.hpp"
#include "opengl/GL.hpp"
#include "util/Random.hpp"
#include "window/Environment.hpp"
#include "window/input/Environment.hpp"

#include <SDL.h>
#include <algorithm>
#include <array>
#include <glm/ext/matrix_clip_space.hpp>
#include <imgui.h>
#include <iostream>
#include <span>

namespace
{
    namespace asset_paths
    {
        const auto ShadowVertexPath   = "D05ShadowMapping/shadows.vert";
        const auto ShadowFragmentPath = "D05ShadowMapping/shadows.frag";
        const auto ShadowShaderName   = "Shadow Mapping Shader";

        const auto FillVertexPath   = "D05ShadowMapping/fill_3d.vert";
        const auto FillFragmentPath = "D05ShadowMapping/fill_3d.frag";
        const auto FillShaderName   = "Fill Shader";

        const auto ViewDepthVertexPath   = "D05ShadowMapping/view_depth.vert";
        const auto ViewDepthFragmentPath = "D05ShadowMapping/view_depth.frag";
        const auto ViewDepthShaderName   = "View Depth Map Shader";

        const auto WriteDepthVertexPath   = "D05ShadowMapping/fill_3d.vert";
        const auto WriteDepthFragmentPath = "D05ShadowMapping/write_depth.frag";
        const auto WriteDepthShaderName   = "Write Depth Map Shader";
    }

    using namespace std::string_literals;

    namespace Uniforms
    {
        const auto Ambient             = "uAmbient"s;
        const auto Diffuse             = "uDiffuse"s;
        const auto DoShadowBehindLight = "uDoShadowBehindLight";
        const auto FarDistance         = "uFarDistance";
        const auto FogColor            = "uFogColor"s;
        const auto FogDensity          = "uFogDensity"s;
        const auto LightPosition       = "uLightPosition"s;
        const auto ModelMatrix         = "uModelMatrix"s;
        const auto NearDistance        = "uNearDistance";
        const auto NormalMatrix        = "uNormalMatrix"s;
        const auto Projection          = "uProjection"s;
        const auto ShadowMap           = "uShadowMap"s;
        const auto ShadowMatrix        = "uShadowMatrix"s;
        const auto Shininess           = "uShininess"s;
        const auto SpecularColor       = "uSpecularColor"s;
        const auto ViewMatrix          = "uViewMatrix"s;
    }

    namespace camera
    {
        constexpr float FOV          = glm::radians(60.0f);
        constexpr float NearDistance = 0.5f;
        constexpr float FarDistance  = 150.0f;
    }
}

namespace demos
{
    D05ShadowMapping::D05ShadowMapping()
    {
        assetReloader.SetAndAutoReloadShader(shaders[Shaders::Shadow], asset_paths::ShadowShaderName, { asset_paths::ShadowVertexPath, asset_paths::ShadowFragmentPath });
        assetReloader.SetAndAutoReloadShader(shaders[Shaders::Fill], asset_paths::FillShaderName, { asset_paths::FillVertexPath, asset_paths::FillFragmentPath });
        assetReloader.SetAndAutoReloadShader(shaders[Shaders::ViewDepth], asset_paths::ViewDepthShaderName, { asset_paths::ViewDepthVertexPath, asset_paths::ViewDepthFragmentPath });
        assetReloader.SetAndAutoReloadShader(shaders[Shaders::WriteDepth], asset_paths::WriteDepthShaderName, { asset_paths::WriteDepthVertexPath, asset_paths::WriteDepthFragmentPath });

        buildMeshes();
        buildScene();
        setupShadowFrameBuffer();

        const float aspect             = static_cast<float>(window::DisplayWidth) / static_cast<float>(window::DisplayHeight);
        projectionMatrix               = glm::perspective(camera::FOV, aspect, camera::NearDistance, camera::FarDistance);
        const float light_aspect_ratio = static_cast<float>(shadowMapWidth) / static_cast<float>(shadowMapHeight);
        lightProjectionMatrix          = glm::perspective(lightFOV, light_aspect_ratio, lightNear, lightFar);

        constexpr glm::vec3 eye_position{ -5, 5, 5 };
        constexpr glm::vec3 target_position{ 0, 0, 0 };
        camera = graphics::Camera(eye_position, target_position - eye_position, graphics::Camera::WORLD_UP);

        constexpr glm::vec3 light_position{ 4.72f, 2.34f, 5.95f };
        lightCamera = graphics::Camera(light_position, target_position - light_position, graphics::Camera::WORLD_UP);
        GL::Enable(GL_DEPTH_TEST);
        GL::Enable(GL_CULL_FACE);
        GL::GetIntegerv(GL_VIEWPORT, &viewport.x);
    }

    void D05ShadowMapping::Update()
    {
        assetReloader.Update();
        auto& the_camera = (cameraMode == CameraMode::View) ? camera : lightCamera;
        updateSpectatorCamera(the_camera);
        const auto ViewMatrix = the_camera.ViewMatrix();


        const auto  LightViewMatrix    = lightCamera.ViewMatrix();
        const float light_aspect_ratio = static_cast<float>(shadowMapWidth) / static_cast<float>(shadowMapHeight);
        lightProjectionMatrix          = glm::perspective(lightFOV, light_aspect_ratio, lightNear, lightFar);

        const auto& Projection = (cameraMode == CameraMode::View) ? projectionMatrix : lightProjectionMatrix;

        constexpr auto      shadow_bias_column_0 = glm::vec4(0.5f, 0.0f, 0.0f, 0.0f);
        constexpr auto      shadow_bias_column_1 = glm::vec4(0.0f, 0.5f, 0.0f, 0.0f);
        constexpr auto      shadow_bias_column_2 = glm::vec4(0.0f, 0.0f, 0.5f, 0.0f);
        constexpr auto      shadow_bias_column_3 = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
        constexpr glm::mat4 ShadowBias{ shadow_bias_column_0, shadow_bias_column_1, shadow_bias_column_2, shadow_bias_column_3 };
        const auto          ShadowMatrix = ShadowBias * lightProjectionMatrix * LightViewMatrix;

        const auto light_position_viewspace = glm::vec3(ViewMatrix * glm::vec4(lightCamera.Eye, 1.0f));

        shaders[Shaders::Shadow].Use();
        shaders[Shaders::Shadow].SendUniform(Uniforms::DoShadowBehindLight, DoShadowBehindLight);
        shaders[Shaders::Shadow].SendUniform(Uniforms::FogColor, FogColor);
        shaders[Shaders::Shadow].SendUniform(Uniforms::FogDensity, fogDensity);
        shaders[Shaders::Shadow].SendUniform(Uniforms::LightPosition, light_position_viewspace);
        shaders[Shaders::Shadow].SendUniform(Uniforms::Projection, Projection);
        shaders[Shaders::Shadow].SendUniform(Uniforms::ShadowMap, 0);
        shaders[Shaders::Shadow].SendUniform(Uniforms::ShadowMatrix, ShadowMatrix);
        shaders[Shaders::Shadow].SendUniform(Uniforms::ViewMatrix, ViewMatrix);
    }

    void D05ShadowMapping::Draw() const
    {
        renderToDepthBuffer();
        renderToScreen();
        drawLightFrustum();
        drawDepthTexture();
        graphics::DEFAULT_MATERIAL.ForceApplyAllSettings();
    }

    void D05ShadowMapping::ImGuiDraw()
    {
        ImGui::Text("Press and hold any mouse button to look around.\nW,A,S,D,Q,E to move around");
        ImGui::Checkbox("Draw Depth map", &shouldDrawDepthTexture);
        int currentCameraIndex = static_cast<int>(cameraMode);
        if (ImGui::Combo("Camera Mode", &currentCameraIndex, "View\0Light\0\0"))
        {
            cameraMode = static_cast<CameraMode>(currentCameraIndex);
        }
        ImGui::SliderAngle("Light Field of View", &lightFOV, 1.0f, 179.0f);
        ImGui::SliderFloat("Light Near Distance", &lightNear, 0.01f, lightFar - 0.01f);
        ImGui::SliderFloat("Light Far Distance", &lightFar, lightNear + 0.01f, lightNear + 1000.0f);
        ImGui::InputFloat("glPolygonOffset factor", &glPolygonOffset_factor, 0.01f, 1.0f);
        ImGui::InputFloat("glPolygonOffset units", &glPolygonOffset_units, 0.01f, 1.0f);
        ImGui::Checkbox("Draw Back Faces for Recording Depth", &drawBackFacesForRecordDepthPass);
        ImGui::Checkbox("DoShadowBehindLight", &DoShadowBehindLight);
        const auto to_index = [](opengl::Texture::DepthComponentSize bd)
        {
            switch (bd)
            {
                default:
                case opengl::Texture::DepthBits16: return 0;
                case opengl::Texture::DepthBits24: return 1;
                case opengl::Texture::DepthBits32: return 2;
                case opengl::Texture::DepthBits32F: return 3;
            }
        };
        const auto to_enum = [](int index)
        {
            switch (index)
            {
                default:
                case 0: return opengl::Texture::DepthBits16;
                case 1: return opengl::Texture::DepthBits24;
                case 2: return opengl::Texture::DepthBits32;
                case 3: return opengl::Texture::DepthBits32F;
            }
        };
        int current_bit_depth = to_index(depthBitSize);
        if (ImGui::Combo("Depth Component Bit Size", &current_bit_depth, " 16 bits\0 24 bits\0 32 bits\0 32 bit float\0\0"))
        {
            depthBitSize = to_enum(current_bit_depth);
            setupShadowFrameBuffer();
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
            const auto size_itr       = std::find(std::begin(texture_sizes), std::end(texture_sizes), shadowMapWidth);
            int        width_location = (size_itr != std::end(texture_sizes)) ? static_cast<int>(std::distance(std::begin(texture_sizes), size_itr)) : 0;
            if (ImGui::Combo("Shadow Map Dimensions", &width_location, texture_sizes_string.c_str()))
            {
                shadowMapWidth  = texture_sizes[static_cast<size_t>(width_location)];
                shadowMapHeight = shadowMapWidth;
                setupShadowFrameBuffer();
            }
        }
        ImGui::Checkbox("Draw Light Frustum", &shouldDrawLightFrustum);
    }

    void D05ShadowMapping::SetDisplaySize([[maybe_unused]] int width, [[maybe_unused]] int height)
    {
        const float aspect = static_cast<float>(width) / static_cast<float>(height);
        projectionMatrix   = glm::perspective(camera::FOV, aspect, camera::NearDistance, camera::FarDistance);
        GL::GetIntegerv(GL_VIEWPORT, &viewport.x);
    }

    void D05ShadowMapping::renderToDepthBuffer() const
    {
        const auto light_view_matrix = lightCamera.ViewMatrix();

        shaders[Shaders::WriteDepth].Use();
        shaders[Shaders::WriteDepth].SendUniform(Uniforms::Projection, lightProjectionMatrix);
        shaders[Shaders::WriteDepth].SendUniform(Uniforms::ViewMatrix, light_view_matrix);
        shaders[Shaders::WriteDepth].SendUniform(Uniforms::NearDistance, lightNear);
        shaders[Shaders::WriteDepth].SendUniform(Uniforms::FarDistance, lightFar);

        GL::ClearColor(1.0f, 1.0f, 1.0f, 1.0f);

        shadowFrameBuffer.Use(true);

        GL::Viewport(0, 0, shadowMapWidth, shadowMapHeight);
        GL::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        GL::Enable(GL_CULL_FACE);
        GL::Enable(GL_DEPTH_TEST);
        GL::DepthMask(GL_TRUE);

        GL::Enable(GL_POLYGON_OFFSET_FILL);
        GL::PolygonOffset(glPolygonOffset_factor, glPolygonOffset_units);

        const GLenum culling = drawBackFacesForRecordDepthPass ? GL_FRONT : GL_BACK;
        drawSceneObjects(shaders[Shaders::WriteDepth], culling);

        GL::CullFace(GL_BACK);

        shadowFrameBuffer.Use(false);

        GL::Disable(GL_POLYGON_OFFSET_FILL);

        GL::Viewport(viewport.x, viewport.y, viewport.width, viewport.height);
    }

    void D05ShadowMapping::renderToScreen() const
    {
        GL::ClearColor(FogColor.r, FogColor.g, FogColor.b, 1.0f);
        GL::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        GL::Enable(GL_DEPTH_TEST);
        GL::DepthMask(GL_TRUE);

        shadowFrameBuffer.DepthTexture().UseForSlot(0);

        drawSceneObjects(shaders[Shaders::Shadow], GL_BACK);
    }

    void D05ShadowMapping::drawLightFrustum() const
    {
        if (shouldDrawLightFrustum && cameraMode != CameraMode::Light)
        {
            const glm::mat4 model_matrix = lightCamera.ToWorldMatrix() * glm::inverse(lightProjectionMatrix);

            shaders[Shaders::Fill].Use();
            shaders[Shaders::Fill].SendUniform(Uniforms::Projection, projectionMatrix);
            shaders[Shaders::Fill].SendUniform(Uniforms::ViewMatrix, camera.ViewMatrix());
            shaders[Shaders::Fill].SendUniform(Uniforms::Diffuse, glm::vec3(1.0f));
            shaders[Shaders::Fill].SendUniform(Uniforms::ModelMatrix, model_matrix);

            GL::Disable(GL_CULL_FACE);

            ndcCube.VertexArrayObj.Use();
            opengl::DrawIndexed(ndcCube.VertexArrayObj);

            GL::Enable(GL_CULL_FACE);
        }
    }

    void D05ShadowMapping::drawDepthTexture() const
    {
        if (shouldDrawDepthTexture)
        {
            GL::Disable(GL_DEPTH_TEST);
            GL::Disable(GL_CULL_FACE);

            shaders[Shaders::ViewDepth].Use();
            shaders[Shaders::ViewDepth].SendUniform(Uniforms::ShadowMap, 0);

            shadowFrameBuffer.ColorTexture().UseForSlot(0);

            ndcQuad.VertexArrayObj.Use();
            opengl::DrawIndexed(ndcQuad.VertexArrayObj);

            GL::Enable(GL_DEPTH_TEST);
            GL::Enable(GL_CULL_FACE);
        }
    }

    void D05ShadowMapping::drawSceneObjects(const opengl::Shader& shader, GLenum culling) const
    {
        auto&      the_camera = (cameraMode == CameraMode::View) ? camera : lightCamera;
        const auto ViewMatrix = glm::mat3(the_camera.ViewMatrix());
        shader.Use();
        for (const auto& scene_object : sceneObjects)
        {
            const glm::mat4 s            = glm::scale(glm::mat4(1.0f), scene_object.Scale);
            const glm::mat4 r            = graphics::euler_angle_xyz_matrix(scene_object.EulerAngles.x, scene_object.EulerAngles.y, scene_object.EulerAngles.z);
            const glm::mat4 t            = glm::translate(glm::mat4(1.0f), scene_object.Center);
            const auto      ModelMatrix  = t * r * s;
            const auto      NormalMatrix = ViewMatrix * glm::mat3(r);
            shader.SendUniform(Uniforms::ModelMatrix, ModelMatrix);
            shader.SendUniform(Uniforms::NormalMatrix, NormalMatrix);
            shader.SendUniform(Uniforms::Ambient, scene_object.Material.Ambient);
            shader.SendUniform(Uniforms::Diffuse, scene_object.Material.Diffuse);
            shader.SendUniform(Uniforms::SpecularColor, scene_object.Material.SpecularColor);
            shader.SendUniform(Uniforms::Shininess, scene_object.Material.Shininess);
            if (scene_object.Material.CullFaces)
            {
                GL::Enable(GL_CULL_FACE);
                GL::CullFace(culling);
            }
            else
            {
                GL::Disable(GL_CULL_FACE);
            }
            const auto& sub_mesh = subMeshes[scene_object.Model];
            sub_mesh.VertexArrayObj.Use();
            opengl::DrawIndexed(sub_mesh.VertexArrayObj);
        }
    }

    void D05ShadowMapping::updateSpectatorCamera(graphics::Camera& the_camera)
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
                spectatorCamera.Pitch = speed * glm::radians(-input::MouseVelocityY);
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
            const float move_speed = DeltaTime * spectatorCamera.unitsPerSecond;
            spectatorCamera.MoveDirection *= move_speed / length;
            the_camera.Move(spectatorCamera.MoveDirection);
        }

        if (spectatorCamera.Yaw + spectatorCamera.Pitch != 0)
        {
            the_camera.Rotate(spectatorCamera.Yaw, spectatorCamera.Pitch);
        }
    }

    void D05ShadowMapping::setupShadowFrameBuffer()
    {
        opengl::FrameBuffer::Specification spec;
        spec.Width       = shadowMapWidth;
        spec.Height      = shadowMapHeight;
        spec.DepthFormat = depthBitSize;
        spec.ColorFormat = opengl::FrameBuffer::ColorComponent::RGBA8;
        shadowFrameBuffer.LoadWithSpecification(spec);
    }

    void D05ShadowMapping::buildMeshes()
    {
        const std::array geometries = { graphics::create_clover(256, 64), graphics::create_plane(1, 1),     graphics::create_cube(1, 1), graphics::create_sphere(64, 64),
                                        graphics::create_torus(64, 64),    graphics::create_cylinder(4, 64), graphics::create_cone(4, 64) };

        for (size_t i = 0; i < subMeshes.size(); ++i)
        {
            auto& sub_mesh = subMeshes[i];
            sub_mesh       = graphics::to_submesh_as_triangles(geometries[i]);
        }

        auto cube_geometry = geometries[ObjectModel::Cube];
        for (auto& vertex : cube_geometry.Vertices)
        {
            vertex.position *= 2.0f;
        }
        ndcCube = graphics::to_submesh_as_lines(cube_geometry);

        auto plane_geometry = geometries[ObjectModel::Plane];
        for (auto& vertex : plane_geometry.Vertices)
        {
            vertex.position *= 2.0f;
        }
        ndcQuad = graphics::to_submesh_as_triangles(plane_geometry);
    }

    void D05ShadowMapping::buildScene()
    {
        constexpr float SIZE                = 5.0f;
        const auto      set_random_material = [](SceneObject& object)
        {
            glm::vec3 diffuse;
            do
            {
                diffuse.r = util::random();
                diffuse.g = util::random();
                diffuse.b = util::random();
            } while (diffuse.r + diffuse.g + diffuse.b < 1.0f);
            const glm::vec3 ambient       = diffuse * 0.05f;
            const glm::vec3 specular      = diffuse;
            const float     shininess     = util::random(0.5f, 75.0f);
            object.Material.Diffuse       = diffuse;
            object.Material.SpecularColor = specular;
            object.Material.Ambient       = ambient;
            object.Material.Shininess     = shininess;
        };

        { // Ground Plane
            SceneObject ground;
            ground.Model = ObjectModel::Plane;
            set_random_material(ground);
            // so that we get something when recording depth with front face culling
            ground.Material.CullFaces     = false;
            ground.Center                 = { 0, -0.05f, 0 };
            ground.EulerAngles            = { glm::radians(-90.0f), 0, 0 };
            ground.Scale                  = glm::vec3(SIZE * 1000);
            ground.Material.SpecularColor = glm::vec3(0.0f);
            sceneObjects.push_back(ground);
        }
        {
            SceneObject pillar;
            pillar.Model           = ObjectModel::Cylinder;
            constexpr float height = SIZE;
            set_random_material(pillar);
            pillar.Center      = { 0, height * 0.5f, 0 };
            pillar.EulerAngles = { 0, 0, 0 };
            pillar.Scale       = glm::vec3(2.0f, height, 1.0f);
            sceneObjects.push_back(pillar);
        }

        std::vector<glm::vec2> ground_points;
        for (int row = 1; row <= 3; ++row)
        {
            const float row_scale = 1.0f / static_cast<float>(row);
            for (int column = 1; column <= 3; ++column)
            {
                const float column_scale = 1.0f / static_cast<float>(column);
                ground_points.emplace_back(row_scale, column_scale);
                ground_points.emplace_back(-row_scale, -column_scale);
                ground_points.emplace_back(-row_scale, column_scale);
                ground_points.emplace_back(row_scale, -column_scale);
            }
        }
        for (auto position : ground_points)
        {
            SceneObject sphere;
            sphere.Model = ObjectModel::Sphere;
            set_random_material(sphere);
            position *= SIZE * 0.9f;
            sphere.Center      = { position.x, 0, position.y };
            sphere.EulerAngles = { 0, 0, 0 };
            sphere.Scale       = glm::vec3(SIZE / 8.0f);
            sceneObjects.push_back(sphere);
        }

        for (int i = 0; i < 20; ++i)
        {
            SceneObject object;
            object.Model = static_cast<ObjectModel::Type>(util::random(ObjectModel::Count));
            set_random_material(object);
            if (object.Model == ObjectModel::Plane)
            {
                object.Material.CullFaces = false;
            }
            const float x      = util::random(-SIZE, SIZE);
            const float y      = util::random(0.25f, 1.5f);
            const float z      = util::random(-SIZE, SIZE);
            object.Center      = { x, y, z };
            object.EulerAngles = { util::random(-3.14f, 3.14f), util::random(-3.14f, 3.14f), util::random(-3.14f, 3.14f) };
            object.Scale       = { util::random(SIZE / 20.0f, SIZE / 8.0f), util::random(SIZE / 20.0f, SIZE / 8.0f), util::random(SIZE / 20.0f, SIZE / 8.0f) };
            sceneObjects.push_back(object);
        }
    }
}
