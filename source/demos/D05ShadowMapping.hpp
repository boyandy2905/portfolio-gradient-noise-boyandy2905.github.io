/**
 * \file
 * \author Rudy Castan
 * \date 2025 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include "IDemo.hpp"
#include "assets/Reloader.hpp"
#include "graphics/Camera.hpp"
#include "graphics/Mesh.hpp"
#include "opengl/FrameBuffer.hpp"
#include "opengl/Shader.hpp"
#include "opengl/VertexArray.hpp"

namespace demos
{
    class D05ShadowMapping : public IDemo
    {
    public:
        D05ShadowMapping();

        void Update() override;

        void Draw() const override;
        void ImGuiDraw() override;

        void SetDisplaySize([[maybe_unused]] int width, [[maybe_unused]] int height) override;

    private:
        struct ObjectModel
        {
            enum Type
            {
                Clover,
                Plane,
                Cube,
                Sphere,
                Torus,
                Cylinder,
                Cone,
                Count

            };
        };

        struct Shaders
        {
            enum Type
            {
                Shadow,
                Fill,
                WriteDepth,
                ViewDepth,
                Count
            };
        };

        struct SceneObject
        {
            glm::vec3         Center{};
            glm::vec3         EulerAngles{};
            glm::vec3         Scale{};
            size_t            MaterialIndex = 0;
            ObjectModel::Type Model{};

            struct
            {
                glm::vec3 Diffuse;
                glm::vec3 SpecularColor;
                glm::vec3 Ambient;
                float     Shininess;
                bool      CullFaces = true;
            } Material;
        };

        assets::Reloader                                  assetReloader;
        glm::mat4                                         projectionMatrix{ 1.0f };
        glm::mat4                                         lightProjectionMatrix{ 1.0f };
        float                                             lightFOV  = glm::radians(120.0f);
        float                                             lightNear = 0.5f;
        float                                             lightFar  = 45.0f;
        int                                               shadowMapWidth{ 2048 }, shadowMapHeight{ 2048 };
        graphics::Camera                                  camera;
        graphics::Camera                                  lightCamera;
        std::array<opengl::Shader, Shaders::Count>        shaders;
        std::array<graphics::SubMesh, ObjectModel::Count> subMeshes;
        graphics::SubMesh                                 ndcCube;
        graphics::SubMesh                                 ndcQuad;
        std::vector<SceneObject>                          sceneObjects;
        static constexpr glm::vec3                        FogColor{ 0.337f };
        float                                             fogDensity = 0.01f;

        struct
        {
            int x = 0, y = 0, width = 0, height = 0;
        } viewport;

        opengl::FrameBuffer shadowFrameBuffer;

        struct
        {
            bool      IsLookingAround = false;
            glm::vec3 MoveDirection{ 0.0f };
            float     Yaw              = 0;
            float     Pitch            = 0;
            float     radiansPerSecond = 4.0f * 3.1415f;
            float     unitsPerSecond   = 2.5f;
        } spectatorCamera;

        enum class CameraMode
        {
            View,
            Light
        };
        CameraMode cameraMode             = CameraMode::View;
        bool       shouldDrawDepthTexture = false;
        bool       shouldDrawLightFrustum = true;
        bool       DoShadowBehindLight    = true;


        opengl::Texture::DepthComponentSize depthBitSize                    = opengl::Texture::DepthBits24;
        bool                                drawBackFacesForRecordDepthPass = true;
        float                               glPolygonOffset_factor          = 1.0f;
        float                               glPolygonOffset_units           = 0.0f;

    private:
        void renderToDepthBuffer() const;
        void renderToScreen() const;
        void drawLightFrustum() const;
        void drawDepthTexture() const;
        void drawSceneObjects(const opengl::Shader& shader, GLenum culling) const;
        void updateSpectatorCamera(graphics::Camera& the_camera);
        void setupShadowFrameBuffer();
        void buildMeshes();
        void buildScene();
    };

}
